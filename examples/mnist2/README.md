# MNIST Example for PJRT C++ API

This directory contains an example of how to use JAX to create StableHLO programs for a Convolutional Neural Network (CNN) that can be used with the PJRT C++ API.

## Overview

The main script, `mnist_jax.py`, defines a CNN for MNIST classification and provides the logic to export the following three key functions into StableHLO format:

*   `init_model`: Initializes the weights and biases of the CNN.
*   `init_optimizer`: Initializes the state of the Adam optimizer.
*   `train_step`: Performs a single training step, including the forward pass, loss calculation, and weight updates.

These generated StableHLO files can then be loaded and executed by a PJRT C++ application.

## How to Run

To generate the StableHLO programs, follow these steps:

1.  **Create a virtual environment:**
    ```bash
    python3 -m venv venv
    ```

2.  **Activate the virtual environment:**
    ```bash
    source venv/bin/activate
    ```

3.  **Install the required dependencies:**
    ```bash
    pip install jax jaxlib optax
    ```

4.  **Run the script:**
    ```bash
    python mnist_jax.py
    ```

This will generate the following files in the current directory:

*   `init_model.stablehlo`
*   `init_optimizer.stablehlo`
*   `train_step.stablehlo`

These files contain the StableHLO representation of the respective functions and are ready to be used with the PJRT C++ API.
