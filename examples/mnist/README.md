# MNIST Example for PJRT C++ API

This directory contains an example of how to use the PJRT C++ API to train a Convolutional Neural Network (CNN) on the MNIST dataset.

## Overview

This example consists of two main parts:

1.  **Python (JAX):** The `mnist_jax.py` script uses JAX to define a CNN and export the necessary functions (`init_model`, `init_optimizer`, `train_step`) into the StableHLO format.
2.  **C++:** The `mnist_main.cpp` application uses the PJRT C++ API to load and execute these StableHLO programs, running a full training loop.

The `mnist_reader.hpp` file provides a simple utility for loading the MNIST dataset into memory.

## How to Run

Follow these steps to set up the environment, generate the necessary files, and run the training application.

### Step 1: Set up Python Environment

First, create and activate a Python virtual environment.

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### Step 2: Install Dependencies

Install the required Python packages, including JAX, and the C++ data downloader dependencies.

```bash
pip install -r requirements.txt
```

### Step 3: Download the MNIST Dataset

Run the download script to fetch the MNIST dataset.

```bash
python3 download_mnist.py
```

This will place the required `*-ubyte` data files in the current directory.

### Step 4: Generate StableHLO Programs

Run the JAX script to generate the StableHLO files for the model and training logic.

```bash
python mnist_jax.py
```

This will create `init_model.stablehlo`, `init_optimizer.stablehlo`, and `train_step.stablehlo`.

### Step 5: Build and Run the C++ Example

From the **root directory of this repository**, build the project using CMake.

```bash
# From the root of the cppxla project
cmake . -B build
cmake --build build
```

Now, run the C++ executable from the build directory. The necessary `.stablehlo` and dataset files will be copied there automatically during the build process.

```bash
./build/examples/mnist/mnist_main
```

The application will load the model, initialize weights, and run a training loop, printing the loss at each step.

## Cleanup

Once you are done, you can remove the downloaded dataset files with the following command:

```bash
rm t10k-images-idx3-ubyte t10k-labels-idx1-ubyte train-images-idx3-ubyte train-labels-idx1-ubyte
```