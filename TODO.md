# TODOs

Below is a list of things which should be done in no particular order. The list is certainly not exhaustive.

- Support Shapes other than scalars
- Support more than one input buffer
- Expose asynchronity of PJRT API
- Expose a function for each class to free resources which can throw. The destructors can then no longer throw and just log an error if freeing a resource fails.
- There are some calls which could either be done ahead-of-time or just-in-time, such as getting the output shape of a loaded executable. Getting this info requires a handful of calls into PJRT. It could be done immediately once the LoadedExecutable is being constructed, or it could be done at the time of the first execute call. For now, I have chosen to do it immediately upon construction, so as to minimize execution time. The cost is that maybe we sometimes fetch some things which will not be used.
- Which C++ standard is our minimum?

# Open Questions to PJRT

- Is it our responsibility to free PJRT_Executable_OutputElementTypes_Args::output_types?
- What happens regarding the `event` when PJRT_Buffer_ToHostBuffer fails? Is it null? If it's not, is it our responsibility to free?