cmake_minimum_required(VERSION 3.28)
project(compress LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_ARCHITECTURES 89)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

add_executable(huffman_compression
        src/compression/main_compress.cu
        src/compression/GPU_wrapper.cu
        src/compression/GPU_kernels.cu
        src/compression/parallel_utilities.cu)

set_target_properties(huffman_compression PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

add_executable(huffman_decompress
        src/decompression/main_decompress.c
        src/decompression/serial_utilities.c)

set_target_properties(huffman_decompress PROPERTIES LINKER_LANGUAGE C)
