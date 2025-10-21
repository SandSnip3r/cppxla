# Design Principals

This document aims to outlines design principals followed when building this API.

1. The Context is expected to be instantiated before any other object in the `pjrt` namespace and must remain alive until the end of life of every other object in the `pjrt` namespace.
2. Handle all resource management in C++ constructors and destructors. The PJRT C API gives us a lot of pointers to objects which we are then expected to free/destroy/release. At any point when we encounter one of these, we wrap it in a C++ object so that a user of this API never has to be worried of resource leaks.
3. When a PJRT concept exists, but does not own a resource, the corresponding C++ API shall be called a View.
4. Exceptions are our error handling mechanism.
5. API calls never cache results. For example, when getting a specific device via the Client class's API, the number of available devices becomes known. This number of available devices will not be cached. If the user asks for the number of available devices, the PJRT API needs be invoked again.
6. Destructors do not throw, even though errors can occur during destruction. To be safe, use `destroy()` methods before destructors are called.