#include "parallel_utilities.h"


/**
 * @brief CUDA kernel for single-run compression without integer overflow
 * @param d_input_file_data Device array containing the raw input data to compress
 * @param d_compressed_data_offset Device array with pre-calculated bit offsets for each byte
 * @param d_huffman_dictionary Device copy of the Huffman encoding table
 * @param d_byte_compressed_data Device buffer for intermediate bit-level compressed data
 * @param d_input_file_length Length of input data in bytes
 * @param const_memory_flag Flag indicating whether to use constant memory for long bit sequences
 *
 * This kernel handles the optimal compression case:
 * - Small to medium files that fit entirely in GPU memory
 * - No integer overflow in bit offset calculations
 * - Single kernel launch processes entire file
 *
 * The compression process occurs in two phases:
 * 1. Bit-level encoding: Each input byte is replaced with its Huffman bit sequence
 * 2. Bit packing: Groups of 8 bits are packed into output bytes
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         const unsigned int d_input_file_length, const unsigned int const_memory_flag) {
    // Copy Huffman dictionary to shared memory for fast access across all threads in block
    // Shared memory provides much faster access than global memory for frequently used data
    __shared__ struct huffman_dictionary table;
    memcpy(&table, d_huffman_dictionary, sizeof(struct huffman_dictionary));

    const unsigned int input_file_length = d_input_file_length;
    unsigned int index, bit_index;

    // Calculate unique thread ID within the grid
    const unsigned int pos = blockIdx.x * blockDim.x + threadIdx.x;

    // Phase 1: Convert each input byte to its Huffman bit sequence
    // Two paths based on whether constant memory is needed for very long bit sequences
    if (const_memory_flag == 0) {
        // Standard path: All bit sequences fit in shared memory
        // Each thread processes every (blockDim.x)th element to ensure coalesced memory access
        for (index = pos; index < input_file_length; index += blockDim.x) {
            // For each input byte, copy its Huffman bit sequence to the compressed data buffer
            // d_compressed_data_offset[index] gives the bit position where this byte's encoding starts
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                    d_input_file_data[index]][bit_index];
            }
        }
    } else {
        // Hybrid path: Use both shared memory and constant memory
        // For very long bit sequences (>191 bits), use constant memory for the overflow
        for (index = pos; index < input_file_length; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                if (bit_index < 191) {
                    // Short sequences: use fast shared memory
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                        d_input_file_data[index]][
                        bit_index];
                } else {
                    // Long sequences: use constant memory for bits beyond 191
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data[index]][bit_index];
                }
            }
        }
    }

    // Synchronize all threads before proceeding to bit packing phase
    // Ensures all bit sequences are written before packing begins
    __syncthreads();

    // Phase 2: Pack individual bits into bytes
    // Each thread processes 8 bits (1 byte) at a time
    // pos * 8 ensures each thread starts at a different 8-bit boundary
    for (index = pos * 8; index < d_compressed_data_offset[input_file_length]; index += blockDim.x * 8) {
        // Process 8 consecutive bits and pack them into a single output byte
        for (unsigned int sub_index = 0; sub_index < 8; sub_index++) {
            if (d_byte_compressed_data[index + sub_index] == 0) {
                // Bit is 0: shift left and add 0 (just shift)
                d_input_file_data[index / 8] = d_input_file_data[index / 8] << 1;
            } else {
                // Bit is 1: shift left and set LSB to 1
                d_input_file_data[index / 8] = (d_input_file_data[index / 8] << 1) | 1;
            }
        }
    }
}

/**
 * @brief CUDA kernel for single-run compression with integer overflow handling
 * @param d_input_file_data Device input data array (reused for output)
 * @param d_compressed_data_offset Device array with bit offsets
 * @param d_huffman_dictionary Device Huffman encoding table
 * @param d_byte_compressed_data Device buffer for pre-overflow compressed bits
 * @param d_temp_overflow Device buffer for post-overflow compressed bits
 * @param d_input_file_length Length of input data
 * @param const_memory_flag Flag for constant memory usage
 * @param overflow_position Index where integer overflow occurs in offset array
 *
 * This kernel handles compression when bit offsets exceed unsigned int range.
 * The compression is split at the overflow point:
 * - Data before overflow goes to d_byte_compressed_data
 * - Data after overflow goes to d_temp_overflow
 * - Both segments are then packed separately and concatenated
 *
 * This scenario occurs with highly compressible data where the cumulative
 * bit offsets grow beyond what can be represented in 32-bit integers.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned char *d_temp_overflow, const unsigned int d_input_file_length,
                         const unsigned int const_memory_flag, const unsigned int overflow_position) {
    // Copy Huffman table to shared memory for fast access
    __shared__ struct huffman_dictionary table;
    memcpy(&table, d_huffman_dictionary, sizeof(struct huffman_dictionary));

    const unsigned int input_file_length = d_input_file_length;
    unsigned int index, sub_index, bit_index;
    const unsigned int pos = blockIdx.x * blockDim.x + threadIdx.x;

    // Phase 1: Bit-level encoding with overflow handling
    if (const_memory_flag == 0) {
        // Process data before overflow point
        // This data uses normal offset calculations
        for (index = pos; index < overflow_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                    d_input_file_data[index]][bit_index];
            }
        }

        // Process data after overflow point
        // Skip the overflow byte itself (handled separately) and process remaining data
        for (index = overflow_position + pos; index < input_file_length - 1; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index + 1]]; bit_index++) {
                d_temp_overflow[d_compressed_data_offset[index + 1] + bit_index] = table.bit_sequence[d_input_file_data[
                    index + 1]][bit_index];
            }
        }

        // Handle the overflow byte specially (only thread 0 does this to avoid race conditions)
        // Places the overflow byte's bit sequence at the boundary between buffers
        if (pos == 0) {
            memcpy(
                &d_temp_overflow[d_compressed_data_offset[(overflow_position + 1)] - table.bit_sequence_length[
                                     d_input_file_data[overflow_position]]],
                &table.bit_sequence[d_input_file_data[overflow_position]],
                table.bit_sequence_length[d_input_file_data[overflow_position]]);
        }
    } else {
        // Hybrid memory approach with overflow handling
        // Process pre-overflow data using shared/constant memory
        for (index = pos; index < overflow_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                if (bit_index < 191) {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                        d_input_file_data[index]][
                        bit_index];
                } else {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data[index]][bit_index];
                }
            }
        }

        // Process post-overflow data using shared/constant memory
        for (index = overflow_position + pos; index < input_file_length - 1; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index + 1]]; bit_index++) {
                if (bit_index < 191) {
                    d_temp_overflow[d_compressed_data_offset[index + 1] + bit_index] = table.bit_sequence[
                        d_input_file_data[index + 1]][
                        bit_index];
                } else {
                    d_temp_overflow[d_compressed_data_offset[index + 1] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data
                        [index + 1]][bit_index];
                }
            }
        }

        // Handle overflow byte using constant memory if sequence is long
        if (pos == 0) {
            memcpy(
                &d_temp_overflow[d_compressed_data_offset[(overflow_position + 1)] - table.bit_sequence_length[
                                     d_input_file_data[overflow_position]]],
                &d_bit_sequence_const_memory[d_input_file_data[overflow_position]],
                table.bit_sequence_length[d_input_file_data[overflow_position]]);
        }
    }

    // Ensure all bit sequences are written before packing
    __syncthreads();

    // Phase 2: Bit packing for pre-overflow data
    // Pack bits from d_byte_compressed_data into the beginning of output buffer
    for (index = pos * 8; index < d_compressed_data_offset[overflow_position]; index += blockDim.x * 8) {
        for (sub_index = 0; sub_index < 8; sub_index++) {
            if (d_byte_compressed_data[index + sub_index] == 0) {
                d_input_file_data[index / 8] = d_input_file_data[index / 8] << 1;
            } else {
                d_input_file_data[index / 8] = (d_input_file_data[index / 8] << 1) | 1;
            }
        }
    }

    // Calculate byte offset where overflow data should start in output
    const unsigned int offset_overflow = d_compressed_data_offset[overflow_position] / 8;

    // Phase 3: Bit packing for post-overflow data
    // Pack bits from d_temp_overflow into output buffer after the overflow offset
    for (index = pos * 8; index < d_compressed_data_offset[input_file_length]; index += blockDim.x * 8) {
        for (sub_index = 0; sub_index < 8; sub_index++) {
            if (d_temp_overflow[index + sub_index] == 0) {
                d_input_file_data[(index / 8) + offset_overflow] =
                        d_input_file_data[(index / 8) + offset_overflow] << 1;
            } else {
                d_input_file_data[(index / 8) + offset_overflow] =
                        (d_input_file_data[(index / 8) + offset_overflow] << 1) | 1;
            }
        }
    }
}

/**
 * @brief CUDA kernel for multi-run compression without integer overflow
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman table
 * @param d_byte_compressed_data Device compressed bit buffer
 * @param d_lower_position Starting index for this chunk (inclusive)
 * @param const_memory_flag Constant memory usage flag
 * @param d_upper_position Ending index for this chunk (exclusive)
 *
 * This kernel processes a specific chunk of a large file in a multi-kernel approach.
 * Large files are divided into chunks to fit within GPU memory constraints.
 * Each kernel call processes one chunk sequentially.
 *
 * Special handling is needed for chunk boundaries to ensure bit sequences
 * that span chunk boundaries are properly encoded.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         const unsigned int d_lower_position, const unsigned int const_memory_flag,
                         const unsigned int d_upper_position) {
    // Load Huffman table into shared memory
    __shared__ struct huffman_dictionary table;
    memcpy(&table, d_huffman_dictionary, sizeof(struct huffman_dictionary));

    unsigned int index, bit_index;
    const unsigned int pos = blockIdx.x * blockDim.x + threadIdx.x;

    // Phase 1: Bit-level encoding for this chunk
    if (const_memory_flag == 0) {
        // Process bytes within the specified chunk range [d_lower_position, d_upper_position)
        for (index = pos + d_lower_position; index < d_upper_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                    d_input_file_data[index]][bit_index];
            }
        }

        // Handle chunk boundary condition
        // If this isn't the first chunk, need to encode the last byte of previous chunk
        // This ensures proper bit sequence continuity across chunk boundaries
        if (pos == 0 && d_lower_position != 0) {
            memcpy(
                &d_byte_compressed_data[d_compressed_data_offset[(d_lower_position)] - table.bit_sequence_length[
                                            d_input_file_data[d_lower_position - 1]]],
                &table.bit_sequence[d_input_file_data[d_lower_position - 1]],
                table.bit_sequence_length[d_input_file_data[d_lower_position - 1]]);
        }
    } else {
        // Hybrid memory approach for chunk processing
        for (index = pos + d_lower_position; index < d_upper_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                if (bit_index < 191) {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                        d_input_file_data[index]][
                        bit_index];
                } else {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data[index]][bit_index];
                }
            }
        }

        // Handle chunk boundary with constant memory
        if (pos == 0 && d_lower_position != 0) {
            memcpy(
                &d_byte_compressed_data[d_compressed_data_offset[(d_lower_position)] - table.bit_sequence_length[
                                            d_input_file_data[d_lower_position - 1]]],
                &d_bit_sequence_const_memory[d_input_file_data[d_lower_position - 1]],
                table.bit_sequence_length[d_input_file_data[d_lower_position - 1]]);
        }
    }

    // Synchronize before bit packing
    __syncthreads();

    // Phase 2: Bit packing for this chunk
    // Pack all bits generated by this chunk into bytes
    for (index = pos * 8; index < d_compressed_data_offset[d_upper_position]; index += blockDim.x * 8) {
        for (unsigned int sub_index = 0; sub_index < 8; sub_index++) {
            if (d_byte_compressed_data[index + sub_index] == 0) {
                d_input_file_data[(index / 8)] = d_input_file_data[(index / 8)] << 1;
            } else {
                d_input_file_data[(index / 8)] = (d_input_file_data[index / 8] << 1) | 1;
            }
        }
    }
}

/**
 * @brief CUDA kernel for multi-run compression with integer overflow handling
 * @param d_input_file_data Device input data array
 * @param d_compressed_data_offset Device bit offset array
 * @param d_huffman_dictionary Device Huffman table
 * @param d_byte_compressed_data Device buffer for pre-overflow bits
 * @param d_temp_overflow Device buffer for post-overflow bits
 * @param d_lower_position Start of chunk range
 * @param const_memory_flag Constant memory flag
 * @param d_upper_position End of chunk range
 * @param overflow_position Index where integer overflow occurs in this chunk
 *
 * This is the most complex compression scenario, combining:
 * - Multi-chunk processing for large files
 * - Integer overflow handling within chunks
 * - Chunk boundary management
 * - Dual buffer management for overflow data
 *
 * When a chunk itself experiences integer overflow, it must be split
 * into pre-overflow and post-overflow segments, each using different buffers.
 */
