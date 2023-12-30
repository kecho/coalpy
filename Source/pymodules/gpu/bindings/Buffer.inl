COALPY_FN(mappedMemory, bufferMappedMemory, R"(
    Returns:
        Returns a memory view object with the mapped memory.
        NOTE: only works if the buffer is created with BufferUsage.Upload flag.
)")

#undef COALPY_FN
