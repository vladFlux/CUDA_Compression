#pragma once

/**
 * @file serial_utilities.h
 * @brief Header file for serial Huffman decompression system
 *
 * This header defines the data structures and function interfaces for CPU-based
 * Huffman decompression. The structures are specifically optimized for decompression
 * operations where tree traversal is the primary access pattern rather than
 * dictionary lookup.
 *
 * Key differences from parallel compression header:
 * - Simpler dictionary structure (no GPU memory optimizations needed)
 * - Single-threaded CPU function interfaces
 * - Focus on tree traversal rather than parallel bit processing
 * - No memory overflow handling (decompression works sequentially)
 */

/*=============================================================================
 * CORE DATA STRUCTURES FOR DECOMPRESSION
 *=============================================================================*/

/**
 * @struct huffman_dictionary
 * @brief Simplified dictionary structure for decompression verification
 *
 * This structure is much simpler than the parallel compression version because
 * decompression primarily uses tree traversal rather than dictionary lookup:
 *
 * - bit_sequence[255]: Complete bit sequence for each character (no split needed)
 * - bit_sequence_length: Length of the bit sequence
 *
 * Key differences from compression version:
 * - No 191-bit limitation (no GPU shared memory constraints)
 * - No hybrid memory management needed
 * - Used primarily for validation rather than primary decompression
 * - Single array per character instead of 2D array
 *
 * During decompression, this dictionary serves mainly as verification data.
 * The actual decompression process walks the tree directly for each bit sequence.
 */
struct huffman_dictionary {
    unsigned char bit_sequence[255];        // Complete bit sequence (up to 255 bits max)
    unsigned char bit_sequence_length;      // Length of this character's bit sequence
};

/**
 * @struct huffman_tree
 * @brief Binary tree node structure for decompression tree traversal
 *
 * Identical to the compression version but used differently during decompression:
 * - letter: Character value for leaf nodes (what gets output during decompression)
 * - count: Frequency count (used during tree reconstruction, then ignored)
 * - left/right: Child pointers for tree traversal during bit decoding
 *
 * Decompression usage pattern:
 * 1. Tree reconstruction: Uses count values to rebuild identical tree structure
 * 2. Bit decoding: Uses left/right pointers to traverse tree for each compressed bit
 * 3. Character output: Uses letter value when leaf nodes are reached
 *
 * Tree traversal algorithm:
 * - Read next bit from compressed data
 * - If bit == 0: go to left child
 * - If bit == 1: go to right child
 * - If leaf node reached: output character and return to root
 */
struct huffman_tree {
    unsigned char letter;                   // Character this leaf represents
    unsigned int count;                     // Frequency count (for tree building only)
    struct huffman_tree *left, *right;      // Child node pointers for traversal
};

/*=============================================================================
 * GLOBAL VARIABLES FOR DECOMPRESSION
 *=============================================================================*/

/**
 * @brief Array of dictionaries for all possible characters
 *
 * Unlike compression which uses a single dictionary structure with 2D arrays,
 * decompression uses an array of simple dictionary structures. This provides
 * cleaner access patterns for validation and debugging purposes.
 *
 * Index corresponds to character value (0-255), making lookups straightforward.
 * Primarily used for verification rather than core decompression operations.
 */
extern struct huffman_dictionary huffman_dictionary[256];

/**
 * @brief Pointer to the root of the reconstructed Huffman tree
 *
 * This is the entry point for all decompression tree traversals. Every
 * bit sequence decoding operation starts from this root node and follows
 * the tree paths based on the compressed bit values.
 *
 * Critical for decompression: This must point to the exact same tree structure
 * that was created during compression, or decompression will produce incorrect results.
 */
extern struct huffman_tree *head_huffman_tree_node;

/**
 * @brief Static array containing all Huffman tree nodes for decompression
 *
 * Same size and organization as compression version:
 * - Indices 0-255: Leaf nodes for each possible character value
 * - Indices 256-511: Internal nodes created during tree reconstruction
 *
 * During decompression:
 * 1. Leaf nodes (0-255) are populated with frequency data from compressed file
 * 2. Internal nodes (256+) are created by tree building algorithm
 * 3. Entire tree is used for bit-by-bit traversal during decompression
 */
extern struct huffman_tree huffman_tree_node[512];

/*=============================================================================
 * TREE CONSTRUCTION FUNCTIONS (SERIAL VERSIONS)
 *=============================================================================*/

/**
 * @brief Sorts tree nodes by frequency for deterministic tree reconstruction
 * @param index_param Current iteration in tree building
 * @param distinct_character_count Number of unique characters
 * @param combined_huffman_nodes Starting index of uncombined nodes
 *
 * CPU-only version of the sorting algorithm. Must produce identical results
 * to the compression-time sorting to ensure the same tree structure is rebuilt.
 * Uses simple bubble sort for deterministic, reproducible ordering.
 */
void sort_huffman_tree(int index_param, int distinct_character_count, int combined_huffman_nodes);

/**
 * @brief Combines lowest-frequency nodes into internal tree nodes
 * @param index Current tree building iteration
 * @param distinct_character_count Number of unique characters
 * @param combined_huffman_nodes Index of first uncombined node
 *
 * Creates the same binary tree structure that was used during compression.
 * Each internal node represents a decision point in the decompression process:
 * - Reaching this node means "choose left or right based on next bit"
 * - Left child corresponds to bit value 0
 * - Right child corresponds to bit value 1
 */
void build_huffman_tree(int index, int distinct_character_count, int combined_huffman_nodes);

/**
 * @brief Generates bit sequences for verification and debugging
 * @param root Current node in tree traversal
 * @param bit_sequence Array building the current bit path
 * @param bit_sequence_length Current depth in the tree
 *
 * Creates the character→bit mapping for validation purposes. While decompression
 * uses tree traversal (bit→character), this function generates the reverse
 * mapping to verify that tree reconstruction was successful.
 *
 * Validation use: Compare generated sequences with expected compression results
 * to ensure the decompression tree matches the compression tree exactly.
 */
void build_huffman_dictionary(const struct huffman_tree *root, unsigned char *bit_sequence,
                              unsigned char bit_sequence_length);

/*=============================================================================
 * DECOMPRESSION INTERFACE (LEGACY)
 *=============================================================================*/

/**
 * @brief Legacy wrapper function interface (maintained for compatibility)
 * @param file Pointer to filename string
 * @param input_file_data Buffer containing data to process
 * @param input_file_length Size of data buffer
 * @return Status code indicating success or failure
 *
 * Note: This appears to be a legacy interface that may have been copied from
 * the compression header. In a pure decompression context, this function might
 * not be needed or might serve a different purpose than GPU compression.
 *
 * Typical decompression workflow instead involves:
 * 1. Read compressed file (length + frequencies + compressed data)
 * 2. Reconstruct Huffman tree from frequency data
 * 3. Decompress bit stream using tree traversal
 * 4. Output reconstructed original data
 */
int wrapper_gpu(char **file, unsigned char *input_file_data, int input_file_length);
