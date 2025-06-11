#include <cstdio>
#include <cstring>
#include <climits>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "parallel_utilities.h"

/**
 * @file main_compress.cu
 * @brief Main entry point for GPU-accelerated Huffman compression system
 *
 * This file orchestrates the complete compression pipeline:
 * 1. File I/O and validation
 * 2. Character frequency analysis
 * 3. Huffman tree construction
 * 4. GPU resource analysis and optimization
 * 5. Compression execution
 * 6. Output file generation with embedded metadata
 *
 * The system automatically adapts to available GPU memory and file characteristics,
 * choosing optimal compression strategies without user intervention.
 */

// Minimum GPU scratch space required for safe operation (50MB)
// This ensures enough memory for temporary buffers and GPU operations
#define MIN_SCRATCH_SIZE (50 * 1024 * 1024)

/*=============================================================================
 * GLOBAL VARIABLE DEFINITIONS
 *=============================================================================*/

// Global instances of the core data structures defined in header
huffman_tree *head_huffman_tree_node; // Root of constructed Huffman tree
huffman_tree huffman_tree_node[512]; // Static array for all tree nodes
struct huffman_dictionary huffman_dictionary; // Main encoding lookup table
unsigned char bit_sequence_const_memory[256][255]; // Host storage for long bit sequences
unsigned int const_memory_flag = 0; // Flag for constant memory usage

/**
 * @brief Main compression program entry point
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return EXIT_SUCCESS on successful compression, EXIT_FAILURE on error
 *
 * Implements the complete Huffman compression pipeline with automatic
 * GPU optimization and resource management. The program:
 *
 * 1. **File Processing**: Reads input file and validates arguments
 * 2. **Statistical Analysis**: Calculates character frequencies
 * 3. **Tree Construction**: Builds optimal Huffman encoding tree
 * 4. **GPU Analysis**: Determines optimal compression strategy based on:
 *    - Available GPU memory
 *    - File size and compression ratio
 *    - Integer overflow potential
 * 5. **Compression Execution**: Launches appropriate GPU kernels
 * 6. **Output Generation**: Creates compressed file with metadata
 *
 * The output file format includes:
 * - Original file length (4 bytes)
 * - Character frequency table (1024 bytes)
 * - Compressed data (variable length)
 *
 * This allows for complete decompression without external metadata.
 */
