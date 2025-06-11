# CUDA Compression App

A high-performance compression application that leverages NVIDIA GPU acceleration for fast data compression and 
decompression.

## Prerequisites

### Hardware Requirements
- **NVIDIA GPU**: This application requires an NVIDIA graphics card with CUDA support
- Minimum compute capability 3.5 or higher recommended

### Software Requirements
- **CUDA Toolkit**: Must be installed following the official installation guide on [NVIDIA's website](https://developer.nvidia.com/cuda-downloads)
- Compatible C/C++ compiler
- **CMake 3.28** and **Make 4.3** or higher

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

You can run the app with the included binaries or build from source.

If you wish to build from source, follow these steps:

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

The new binaries will be written to the ``bin/`` directory


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
cd Python\ App/
python3 main.py
```
