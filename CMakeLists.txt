cmake_minimum_required(VERSION 3.28)
project(compress LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_ARCHITECTURES "native")
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# GPU binaries
add_executable(huffman_compression
        src/gpu_algorithm/compression/compress.cu
        src/gpu_algorithm/compression/gpu_wrapper.cu
        src/gpu_algorithm/compression/kernels.cu
        src/gpu_algorithm/compression/parallel.cu)

set_target_properties(huffman_compression PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

add_executable(huffman_decompression
        src/gpu_algorithm/decompression/main_decompress.c
        src/gpu_algorithm/decompression/serial_utilities.c)

set_target_properties(huffman_decompression PROPERTIES LINKER_LANGUAGE C)


# CPU binaries
add_executable(cpu_huffman_compression
        src/cpu_algorithm/huffman_cpu_compression.cpp)

add_executable(cpu_huffman_decompression
        src/cpu_algorithm/huffman_cpu_decompression.cpp)