__global__ void compress(unsigned char *d_input_file_data, const unsigned int *d_compressed_data_offset,
                         const struct huffman_dictionary *d_huffman_dictionary, unsigned char *d_byte_compressed_data,
                         unsigned char *d_temp_overflow, const unsigned int d_lower_position,
                         const unsigned int const_memory_flag,
                         const unsigned int d_upper_position, const unsigned int overflow_position) {
    // Load Huffman table to shared memory
    __shared__ struct huffman_dictionary table;
    memcpy(&table, d_huffman_dictionary, sizeof(struct huffman_dictionary));

    unsigned int index, sub_index, bit_index;
    const unsigned int pos = blockIdx.x * blockDim.x + threadIdx.x;

    // Phase 1: Complex bit-level encoding with both chunk and overflow boundaries
    if (const_memory_flag == 0) {
        // Process chunk data before overflow point
        for (index = pos + d_lower_position; index < overflow_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                    d_input_file_data[index]][bit_index];
            }
        }

        // Process chunk data after overflow point
        for (index = overflow_position + pos; index < d_upper_position - 1; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index + 1]]; bit_index++) {
                d_temp_overflow[d_compressed_data_offset[index + 1] + bit_index] = table.bit_sequence[d_input_file_data[
                    index + 1]][bit_index];
            }
        }

        // Handle the overflow byte (thread 0 only)
        if (pos == 0) {
            memcpy(
                &d_temp_overflow[d_compressed_data_offset[(overflow_position + 1)] - table.bit_sequence_length[
                                     d_input_file_data[overflow_position]]],
                &table.bit_sequence[d_input_file_data[overflow_position]],
                table.bit_sequence_length[d_input_file_data[overflow_position]]);
        }

        // Handle chunk boundary (if not first chunk)
        if (pos == 0 && d_lower_position != 0) {
            memcpy(
                &d_byte_compressed_data[d_compressed_data_offset[(d_lower_position)] - table.bit_sequence_length[
                                            d_input_file_data[d_lower_position - 1]]],
                &table.bit_sequence[d_input_file_data[d_lower_position - 1]],
                table.bit_sequence_length[d_input_file_data[d_lower_position - 1]]);
        }
    } else {
        // Hybrid memory approach with complex boundary handling
        // Process entire chunk range, but handle overflow internally
        for (index = pos + d_lower_position; index < d_upper_position; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index]]; bit_index++) {
                if (bit_index < 191) {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                        d_input_file_data[index]][
                        bit_index];
                } else {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data[index]][bit_index];
                }
            }
        }

        // Process post-overflow data separately
        for (index = overflow_position + pos; index < d_upper_position - 1; index += blockDim.x) {
            for (bit_index = 0; bit_index < table.bit_sequence_length[d_input_file_data[index + 1]]; bit_index++) {
                if (bit_index < 191) {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = table.bit_sequence[
                        d_input_file_data[index]][
                        bit_index];
                } else {
                    d_byte_compressed_data[d_compressed_data_offset[index] + bit_index] = d_bit_sequence_const_memory[
                        d_input_file_data[index]][bit_index];
                }
            }
        }

        // Handle overflow byte with constant memory
        if (pos == 0) {
            memcpy(
                &d_temp_overflow[d_compressed_data_offset[(overflow_position + 1)] - table.bit_sequence_length[
                                     d_input_file_data[overflow_position]]],
                &d_bit_sequence_const_memory[d_input_file_data[overflow_position]],
                table.bit_sequence_length[d_input_file_data[overflow_position]]);
        }

        // Handle chunk boundary with constant memory
        if (pos == 0 && d_lower_position != 0) {
            memcpy(
                &d_byte_compressed_data[d_compressed_data_offset[(d_lower_position)] - table.bit_sequence_length[
                                            d_input_file_data[d_lower_position - 1]]],
                &d_bit_sequence_const_memory[d_input_file_data[d_lower_position - 1]],
                table.bit_sequence_length[d_input_file_data[d_lower_position - 1]]);
        }
    }

    // Synchronize before bit packing phase
    __syncthreads();

    // Phase 2: Bit packing for pre-overflow segment
    for (index = pos * 8; index < d_compressed_data_offset[overflow_position]; index += blockDim.x * 8) {
        for (sub_index = 0; sub_index < 8; sub_index++) {
            if (d_byte_compressed_data[index + sub_index] == 0) {
                d_input_file_data[(index / 8)] = d_input_file_data[(index / 8)] << 1;
            } else {
                d_input_file_data[(index / 8)] = (d_input_file_data[index / 8] << 1) | 1;
            }
        }
    }

    // Calculate overflow offset for this chunk
    const unsigned int offset_overflow = d_compressed_data_offset[overflow_position] / 8;

    // Phase 3: Bit packing for post-overflow segment
    for (index = pos * 8; index < d_compressed_data_offset[d_upper_position]; index += blockDim.x * 8) {
        for (sub_index = 0; sub_index < 8; sub_index++) {
            if (d_temp_overflow[index + sub_index] == 0) {
                d_input_file_data[(index / 8) + offset_overflow] =
                        d_input_file_data[(index / 8) + offset_overflow] << 1;
            } else {
                d_input_file_data[(index / 8) + offset_overflow] =
                        (d_input_file_data[(index / 8) + offset_overflow] << 1) | 1;
            }
        }
    }
}
