# C++ XLA

This project has two primary goals. These two goals are to provide a way to use XLA:

1. Directly in C++ without needing to build all of XLA
2. In a CMake-based project

XLA is a C++ project, but the only way to use it in another project is to build all of XLA and have a project which is built using Bazel.

# Dependencies

1. Since we are not building XLA, we need to get it from somewhere. We need an "XLA release". For now, we use the XLA plugin that ships with `jax[cuda]`. This plugin is a shared library.
2. We also need an API to interface with the shared library. We need to use the [PJRT C API](https://github.com/openxla/xla/blob/main/xla/pjrt/c/pjrt_c_api.h) from [OpenXLA](https://github.com/openxla/xla).

This project works by loading the XLA shared library and wraps the C API with an easier to use C++ API which does not depend on anything external.

# Setup

After cloning this repository, two steps are required before you are ready to build:

1. Go grab the [pjrt_c_api.h](https://github.com/openxla/xla/blob/main/xla/pjrt/c/pjrt_c_api.h) from [OpenXLA](https://github.com/openxla/xla). Place it in the root level of this project. We provide one, but it is not guaranteed to be kept up to date.
2. Use [get_compile_options.ipynb](get_compile_options.ipynb) to reuse the JAX code which creates the compile options which are passed to PJRT. _Ideally you should run this python code on the same machine which you intend to use this C++ library on._ Copy these compile option bytes into the `compileOptionsData` array in [pjrt/client.cpp](pjrt/client.cpp).
3. Pip install jax\[cuda\] in a python virtual environment in the current directory:

```
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```
> Note: Sourcing this virtual environment sets `PJRT_PLUGIN_FULL_PATH_CONFIG` which is required by CMake to find the PJRT plugin path as installed by JAX

# Building

To build:
```
cmake . -B build
cmake --build build
```

# Running

To run the examples in this repository, first generate the example programs in the StableHlo text format. PJRT's input format for a program which you'd like to compile is StableHLO.

There are a few examples:
* `scalar_add_1`: A simple example of adding two scalars.
* `vector_add_1`: A simple example of adding two vectors.
* `mnist`: A more complex example of training a Convolutional Neural Network (CNN) on the MNIST dataset.

For the simple examples, you can generate the StableHLO like this:
```
python examples/scalar_add_1/myJax.py > scalar_add_1.stablehlo
```
Now, run the example with the generated StableHlo program:
```
./build/examples/scalar_add_1/scalar_add_1 scalar_add_1.stablehlo
```

For the MNIST example, please see the [README in the mnist example directory](examples/mnist/README.md).

# Design

Dive into the [pjrt directory](pjrt/) to see the design principals.

# What's Next?

I plan to tackle items in [the TODO list](TODO.md).