# Vector Add Example

This example demonstrates a simple "add one" operation on a vector of floating-point values using the PJRT C++ API.

## How to Run

1.  **Generate StableHLO:**
    ```bash
    python3 examples/vector_add_1/myJax.py
    ```
    This will create `vector_add_1.stablehlo` in the `examples/vector_add_1` directory.

2.  **Build and Run:**
    From the root of the repository, build the project and run the example:
    ```bash
    cmake --build build
    ./build/examples/vector_add_1/vector_add_1
    ```
