# C++ XLA

This project has two primary goals. These two goals are to provide a way to use XLA:

1. Directly in C++ without needing to build all of XLA.
2. In a CMake-based project.

XLA is a C++ project, but the only way to use it in another project is to build all of XLA and have a project which is built using Bazel.

# Dependencies

1. Since we do not need to build XLA, we need to get it from somewhere. We need an "XLA release". For now, we use the XLA plugin that ships with `jax[cuda]`. This plugin in a shared library.
2. We also need an API to interface with the shared library. We need to use the [PJRT C API](https://github.com/openxla/xla/blob/main/xla/pjrt/c/pjrt_c_api.h) from [OpenXLA](https://github.com/openxla/xla).

This project works by loading the XLA shared library and wraps the C API with an easier to use C++ API which does not depend on anything external.

# Setup

After cloning this repository, two steps are required before you are ready to build:

1. Go grab the pjrt_c_api.h file from OpenXLA. Place it in the root level of this project. We provide one, but it is not always kept up to date.
2. Pip install jax\[cuda\] in a python virtual environment in the current directory.

```
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

# Building

To build:
```
cmake . -B build
cmake --build build
```

# Running

To run:
```
./build/pjrt_example
```

# Design

Dive into the [pjrt directory](pjrt/) to see the design principals.

# What's Next?

I plan to tackle items in [the TODO list](TODO.md).