int main(const int argc, char **argv) {
    unsigned int index;
    unsigned int input_file_length, frequency[256];
    constexpr unsigned char bit_sequence_length = 0;
    unsigned char bit_sequence[255];
    long unsigned int mem_free, mem_total;

    /*=========================================================================
     * ARGUMENT VALIDATION AND FILE INPUT
     *=========================================================================*/

    // Validate command line arguments
    if (argc != 3) {
        std::cerr << "Invalid number of arguments." << std::endl <<
                "Example: <path_to_input_file> <path_to_output_file>" << std::endl;
        return EXIT_FAILURE;
    }

    // Read entire input file into memory
    // Using binary mode to handle all file types correctly
    FILE *input_file = fopen(argv[1], "rb");
    fseek(input_file, 0, SEEK_END); // Seek to end to get file size
    input_file_length = ftell(input_file); // Get file size in bytes
    fseek(input_file, 0, SEEK_SET); // Return to beginning for reading

    // Allocate memory buffer for entire file content
    auto *input_file_data = static_cast<unsigned char *>(malloc(input_file_length * sizeof(unsigned char)));
    fread(input_file_data, sizeof(unsigned char), input_file_length, input_file);
    fclose(input_file);

    /*=========================================================================
     * PERFORMANCE TIMING SETUP
     *=========================================================================*/

    // Start high-resolution timer for total execution time measurement
    const auto start = std::chrono::high_resolution_clock::now();

    /*=========================================================================
     * CHARACTER FREQUENCY ANALYSIS
     *=========================================================================*/

    // Initialize frequency array for all possible byte values (0-255)
    for (index = 0; index < 256; index++) {
        frequency[index] = 0;
    }

    // Count occurrence of each character in input data
    // This statistical analysis determines the optimal Huffman tree structure
    for (index = 0; index < input_file_length; index++) {
        frequency[input_file_data[index]]++;
    }

    /*=========================================================================
     * HUFFMAN TREE INITIALIZATION
     *=========================================================================*/

    // Create leaf nodes for each character that appears in the input
    // Only characters with non-zero frequency get nodes in the tree
    unsigned int distinct_character_count = 0;
    for (index = 0; index < 256; index++) {
        if (frequency[index] > 0) {
            huffman_tree_node[distinct_character_count].count = frequency[index];
            huffman_tree_node[distinct_character_count].letter = index;
            huffman_tree_node[distinct_character_count].left = nullptr; // Leaf nodes have no children
            huffman_tree_node[distinct_character_count].right = nullptr;
            distinct_character_count++;
        }
    }

    /*=========================================================================
     * HUFFMAN TREE CONSTRUCTION
     *=========================================================================*/

    // Build the binary tree by repeatedly combining lowest-frequency nodes
    // This implements the classic Huffman algorithm for optimal prefix codes
    for (index = 0; index < distinct_character_count - 1; index++) {
        const unsigned int combined_huffman_nodes = 2 * index;

        // Sort remaining nodes by frequency (lowest first)
        sort_huffman_tree(index, distinct_character_count, combined_huffman_nodes);

        // Combine the two lowest-frequency nodes into a new internal node
        build_huffman_tree(index, distinct_character_count, combined_huffman_nodes);
    }

    // Special case: if only one unique character exists, tree is just that character
    if (distinct_character_count == 1) {
        head_huffman_tree_node = &huffman_tree_node[0];
    }

    /*=========================================================================
     * HUFFMAN DICTIONARY GENERATION
     *=========================================================================*/

    // Traverse the completed tree to generate bit sequences for each character
    // Characters with higher frequency get shorter bit sequences
    build_huffman_dictionary(head_huffman_tree_node, bit_sequence, bit_sequence_length);

    /*=========================================================================
     * GPU MEMORY ANALYSIS AND OPTIMIZATION
     *=========================================================================*/

    // Query available GPU memory to determine compression strategy
    if (const cudaError_t cuda_status = cudaMemGetInfo(&mem_free, &mem_total); cuda_status != cudaSuccess) {
        std::cerr << "Failed to get GPU memory info: " << cudaGetErrorString(cuda_status) << std::endl;
        return EXIT_FAILURE;
    }

    // Display GPU memory information for user awareness
    std::cout << std::left << std::setw(25) << "Total GPU VRAM: " << std::right << std::setw(20) <<
            mem_total / (1024 * 1024) << " MB" << std::endl;
    std::cout << std::left << std::setw(25) << "Free GPU VRAM:  " << std::right << std::setw(20) <<
            mem_free / (1024 * 1024) << " MB" << std::endl;

    /*=========================================================================
     * COMPRESSION SIZE CALCULATION
     *=========================================================================*/

    // Calculate total compressed size in bits by summing each character's contribution
    // Each character contributes: frequency × bit_sequence_length
    long unsigned int mem_offset = 0;
    for (index = 0; index < 256; index++) {
        mem_offset += frequency[index] * huffman_dictionary.bit_sequence_length[index];
    }

    // Round up to nearest byte boundary for proper bit packing
    mem_offset = mem_offset % 8 == 0 ? mem_offset : mem_offset + 8 - mem_offset % 8;

    /*=========================================================================
     * GPU MEMORY REQUIREMENT CALCULATION
     *=========================================================================*/

    // Calculate fixed memory requirements for GPU compression:
    // - Input data array
    // - Bit offset array (input_file_length + 1 elements)
    // - Huffman dictionary structure
    const long unsigned int mem_data = input_file_length + (input_file_length + 1) * sizeof(unsigned int) + sizeof(
                                           huffman_dictionary);

    // Verify sufficient GPU memory exists for compression
    if (mem_free - mem_data < MIN_SCRATCH_SIZE) {
        printf("\nExiting : Not enough memory on GPU\nmem_free = %lu\nmin_mem_req = %lu\n", mem_free,
               mem_data + MIN_SCRATCH_SIZE);
        return EXIT_FAILURE;
    }

    /*=========================================================================
     * COMPRESSION STRATEGY DETERMINATION
     *=========================================================================*/

    // Calculate available memory for compressed data buffers (with 10MB safety margin)
    const long unsigned int mem_req = mem_free - mem_data - 10 * 1024 * 1024;

    // Determine number of kernel runs needed based on memory constraints
    // If compressed data fits in GPU memory: 1 run
    // If not: multiple runs with chunking
    const int num_kernel_runs = ceil(static_cast<double>(mem_offset) / mem_req);

    // Determine if integer overflow is possible in bit offset calculations
    // Check if memory requirements or compressed size could exceed UINT_MAX
    const unsigned int integer_overflow_flag = mem_req + 255 <= UINT_MAX || mem_offset + 255 <= UINT_MAX ? 0 : 1;

    /*=========================================================================
     * COMPRESSION STATISTICS DISPLAY
     *=========================================================================*/

    // Display compression information for user feedback
    std::cout << std::left << std::setw(25) << "Input file size: " << std::right << std::setw(20)
            << input_file_length << "  B" << std::endl;
    std::cout << std::left << std::setw(25) << "Compressed file size: " << std::right << std::setw(20)
            << mem_offset / 8 << "  B" << std::endl;

    /*=========================================================================
     * OFFSET ARRAY ALLOCATION AND COMPRESSION EXECUTION
     *=========================================================================*/

    // Allocate array for storing cumulative bit offsets
    // This array tells GPU threads exactly where to write compressed bits
    auto *compressed_data_offset = static_cast<unsigned int *>(malloc((input_file_length + 1) * sizeof(unsigned int)));

    // Launch the GPU compression pipeline
    // This function automatically handles all complexity:
    // - Offset array generation
    // - GPU memory management
    // - Kernel selection based on scenario
    // - Result retrieval
    launch_cuda_huffman_compress(input_file_data, compressed_data_offset, input_file_length, num_kernel_runs,
                                 integer_overflow_flag, mem_req);

    /*=========================================================================
     * PERFORMANCE MEASUREMENT
     *=========================================================================*/

    // Stop timer and calculate total execution time
    const auto end = std::chrono::high_resolution_clock::now();

    /*=========================================================================
     * COMPRESSED FILE OUTPUT
     *=========================================================================*/

    // Write compressed file with embedded metadata for decompression:
    // 1. Original file length (4 bytes) - needed to allocate decompression buffer
    // 2. Character frequency table (1024 bytes) - needed to reconstruct Huffman tree
    // 3. Compressed data (variable length) - the actual compressed content
    FILE *compressed_file = fopen(argv[2], "wb");
    fwrite(&input_file_length, sizeof(unsigned int), 1, compressed_file); // Original size
    fwrite(frequency, sizeof(unsigned int), 256, compressed_file); // Frequency table
    fwrite(input_file_data, sizeof(unsigned char), mem_offset / 8, compressed_file); // Compressed data
    fclose(compressed_file);

    /*=========================================================================
     * PERFORMANCE REPORTING
     *=========================================================================*/

    // Calculate and display execution time with millisecond precision
    const auto duration = std::chrono::duration<double>(end - start);
    const double total_seconds = duration.count();
    const int seconds = static_cast<int>(total_seconds);
    const int milliseconds = static_cast<int>((total_seconds - seconds) * 1000);

    std::cout << std::left << std::setw(25) << "Execution time: " << std::right << std::setw(15)
            << seconds << "s" << std::setw(5) << milliseconds << "ms" << std::endl;

    /*=========================================================================
     * CLEANUP AND EXIT
     *=========================================================================*/

    // Free all dynamically allocated memory
    free(input_file_data);
    free(compressed_data_offset);

    return EXIT_SUCCESS;
}
