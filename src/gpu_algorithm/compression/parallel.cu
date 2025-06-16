#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "parallel.h"


/**
 * @brief Sorts Huffman tree nodes by frequency using insertion sort
 * @param index_param Current iteration index in the tree building process
 * @param distinct_character_count Number of unique characters in input data
 * @param combined_huffman_nodes Starting index for nodes that haven't been combined yet
 *
 * This function implements insertion sort to arrange Huffman tree nodes in ascending order
 * by their frequency counts. The sorting is essential for the Huffman algorithm to work
 * correctly - we always want to combine the two nodes with the lowest frequencies.
 *
 * The sorting range is dynamic and shrinks with each iteration as nodes get combined
 * into the tree structure. Only uncombined nodes (from combined_huffman_nodes onward)
 * need to be sorted in each iteration.
 */
void sort_huffman_tree(const int index_param, const int distinct_character_count, const int combined_huffman_nodes) {
    // Define the range of nodes that need to be sorted
    const int start = combined_huffman_nodes;
    const int end = distinct_character_count - 1 + index_param;

    // Insertion sort: iterate through unsorted portion starting from second element
    for (int index = start + 1; index <= end; index++) {
        // Store the current element to be inserted into sorted portion
        const huffman_tree temp = huffman_tree_node[index];
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
 * @brief Creates internal Huffman tree nodes by combining the two lowest-frequency nodes
 * @param index Current iteration in the tree building process
 * @param distinct_character_count Number of unique characters (leaf nodes)
 * @param combined_huffman_nodes Index of the first uncombined node
 *
 * This function implements the core of the Huffman algorithm by:
 * 1. Taking the two nodes with the lowest frequencies (after sorting)
 * 2. Creating a new internal node with their combined frequency
 * 3. Setting the new node's left and right children to point to these nodes
 * 4. Updating the tree head pointer to the newly created node
 *
 * The tree is built bottom-up, with leaf nodes representing individual characters
 * and internal nodes representing combined frequency groups. The final tree structure
 * determines the Huffman codes - more frequent characters get shorter paths from root.
 */
void build_huffman_tree(const int index, const int distinct_character_count, const int combined_huffman_nodes) {
    // Create new internal node by combining the two lowest-frequency nodes
    // The combined frequency is the sum of the two child frequencies
    huffman_tree_node[distinct_character_count + index].count =
            huffman_tree_node[combined_huffman_nodes].count + huffman_tree_node[combined_huffman_nodes + 1].count;

    // Set left child to point to the first (lowest frequency) node
    huffman_tree_node[distinct_character_count + index].left = &huffman_tree_node[combined_huffman_nodes];

    // Set right child to point to the second (second lowest frequency) node
    huffman_tree_node[distinct_character_count + index].right = &huffman_tree_node[combined_huffman_nodes + 1];

    // Update the tree head to point to this new internal node
    // The head always points to the most recently created internal node
    // After all iterations, head will point to the root of the complete tree
    head_huffman_tree_node = &(huffman_tree_node[distinct_character_count + index]);
}

/**
 * @brief Recursively traverses the Huffman tree to generate bit sequences for each character
 * @param root Current node being processed in the tree traversal
 * @param bit_sequence Array building the current bit sequence path
 * @param bit_sequence_length Current length of the bit sequence being built
 *
 * This function performs a depth-first traversal of the Huffman tree to generate
 * the binary codes for each character. The algorithm works as follows:
 *
 * - Left traversal adds a '0' bit to the current sequence
 * - Right traversal adds a '1' bit to the current sequence
 * - When a leaf node (character) is reached, the complete bit sequence is stored
 *
 * The function handles two memory storage strategies:
 * - Short sequences (<192 bits): stored in regular shared memory dictionary
 * - Long sequences (≥192 bits): split between shared memory and constant memory
 *
 * This dual storage approach optimizes GPU memory access patterns for different
 * sequence lengths, balancing memory bandwidth and cache efficiency.
 */
void build_huffman_dictionary(const huffman_tree *root, unsigned char *bit_sequence,
                              const unsigned char bit_sequence_length) {
    // Traverse left subtree (add '0' to bit sequence)
    if (root->left) {
        bit_sequence[bit_sequence_length] = 0;
        build_huffman_dictionary(root->left, bit_sequence, bit_sequence_length + 1);
    }

    // Traverse right subtree (add '1' to bit sequence)
    if (root->right) {
        bit_sequence[bit_sequence_length] = 1;
        build_huffman_dictionary(root->right, bit_sequence, bit_sequence_length + 1);
    }

    // Leaf node reached - store the complete bit sequence for this character
    if (root->left == nullptr && root->right == nullptr) {
        // Store the length of this character's bit sequence
        huffman_dictionary.bit_sequence_length[root->letter] = bit_sequence_length;

        if (bit_sequence_length < 192) {
            // Short sequence: store entirely in shared memory dictionary
            // This provides fastest access during GPU compression
            memcpy(huffman_dictionary.bit_sequence[root->letter], bit_sequence,
                   bit_sequence_length * sizeof(unsigned char));
        } else {
            // Long sequence: hybrid storage strategy
            // Store complete sequence in constant memory for full access
            memcpy(bit_sequence_const_memory[root->letter], bit_sequence, bit_sequence_length * sizeof(unsigned char));

            // Store first 191 bits in shared memory dictionary for fast initial access
            memcpy(huffman_dictionary.bit_sequence[root->letter], bit_sequence, 191);

            // Set global flag indicating constant memory is needed
            // This informs the GPU kernels to use hybrid memory access
            const_memory_flag = 1;
        }
    }
}

/**
 * @brief Generates bit offset array for simple single-kernel compression
 * @param compressed_data_offset Output array storing cumulative bit offsets
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data in bytes
 *
 * This is the simplest offset calculation for optimal compression scenarios:
 * - Small to medium files that fit in GPU memory
 * - No integer overflow in cumulative bit calculations
 * - Single kernel launch will process entire file
 *
 * The offset array is crucial for parallel compression - it tells each GPU thread
 * exactly where to write the compressed bits for each input byte. Without
 * pre-calculated offsets, threads would need to synchronize constantly.
 *
 * The final offset is padded to byte boundary to ensure proper bit packing.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              const unsigned int input_file_length) {
    // Initialize first offset to 0 (compression starts at bit 0)
    compressed_data_offset[0] = 0;

    // Calculate cumulative bit offsets for each input byte
    // Each byte's offset = previous offset + bit length of current byte's Huffman code
    for (int index = 0; index < input_file_length; index++) {
        compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]] +
                                            compressed_data_offset[index];
    }

    // Pad final offset to byte boundary if necessary
    // This ensures the compressed data aligns properly for bit packing
    // Example: if final bit offset is 13, pad to 16 (next multiple of 8)
    if (compressed_data_offset[input_file_length] % 8 != 0) {
        compressed_data_offset[input_file_length] = compressed_data_offset[input_file_length] + (
                                                        8 - (compressed_data_offset[input_file_length] % 8));
    }
}

/**
 * @brief Generates offset array with integer overflow detection and handling
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param integer_overflow_index Output array marking overflow positions
 * @param bit_padding_flag Output flags indicating bit padding needs at overflow points
 * @param num_bytes Safety margin to detect impending overflow
 *
 * This function handles compression scenarios where the cumulative bit offsets
 * exceed the range of unsigned integers (4.3 billion bits ≈ 537MB compressed).
 * This occurs with highly compressible data or very large files.
 *
 * When overflow is detected:
 * 1. The overflow position is recorded
 * 2. Bit alignment is checked and padding applied if needed
 * 3. Offset calculation restarts from 0 for post-overflow data
 *
 * The num_bytes parameter provides a safety margin (typically 8192) to detect
 * overflow before it occurs, preventing integer wraparound errors.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              const unsigned int input_file_length, unsigned int *integer_overflow_index,
                              unsigned int *bit_padding_flag, const int num_bytes) {
    // Index for tracking multiple overflow points
    int sub_index = 0;
    compressed_data_offset[0] = 0;

    for (int index = 0; index < input_file_length; index++) {
        // Calculate next cumulative offset
        compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]] +
                                            compressed_data_offset[index];

        // Check for integer overflow (addition wraparound detection)
        // If next_offset + safety_margin < current_offset, overflow occurred
        if (compressed_data_offset[index + 1] + num_bytes < compressed_data_offset[index]) {
            // Record the position where overflow occurred
            integer_overflow_index[sub_index] = index;

            // Check if current position requires bit padding
            if (compressed_data_offset[index] % 8 != 0) {
                // Not on byte boundary - padding required
                bit_padding_flag[sub_index] = 1;

                // Calculate new offset with bit alignment consideration
                // Keep the remainder bits and add current byte's bit length
                compressed_data_offset[index + 1] =
                        (compressed_data_offset[index] % 8) + huffman_dictionary.bit_sequence_length
                        [input_file_data[index]];

                // Pad current offset to byte boundary
                compressed_data_offset[index] = compressed_data_offset[index] + (
                                                    8 - (compressed_data_offset[index] % 8));
            } else {
                // On byte boundary - no padding needed
                // Reset offset calculation starting from current byte's length
                compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]];
            }
            sub_index++;
        }
    }

    // Apply final byte boundary padding
    if (compressed_data_offset[input_file_length] % 8 != 0) {
        compressed_data_offset[input_file_length] = compressed_data_offset[input_file_length] + (
                                                        8 - (compressed_data_offset[input_file_length] % 8));
    }
}

/**
 * @brief Generates offset array for multi-kernel compression without integer overflow
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param gpu_memory_overflow_index Output array marking memory chunk boundaries
 * @param gpu_bit_padding_flag Output flags for bit padding at chunk boundaries
 * @param mem_req GPU memory limit for chunking decisions
 *
 * This function handles large files that must be split across multiple kernel
 * launches due to GPU memory constraints. The file is divided into chunks
 * that fit within available GPU memory.
 *
 * Key differences from single-run:
 * 1. Monitors memory usage instead of integer overflow
 * 2. Records chunk boundaries in gpu_memory_overflow_index
 * 3. Handles bit padding between chunks to maintain compression integrity
 * 4. Each chunk can be processed independently by separate kernel launches
 *
 * The chunking strategy ensures optimal GPU memory utilization while
 * maintaining compression efficiency across chunk boundaries.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              const unsigned int input_file_length, unsigned int *gpu_memory_overflow_index,
                              unsigned int *gpu_bit_padding_flag, const long unsigned int mem_req) {
    int sub_index = 0;

    // Initialize chunk tracking arrays
    gpu_memory_overflow_index[0] = 0; // First chunk starts at index 0
    gpu_bit_padding_flag[0] = 0; // First chunk doesn't need padding
    compressed_data_offset[0] = 0;

    for (int index = 0; index < input_file_length; index++) {
        // Calculate cumulative bit offset
        compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]] +
                                            compressed_data_offset[index];

        // Check if current offset exceeds GPU memory limit
        if (compressed_data_offset[index + 1] > mem_req) {
            // Record chunk boundary: current chunk ends at position index
            gpu_memory_overflow_index[sub_index * 2 + 1] = index;
            // Next chunk starts at position index + 1
            gpu_memory_overflow_index[sub_index * 2 + 2] = index + 1;

            // Check bit alignment at chunk boundary
            if (compressed_data_offset[index] % 8 != 0) {
                // Chunk doesn't end on byte boundary - padding needed for next chunk
                gpu_bit_padding_flag[sub_index + 1] = 1;

                // Calculate offset for next chunk considering bit remainder
                compressed_data_offset[index + 1] =
                        (compressed_data_offset[index] % 8) + huffman_dictionary.bit_sequence_length
                        [input_file_data[index]];

                // Pad current chunk to byte boundary
                compressed_data_offset[index] = compressed_data_offset[index] + (
                                                    8 - (compressed_data_offset[index] % 8));
            } else {
                // Chunk ends on byte boundary - clean break
                compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]];
            }
            sub_index++;
        }
    }

    // Apply final padding and record final chunk boundary
    if (compressed_data_offset[input_file_length] % 8 != 0) {
        compressed_data_offset[input_file_length] = compressed_data_offset[input_file_length] + (
                                                        8 - (compressed_data_offset[input_file_length] % 8));
    }
    gpu_memory_overflow_index[sub_index * 2 + 1] = input_file_length;
}

/**
 * @brief Generates offset array for the most complex scenario: multi-kernel with integer overflow
 * @param compressed_data_offset Output bit offset array
 * @param input_file_data Input data to analyze
 * @param input_file_length Size of input data
 * @param integer_overflow_index Output array for integer overflow positions
 * @param bit_padding_flag Output flags for overflow padding
 * @param gpu_memory_overflow_index Output array for memory chunk boundaries
 * @param gpu_bit_padding_flag Output flags for chunk padding
 * @param num_bytes Safety margin for overflow detection
 * @param mem_req GPU memory limit
 *
 * This is the most complex offset calculation, handling both:
 * 1. Memory-based chunking for large files
 * 2. Integer overflow within chunks for highly compressible data
 *
 * The function must coordinate two different types of boundaries:
 * - Memory boundaries: where chunks are split due to GPU memory limits
 * - Overflow boundaries: where integer arithmetic overflows within chunks
 *
 * Special logic handles the interaction between these two boundary types,
 * ensuring that both memory management and overflow recovery work correctly
 * when they occur in the same compression job.
 *
 * This scenario typically occurs with very large, highly compressible files
 * that require both multi-kernel processing and overflow handling.
 */
void create_data_offset_array(unsigned int *compressed_data_offset, const unsigned char *input_file_data,
                              const unsigned int input_file_length, unsigned int *integer_overflow_index,
                              unsigned int *bit_padding_flag, unsigned int *gpu_memory_overflow_index,
                              unsigned int *gpu_bit_padding_flag, const int num_bytes,
                              const long unsigned int mem_req) {
    int sub_index = 0; // Counter for integer overflow events
    int overflow_index = 0; // Counter for memory overflow (chunk) events
    compressed_data_offset[0] = 0;

    for (int index = 0; index < input_file_length; index++) {
        // Calculate next cumulative offset
        compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]] +
                                            compressed_data_offset[index];

        // Check for memory limit exceeded (but only if we've had integer overflow before)
        // This complex condition handles the interaction between memory chunking and integer overflow
        if (sub_index != 0 && (static_cast<long unsigned int>(compressed_data_offset[index + 1]) + compressed_data_offset[
                           integer_overflow_index[sub_index - 1]] > mem_req)) {
            // Memory limit exceeded - create new chunk boundary
            gpu_memory_overflow_index[overflow_index * 2 + 1] = index;
            gpu_memory_overflow_index[overflow_index * 2 + 2] = index + 1;

            // Handle bit padding for chunk boundary
            if (compressed_data_offset[index] % 8 != 0) {
                gpu_bit_padding_flag[overflow_index + 1] = 1;
                compressed_data_offset[index + 1] =
                        (compressed_data_offset[index] % 8) + huffman_dictionary.bit_sequence_length
                        [input_file_data[index]];
                compressed_data_offset[index] = compressed_data_offset[index] + (
                                                    8 - (compressed_data_offset[index] % 8));
            } else {
                compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]];
            }
            overflow_index++;
        } else if (compressed_data_offset[index + 1] + num_bytes < compressed_data_offset[index]) {
            // Integer overflow detected - handle overflow boundary
            integer_overflow_index[sub_index] = index;

            // Handle bit padding for overflow boundary
            if (compressed_data_offset[index] % 8 != 0) {
                bit_padding_flag[sub_index] = 1;
                compressed_data_offset[index + 1] =
                        (compressed_data_offset[index] % 8) + huffman_dictionary.bit_sequence_length
                        [input_file_data[index]];
                compressed_data_offset[index] = compressed_data_offset[index] + (
                                                    8 - (compressed_data_offset[index] % 8));
            } else {
                compressed_data_offset[index + 1] = huffman_dictionary.bit_sequence_length[input_file_data[index]];
            }
            sub_index++;
        }
    }

    // Apply final padding and record final boundaries
    if (compressed_data_offset[input_file_length] % 8 != 0) {
        compressed_data_offset[input_file_length] = compressed_data_offset[input_file_length] + (
                                                        8 - (compressed_data_offset[input_file_length] % 8));
    }
    gpu_memory_overflow_index[sub_index * 2 + 1] = input_file_length;
}
