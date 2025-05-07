# Design Principals

This document aims to outlines design principals followed when building this API.

1. The Context is expected to be instantiated before any other object in the `pjrt` namespace and must remain alive until the end of life of every other object in the `pjrt` namespace.
2. Handle all resource management in C++ constructors and destructors. The PJRT C API gives us a lot of pointers to objects which we are then expected to free/destroy/release. At any point when we encounter one of these, we wrap it in a C++ object so that a user of this API never has to be worried of resource leaks.
3. When a PJRT concept exists, but does not own a resource, the corresponding C++ API shall be called a View.
4. Exceptions are our error handling mechanism.