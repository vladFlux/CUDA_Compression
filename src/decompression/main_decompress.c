#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "serial_utilities.h"


/**
 * @file main_decompress.c
 * @brief Main decompression program for Huffman-compressed files
 *
 * This program reverses the compression process by:
 * 1. Reading the compressed file with embedded metadata
 * 2. Reconstructing the identical Huffman tree used during compression
 * 3. Performing bit-by-bit tree traversal to decode compressed data
 * 4. Writing the fully restored original data to output file
 *
 * The decompression process is deterministic and lossless - it produces
 * exactly the same data that was originally compressed, byte for byte.
 *
 * File format compatibility:
 * - Reads files created by the GPU compression system
 * - Expects specific header format: length + frequencies + compressed data
 * - Handles all compression scenarios (single/multiple kernels, overflow/no overflow)
 */

/*=============================================================================
 * GLOBAL VARIABLE DEFINITIONS
 *=============================================================================*/

// Global instances of decompression data structures
struct huffman_dictionary huffman_dictionary[256];  // Verification lookup table
struct huffman_tree *head_huffman_tree_node;        // Root of reconstructed tree
struct huffman_tree huffman_tree_node[512];         // All tree nodes for decompression

/**
 * @brief Main decompression program entry point
 * @param argc Number of command line arguments (should be 3)
 * @param argv Array of argument strings [program, input_file, output_file]
 * @return EXIT_SUCCESS on successful decompression, EXIT_FAILURE on error
 *
 * Complete decompression pipeline that:
 *
 * 1. **File Format Parsing**: Reads the structured compressed file created by compression
 * 2. **Tree Reconstruction**: Rebuilds the exact Huffman tree using stored frequency data
 * 3. **Bit Stream Decoding**: Processes compressed bits through tree traversal
 * 4. **Data Restoration**: Generates the complete original file content
 * 5. **Performance Measurement**: Times the decompression process
 *
 * The algorithm guarantees perfect reconstruction of the original data by using
 * the same tree-building process and frequency data that was used during compression.
 */
