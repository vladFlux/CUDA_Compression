#pragma once

/**
 * @file parallel_utilities.h
 * @brief Header file for GPU-accelerated Huffman compression system
 *
 * This header defines the core data structures and function interfaces for a
 * sophisticated parallel Huffman compression implementation that handles:
 * - Variable file sizes (small to very large)
 * - Integer overflow in bit offset calculations
 * - GPU memory limitations through chunking
 * - Hybrid memory management for optimal GPU performance
 *
 * The system automatically adapts compression strategy based on file characteristics
 * and available GPU resources, providing optimal performance across all scenarios.
 */

/*=============================================================================
 * CORE DATA STRUCTURES
 *=============================================================================*/

/**
 * @struct huffman_dictionary
 * @brief GPU-optimized storage for Huffman encoding lookup table
 *
 * This structure is designed for efficient GPU memory access patterns:
 * - bit_sequence[256][191]: Stores first 191 bits of each character's Huffman code
 * - bit_sequence_length[256]: Length of complete bit sequence for each character
 *
 * The 191-bit limitation for shared memory storage is a GPU optimization:
 * - Sequences ≤191 bits: stored entirely here for fastest access
 * - Sequences >191 bits: first 191 bits here, remainder in constant memory
 *
 * This hybrid approach balances memory bandwidth and cache efficiency on GPU.
 */
struct huffman_dictionary {
    unsigned char bit_sequence[256][191]; // Fast-access bit sequences (shared memory)
    unsigned char bit_sequence_length[256]; // Length of each character's encoding
};

/**
 * @struct huffman_tree
 * @brief Node structure for building and traversing Huffman trees
 *
 * Used during tree construction phase to build optimal Huffman codes:
 * - letter: The character this leaf node represents (unused for internal nodes)
 * - count: Frequency count for this character or combined count for internal nodes
 * - left/right: Pointers to child nodes (null for leaf nodes)
 *
 * The tree building algorithm combines nodes with lowest frequencies first,
 * creating a binary tree where frequent characters have shorter paths from root.
 */
struct huffman_tree {
    unsigned char letter; // Character value (0-255)
    unsigned int count; // Frequency count
    huffman_tree *left, *right; // Child node pointers
};

/*=============================================================================
 * GLOBAL VARIABLES
 *=============================================================================*/

/**
 * @brief Pointer to the root of the constructed Huffman tree
 *
 * Points to the top-level node after tree construction is complete.
 * Used as entry point for recursive dictionary generation.
 */
extern huffman_tree *head_huffman_tree_node;

/**
 * @brief Static array containing all Huffman tree nodes
 *
 * Size calculation: 256 possible characters + up to 255 internal nodes = 511 max
 * (Actually 512 for safe array bounds)
 *
 * Layout:
 * - Indices 0-255: Leaf nodes for each possible byte value
 * - Indices 256+: Internal nodes created during tree construction
 */
extern huffman_tree huffman_tree_node[512];

/**
 * @brief Host memory storage for long bit sequences (>191 bits)
 *
 * Stores the complete bit sequences for characters requiring more than 191 bits.
 * This data is copied to GPU constant memory for access during compression.
 * Only used when const_memory_flag is set to 1.
 */
extern unsigned char bit_sequence_const_memory[256][255];

/**
 * @brief Flag indicating whether constant memory is needed for long sequences
 *
 * Values:
 * - 0: All bit sequences fit in shared memory (≤191 bits)
 * - 1: Some sequences require constant memory (>191 bits)
 *
 * This flag determines which GPU kernel code path to use during compression.
 */
extern unsigned int const_memory_flag;

/**
 * @brief Global Huffman dictionary instance
 *
 * Contains the lookup table mapping each byte value to its compressed bit sequence.
 * Built during preprocessing and copied to GPU for parallel compression.
 */
extern huffman_dictionary huffman_dictionary;

/**
 * @brief GPU constant memory array for long bit sequences
 *
 * Mirror of bit_sequence_const_memory on the GPU device.
 * Constant memory provides cached, read-only access across all threads.
 * Automatically used by kernels when const_memory_flag == 1.
 */
