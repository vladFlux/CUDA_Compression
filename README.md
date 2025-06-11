# CUDA Compression App

A high-performance compression application that leverages NVIDIA GPU acceleration for fast data compression and 
decompression.

## Prerequisites

### Hardware Requirements
- **NVIDIA GPU**: This application requires an NVIDIA graphics card with CUDA support
- Minimum compute capability 3.5 or higher recommended

### Software Requirements
- **CUDA Toolkit**: Must be installed following the official installation guide on [NVIDIA's website](https://developer.nvidia.com/cuda-downloads)
  - Recommended version: **CUDA 12.2** or newer
- Compatible C/C++ compiler
  - Must be compatible with the installed CUDA Toolkit
  - GCC 11 or newer is recommended
- **CMake 3.28** and **Make 4.3** or higher is recommended

### You can verify the CUDA installation running the following commands
```bash
nvcc --version
nvidia-smi
```

## Installation

### Clone the Repository
```bash
git clone https://github.com/vladFlux/CUDA_Compression.git
cd CUDA_Compression/
```

### Building the binaries from source

### 1. Create a build folder inside the ``CUDA_Compression`` directory
```bash
mkdir build
cd build/
```

### 2. Start building from source
```bash
cmake ..
make -j$(nproc)
```

The new binaries will be written to the ``build/`` directory

### Running the algorithms

While in the ``build/`` directory, execute algorithms as followed:

```bash
./<algorithm_name> <input_file_path> <output_file_path>
```

## If you wish to run the algorithms using the Python app, follow these instructions

### 1. Create a Python virtual environment in the ``CUDA_Compression`` directory and activate it
```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 2. Install all the required Python packages
```bash
pip install -r requirements.txt 
```

### 3. Move to the app directory and run
```bash
cd python_app/
python3 CUDA_app.py
```
