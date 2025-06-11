cmake_minimum_required(VERSION 3.28)
project(compress LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_ARCHITECTURES 89)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(huffman_compression
        huffman_compression/compression/main_compress.cu
        huffman_compression/compression/GPU_wrapper.cu
        huffman_compression/compression/GPU_kernels.cu
        huffman_compression/compression/parallel_utilities.cu)

set_target_properties(huffman_compression PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

add_executable(huffman_decompress
        huffman_compression/decompression/main_decompress.c
        huffman_compression/decompression/serial_utilities.c)

set_target_properties(huffman_decompress PROPERTIES LINKER_LANGUAGE C)