extern __constant__ unsigned char d_bit_sequence_const_memory[256][255];

/*=============================================================================
 * HUFFMAN TREE CONSTRUCTION FUNCTIONS
 *=============================================================================*/

/**
 * @brief Sorts Huffman tree nodes by frequency using bubble sort
 * @param index Current iteration in tree building process
 * @param distinct_character_count Number of unique characters in input
 * @param combined_huffman_nodes Starting index of uncombined nodes
 *
 * Essential for Huffman algorithm correctness - ensures lowest frequency
 * nodes are always combined first to create optimal encoding tree.
 */
void sort_huffman_tree(int index, int distinct_character_count, int combined_huffman_nodes);

/**
 * @brief Combines two lowest-frequency nodes into new internal tree node
 * @param index Current iteration in tree building process
 * @param distinct_character_count Number of unique characters
 * @param combined_huffman_nodes Index of first uncombined node
 *
 * Implements core Huffman algorithm by creating binary tree structure
 * where path length from root determines bit sequence length.
 */
void build_huffman_tree(int index, int distinct_character_count, int combined_huffman_nodes);

/**
 * @brief Recursively generates bit sequences by traversing Huffman tree
 * @param root Current node in tree traversal
 * @param bit_sequence Array building current bit sequence path
 * @param bit_sequence_length Current depth/length of bit sequence
 *
 * Performs depth-first traversal assigning 0/1 bits for left/right paths.
 * Stores complete sequences in appropriate memory (shared vs constant).
 */
void build_huffman_dictionary(const huffman_tree *root, unsigned char *bit_sequence,
                              unsigned char bit_sequence_length);

/*=============================================================================
 * GPU COMPRESSION INTERFACE
 *=============================================================================*/

/**
 * @brief High-level wrapper function for GPU compression
 * @param file Pointer to filename string (for potential file I/O)
 * @param input_file_data Raw input data to compress
 * @param input_file_length Size of input data in bytes
 * @return Success/failure status code
 *
 * Main entry point that orchestrates the entire compression pipeline:
 * analysis, preprocessing, GPU execution, and result handling.
 */
int wrapper_gpu(char **file, unsigned char *input_file_data, int input_file_length);

/*=============================================================================
 * GPU KERNEL FUNCTION OVERLOADS
 *=============================================================================*/

/**
 * @brief GPU kernel for optimal compression scenario (single run, no overflow)
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman lookup table
 * @param d_byte_compressed_data Device output buffer for compressed bits
 * @param d_input_file_length Size of input data
 * @param const_memory_flag Whether to use constant memory for long sequences
 *
 * Handles small to medium files that fit entirely in GPU memory without
 * any integer overflow issues. This is the fastest compression path.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned int d_input_file_length, unsigned int const_memory_flag);

/**
 * @brief GPU kernel for single run with integer overflow handling
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman lookup table
 * @param d_byte_compressed_data Device buffer for pre-overflow compressed bits
 * @param d_temp_overflow Device buffer for post-overflow compressed bits
 * @param d_input_file_length Size of input data
 * @param const_memory_flag Constant memory usage flag
 * @param overflow_position Index where integer overflow occurs
 *
 * Handles files where cumulative bit offsets exceed unsigned int range.
 * Uses dual buffers to manage data before and after overflow point.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned char *d_temp_overflow, unsigned int d_input_file_length,
                         unsigned int const_memory_flag,
                         unsigned int overflow_position);

/**
 * @brief GPU kernel for multi-chunk compression without overflow
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman lookup table
 * @param d_byte_compressed_data Device output buffer
 * @param d_lower_position Starting index for this chunk (inclusive)
 * @param const_memory_flag Constant memory usage flag
 * @param d_upper_position Ending index for this chunk (exclusive)
 *
 * Processes a specific chunk of a large file in multi-kernel approach.
 * Each kernel call handles one sequential chunk of the total file.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned int d_lower_position, unsigned int const_memory_flag, unsigned int d_upper_position);

/**
 * @brief GPU kernel for most complex scenario (multi-chunk with overflow)
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman lookup table
 * @param d_byte_compressed_data Device buffer for regular compressed bits
 * @param d_temp_overflow Device buffer for overflow compressed bits
 * @param d_lower_position Starting index for this chunk
 * @param const_memory_flag Constant memory usage flag
 * @param d_upper_position Ending index for this chunk
 * @param overflow_position Index where integer overflow occurs within chunk
 *
 * Handles large files requiring chunking AND integer overflow within chunks.
 * Most complex kernel managing both memory limitations and overflow recovery.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned char *d_temp_overflow, unsigned int d_lower_position, unsigned int const_memory_flag,
                         unsigned int d_upper_position, unsigned int overflow_position);

/*=============================================================================
 * OFFSET ARRAY GENERATION FUNCTIONS
 *=============================================================================*/

