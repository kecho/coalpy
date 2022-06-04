COALPY_FN(resolve, resolve,R"(Waits for binary data of the texture to be downloaded. This method will block the CPU until the GPU has finished downloading the data. After this call, check availability with is_ready and then get the data as desired. )")
COALPY_FN(is_ready, isReady,R"( Polls the GPU (internally a fence) to check if the data is ready. When doing async this method can be used to query until we have data ready.)")
COALPY_FN(data_as_bytearray, dataAsByteArray,R"( returns the data as a bytearray. This Byte array is internally cached and referenced by the ResourceDownload object.)")
COALPY_FN(data_byte_row_pitch, dataByteRowPitch,R"( Assuming the data is ready, this method returns the row pitch (in case the resource is a texture).)")
#undef COALPY_FN
