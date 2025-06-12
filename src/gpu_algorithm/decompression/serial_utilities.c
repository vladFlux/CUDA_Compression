#include <stdio.h>
#include <string.h>
#include "serial_utilities.h"


/**
 * @file serial_utilities.c
 * @brief Serial implementation of Huffman tree construction for decompression
 *
 * This file contains CPU-only implementations of the core Huffman algorithm
 * functions used during decompression. These functions reconstruct the same
 * Huffman tree that was used during compression, allowing for accurate
 * decompression of the compressed data.
 *
 * The functions mirror the parallel versions but are optimized for single-threaded
 * CPU execution during the decompression phase.
 */

/**
 * @brief Sorts Huffman tree nodes by frequency using insertion sort (serial version)
 * @param index_param Current iteration in the tree building process
 * @param distinct_character_count Number of unique characters found in frequency table
 * @param combined_huffman_nodes Starting index of nodes that haven't been combined yet
 *
 * This is the CPU-only version of the sorting function used during decompression.
 * It reconstructs the same sorting order that was used during compression to ensure
 * the decompression tree matches the compression tree exactly.
 *
 * The insertion sort algorithm ensures deterministic ordering - crucial for decompression
 * since the exact same tree structure must be recreated to properly decode the
 * compressed bit sequences back to their original characters.
 *
 * Unlike the parallel version, this runs on a single CPU thread since decompression
 * tree construction is typically much faster than the original compression process.
 */
void sort_huffman_tree(const int index_param, const int distinct_character_count, const int combined_huffman_nodes) {
    // Define the range of nodes that need to be sorted
    const int start = combined_huffman_nodes;
    const int end = distinct_character_count - 1 + index_param;

    // Insertion sort: iterate through unsorted portion starting from second element
    for (int index = start + 1; index <= end; index++) {
        // Store the current element to be inserted into sorted portion
        const struct huffman_tree temp = huffman_tree_node[index];
        int sub_index = index - 1;

        // Shift elements in sorted portion that are greater than temp to the right
        // This creates space for inserting temp in its correct position
        while (sub_index >= start && huffman_tree_node[sub_index].count > temp.count) {
            huffman_tree_node[sub_index + 1] = huffman_tree_node[sub_index];
            sub_index--;
        }

        // Insert temp into its correct position in the sorted portion
        huffman_tree_node[sub_index + 1] = temp;
    }
}

/**
 * @brief Creates internal tree nodes by combining lowest-frequency nodes (serial version)
 * @param index Current iteration in tree construction
 * @param distinct_character_count Number of unique characters in the data
 * @param combined_huffman_nodes Index of the first uncombined node
 *
 * This function recreates the exact same tree structure that was built during
 * compression. By using the same frequency data (stored in the compressed file)
 * and the same algorithm, it reconstructs the identical binary tree.
 *
 * The tree structure determines the bit-to-character mapping needed for decompression:
 * - Each left traversal corresponds to a '0' bit in the compressed data
 * - Each right traversal corresponds to a '1' bit in the compressed data
 * - Leaf nodes contain the actual characters to be output
 *
 * This deterministic reconstruction is essential - any difference in tree structure
 * would result in incorrect decompression and corrupted output data.
 */
void build_huffman_tree(const int index, const int distinct_character_count, const int combined_huffman_nodes) {
    // Create new internal node with combined frequency of two lowest-frequency nodes
    // This mirrors the exact same combining logic used during compression
    huffman_tree_node[distinct_character_count + index].count =
            huffman_tree_node[combined_huffman_nodes].count + huffman_tree_node[combined_huffman_nodes + 1].count;

    // Set left child pointer to the lowest-frequency node
    huffman_tree_node[distinct_character_count + index].left = &huffman_tree_node[combined_huffman_nodes];

    // Set right child pointer to the second lowest-frequency node
    huffman_tree_node[distinct_character_count + index].right = &huffman_tree_node[combined_huffman_nodes + 1];

    // Update tree head to point to this newly created internal node
    // After all iterations complete, this will point to the root of the complete tree
    head_huffman_tree_node = &(huffman_tree_node[distinct_character_count + index]);
}

/**
 * @brief Recursively builds the character-to-bit-sequence lookup table
 * @param root Current node being processed in tree traversal
 * @param bit_sequence Array accumulating the current bit sequence path
 * @param bit_sequence_length Current depth/length of the bit sequence
 *
 * This function performs depth-first traversal of the reconstructed Huffman tree
 * to generate the same bit sequences that were used during compression. However,
 * for decompression, we typically need the reverse mapping (bit sequence to character)
 * rather than character to bit sequence.
 *
 * The generated dictionary serves as verification that the tree reconstruction
 * was successful and can also be used for validation purposes. The primary
 * decompression process uses direct tree traversal rather than dictionary lookup
 * for better performance when processing long bit sequences.
 *
 * Key differences from compression usage:
 * - Compression: uses dictionary for fast character → bits lookup
 * - Decompression: uses tree traversal for sequential bits → character conversion
 *
 * This function ensures the reconstructed tree produces identical bit sequences,
 * validating that decompression will work correctly.
 */
void build_huffman_dictionary(const struct huffman_tree *root, unsigned char *bit_sequence,
                              const unsigned char bit_sequence_length) {
    // Traverse left subtree (append '0' bit to current sequence)
    if (root->left) {
        bit_sequence[bit_sequence_length] = 0;
        build_huffman_dictionary(root->left, bit_sequence, bit_sequence_length + 1);
    }

    // Traverse right subtree (append '1' bit to current sequence)
    if (root->right) {
        bit_sequence[bit_sequence_length] = 1;
        build_huffman_dictionary(root->right, bit_sequence, bit_sequence_length + 1);
    }

    // Leaf node reached - store the complete bit sequence for this character
    // This creates the character → bit sequence mapping for verification
    if (root->left == NULL && root->right == NULL) {
        // Store the length of this character's bit sequence
        huffman_dictionary[root->letter].bit_sequence_length = bit_sequence_length;

        // Copy the complete bit sequence for this character
        // During decompression, this serves as validation data rather than primary lookup
        memcpy(huffman_dictionary[root->letter].bit_sequence, bit_sequence,
               bit_sequence_length * sizeof(unsigned char));
    }
}