/**
 * @brief Generates simple offset array for optimal compression case
 * @param compressed_data_offset Output array of cumulative bit offsets
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 *
 * Simplest case: small files with no overflow or chunking needed.
 * Pre-calculates where each compressed byte should be written.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              unsigned int input_file_length);

/**
 * @brief Generates offset array for multi-chunk compression
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param gpu_memory_overflow_index Output array marking chunk boundaries
 * @param gpu_bit_padding_flag Output flags for bit padding at boundaries
 * @param mem_req GPU memory limit for chunking decisions
 *
 * Handles large files by dividing into GPU memory-sized chunks.
 * Records chunk boundaries and padding requirements.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              unsigned int input_file_length, unsigned int *gpu_memory_overflow_index,
                              unsigned int *gpu_bit_padding_flag, long unsigned int mem_req);

/**
 * @brief Generates offset array with integer overflow detection
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param integer_overflow_index Output array marking overflow positions
 * @param bit_padding_flag Output flags for padding at overflow points
 * @param num_bytes Safety margin for overflow detection
 *
 * Detects when cumulative bit offsets would overflow unsigned int range.
 * Implements overflow recovery with proper bit boundary management.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              unsigned int input_file_length, unsigned int *integer_overflow_index,
                              unsigned int *bit_padding_flag, int num_bytes);

/**
 * @brief Most complex offset generation (multi-chunk + overflow)
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param integer_overflow_index Output array for overflow positions
 * @param bit_padding_flag Output flags for overflow padding
 * @param gpu_memory_overflow_index Output array for chunk boundaries
 * @param gpu_bit_padding_flag Output flags for chunk padding
 * @param num_bytes Safety margin for overflow detection
 * @param mem_req GPU memory limit
 *
 * Handles both memory-based chunking AND integer overflow within chunks.
 * Coordinates two different boundary management systems simultaneously.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              unsigned int input_file_length, unsigned int *integer_overflow_index,
                              unsigned int *bit_padding_flag, unsigned int *gpu_memory_overflow_index,
                              unsigned int *gpu_bit_padding_flag, int num_bytes, long unsigned int mem_req);

/*=============================================================================
 * MAIN GPU COMPRESSION ORCHESTRATION
 *=============================================================================*/

/**
 * @brief Main function orchestrating GPU Huffman compression pipeline
 * @param input_file_data Input/output data buffer (reused for compressed result)
 * @param compressed_data_offset Pre-calculated bit offset array
 * @param input_file_length Size of input data in bytes
 * @param num_kernel_runs Number of kernel launches required (1 or multiple)
 * @param integer_overflow_flag Whether integer overflow was detected (0 or 1)
 * @param mem_req GPU memory requirement for allocation decisions
 *
 * Central coordination function that:
 * 1. Analyzes compression scenario (size, overflow, chunking needs)
 * 2. Allocates appropriate GPU memory structures
 * 3. Routes to correct compression kernel based on scenario
 * 4. Manages data transfers and memory cleanup
 * 5. Handles all four compression complexity levels automatically
 *
 * This function abstracts away the complexity of scenario detection and
 * provides a clean interface for any file size or compression ratio.
 */
void launch_cuda_huffman_compress(unsigned char *input_file_data, unsigned int *compressed_data_offset,
                                  unsigned int input_file_length, int num_kernel_runs,
                                  unsigned int integer_overflow_flag,
                                  long unsigned int mem_req);