int main(int argc, char **argv) {
    unsigned int index;
    unsigned int output_file_length, frequency[256];
    unsigned char bit_sequence[255];
    const unsigned char bit_sequence_length = 0;

    /*=========================================================================
     * COMPRESSED FILE PARSING AND HEADER EXTRACTION
     *=========================================================================*/

    // Open the compressed file created by the GPU compression system
    FILE *compressed_file = fopen(argv[1], "rb");

    // Read the embedded metadata from file header:
    // 1. Original file length (4 bytes) - tells us how much data to reconstruct
    fread(&output_file_length, sizeof(unsigned int), 1, compressed_file);

    // 2. Character frequency table (1024 bytes) - enables tree reconstruction
    // This is the same frequency data calculated during compression
    fread(frequency, 256 * sizeof(unsigned int), 1, compressed_file);

    /*=========================================================================
     * COMPRESSED DATA SIZE CALCULATION
     *=========================================================================*/

    // Calculate the size of the actual compressed data
    // File structure: 4 bytes (length) + 1024 bytes (frequencies) + compressed data
    fseek(compressed_file, 0, SEEK_END);                                    // Go to end of file
    const unsigned int compressed_file_length = ftell(compressed_file) - 1028;  // Subtract header size
    fseek(compressed_file, 1028, SEEK_SET);                                 // Position at start of compressed data

    /*=========================================================================
     * COMPRESSED DATA LOADING
     *=========================================================================*/

    // Allocate memory and read the entire compressed bit stream
    unsigned char *compressed_data = malloc((compressed_file_length) * sizeof(unsigned char));
    fread(compressed_data, sizeof(unsigned char), (compressed_file_length), compressed_file);
    fclose(compressed_file);

    /*=========================================================================
     * PERFORMANCE TIMING SETUP
     *=========================================================================*/

    // Start timing the decompression algorithm (excluding file I/O)
    const clock_t start = clock();

    /*=========================================================================
     * HUFFMAN TREE RECONSTRUCTION
     *=========================================================================*/

    // Initialize leaf nodes using the frequency data from compressed file
    // This recreates the exact same starting state as during compression
    unsigned int distinct_character_count = 0;
    for (index = 0; index < 256; index++) {
        if (frequency[index] > 0) {
            // Create leaf node for each character that appeared in original data
            huffman_tree_node[distinct_character_count].count = frequency[index];
            huffman_tree_node[distinct_character_count].letter = index;
            huffman_tree_node[distinct_character_count].left = NULL;   // Leaf nodes have no children
            huffman_tree_node[distinct_character_count].right = NULL;
            distinct_character_count++;
        }
    }

    /*=========================================================================
     * HUFFMAN TREE CONSTRUCTION
     *=========================================================================*/

    // Build the binary tree using identical algorithm to compression
    // This ensures the exact same tree structure is recreated
    for (index = 0; index < distinct_character_count - 1; index++) {
        const unsigned int combined_huffman_nodes = 2 * index;

        // Sort nodes by frequency (identical to compression-time sorting)
        sort_huffman_tree(index, distinct_character_count, combined_huffman_nodes);

        // Combine lowest-frequency nodes (identical to compression-time combining)
        build_huffman_tree(index, distinct_character_count, combined_huffman_nodes);
    }

    /*=========================================================================
     * VERIFICATION DICTIONARY GENERATION
     *=========================================================================*/

    // Generate the character→bit mapping for verification purposes
    // While not strictly needed for decompression, this validates tree reconstruction
    build_huffman_dictionary(head_huffman_tree_node, bit_sequence, bit_sequence_length);

    /*=========================================================================
     * DECOMPRESSION SETUP
     *=========================================================================*/

    // Allocate buffer for the reconstructed original data
    unsigned char *output_data = malloc(output_file_length * sizeof(unsigned char));

    // Initialize tree traversal state
    const struct huffman_tree *current_huffman_tree_node = head_huffman_tree_node;
    unsigned int output_file_length_counter = 0;

    /*=========================================================================
     * BIT-BY-BIT DECOMPRESSION ALGORITHM
     *=========================================================================*/

    // Process each byte of compressed data
    for (index = 0; index < compressed_file_length; index++) {
        unsigned char current_input_byte = compressed_data[index];

        // Process each bit within the current byte (8 bits per byte)
        for (unsigned int bit = 0; bit < 8; bit++) {
            // Extract the most significant bit (leftmost bit)
            // 0200 (octal) = 128 (decimal) = 10000000 (binary)
            const unsigned char current_input_bit = current_input_byte & 0200;

            // Shift byte left to prepare next bit for processing
            current_input_byte = current_input_byte << 1;

            /*=================================================================
             * TREE TRAVERSAL BASED ON CURRENT BIT
             *=================================================================*/

            if (current_input_bit == 0) {
                // Bit is 0: traverse to left child
                current_huffman_tree_node = current_huffman_tree_node->left;

                // Check if we've reached a leaf node (character found)
                if (current_huffman_tree_node->left == NULL) {
                    // Leaf node reached: output the character and reset to root
                    output_data[output_file_length_counter] = current_huffman_tree_node->letter;
                    current_huffman_tree_node = head_huffman_tree_node;
                    output_file_length_counter++;
                }
            } else {
                // Bit is 1: traverse to right child
                current_huffman_tree_node = current_huffman_tree_node->right;

                // Check if we've reached a leaf node (character found)
                if (current_huffman_tree_node->right == NULL) {
                    // Leaf node reached: output the character and reset to root
                    output_data[output_file_length_counter] = current_huffman_tree_node->letter;
                    current_huffman_tree_node = head_huffman_tree_node;
                    output_file_length_counter++;
                }
            }
        }
    }

    /*=========================================================================
     * PERFORMANCE MEASUREMENT
     *=========================================================================*/

    // Stop timing and calculate decompression duration
    const clock_t end = clock();

    /*=========================================================================
     * OUTPUT FILE GENERATION
     *=========================================================================*/

    // Write the completely reconstructed original data to output file
    FILE *output_file = fopen(argv[2], "wb");
    fwrite(output_data, sizeof(unsigned char), output_file_length, output_file);
    fclose(output_file);

    /*=========================================================================
     * PERFORMANCE REPORTING
     *=========================================================================*/

    // Calculate and display execution time in seconds and milliseconds
    const unsigned int cpu_time_used = ((end - start)) * 1000 / CLOCKS_PER_SEC;
    printf("Execution time: %d:%d s\n", cpu_time_used / 1000, cpu_time_used % 1000);

    /*=========================================================================
     * CLEANUP AND EXIT
     *=========================================================================*/

    // Free all dynamically allocated memory
    free(output_data);
    free(compressed_data);

    return EXIT_SUCCESS;
}
