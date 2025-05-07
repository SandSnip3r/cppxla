# TODOs

Below is a list of things which should be done in no particular order. The list is certainly not exhaustive.

- Failure to create StableHLO from JAX should be loud and be caught in CMake
- Support Shapes other than scalars
- Support more than one input buffer
- Expose asynchronity of PJRT API
- Expose a function for each class to free resources which can throw. The destructors can then no longer throw and just log an error if freeing a resource fails.