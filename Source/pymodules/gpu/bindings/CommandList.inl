COALPY_FN(dispatch, cmdDispatch,R"(
    Dispatch method, which executes a compute shader with resources bound.
    
    Parameters:
        x (int)(optional): the number of groups on the x axis. By default is 1.
        y (int)(optional): the number of groups on the y axis. By default is 1.
        z (int)(optional):  the number of groups on the z axis. By defaul/t
        shader (Shader): object of type Shader. This will be the compute shader launched on the GPU.
        name (str)(optional): Debug name of this dispatch to see in render doc / profiling captures.
        constants (optional): Constant can be, an array of ints and floats, an array.array
                              or any object compatible with the object protocol, or a list of Buffer objects.
                              If a list of Buffer objects, these can be indexed via register(b#) in the shader used,
                              Every other type will always be bound to a constant Buffer on register(b0).
        samplers (optional): a single Sampler object, or an array of Sampler objects, or a single SamplerTable object, or an array of SamplerTable objects. If a single
                               SamplerTable object is used, the table will be automatically bound to register space 0, and each resource accessed either
                               by bindless dynamic indexing, or a hard register(s#). If an array of SamplerTable is passed, each resource
                               table will be bound using a register space index corresponding to the table index in the array, and the rules
                               to reference each sampler within the table stay the same.
                               If and array of Sampler objects are passed (or a single Sampler object), each sampler object is bound to a single register and always to the default space (0).
        inputs (optional): a single InResourceTable object, or an array of InResourceTable objects, or a single Texture/Buffer object, or an array of Texture/Buffer objects. If a single
                               object is used, the table will be automatically bound to register space 0, and each resource accessed either
                               by bindless dynamic indexing, or a hard register(t#). If an array of InResourceTable is passed, each resource
                               table will be bound using a register space index corresponding to the table index in the array, and the rules
                               to reference each resource within the table stay the same.
                               If and array of Texture/Buffer objects are passed (or a single Texture/Buffer object), each object is bound to a single register and always to the default space (0).
        outputs (optional): a single OutResourceTable object, or an array of OutResourceTable objects. Same rules as inputs apply,
                                 but we use registers u# to address the UAVs.

        indirect_args (Buffer)(optional): a single object of type Buffer, which contains the x, y and z groups packed tightly as 3 ints. 
                                  If this buffer is provided, the x, y and z arguments are ignored.
)")

COALPY_FN(copy_resource, cmdCopyResource, R"(
    copy_resource method, copies one resource to another.
    Both source and destination must be the same type (either Buffer or textures).
    Parameters:
        source (Texture or Buffer): an object of type Texture or Buffer. This will be the source resource to copy from.
        destination (Texture or Buffer): an object of type Texture or Buffer. This will be the destination resource to copy to.
        source_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the source Buffer.
        destination_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the destianation Buffer.
        size (tuple or int): if texture a tuple with the x, y and z size. If Buffer just an int with the proper byte size. Default covers the entire size.
)")

COALPY_FN(upload_resource, cmdUploadResource, R"(
    upload_resource method. Uploads an python array [], an array.array or any buffer protocol compatible object to a gpu resource.
    
    Parameters:
        source: an array of ints and floats, or an array.array object or any object compatible with the buffer protocol (for example a bytearray).
        destination (Texture or Buffer): a destination object of type Texture or Buffer.
        size (tuple): if texture upload, a tuple with the x, y and z size of the box to copy of the source buffer. If a Buffer upload, then this parameter gets ignored.
        destination_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the destianation Buffer.
)")

COALPY_FN(clear_append_consume_counter, cmdClearAppendConsume, R"(
    Clears the append consume counter of a Buffer that was created with the is_append_consume flag set to true.

    Parameters:
        source (Buffer): source Buffer object
        clear_value (int)(optional): integer value with the clear count. By default is 0.
)")

COALPY_FN(copy_append_consume_counter, cmdCopyAppendConsumeCounter, R"(
    Copies the counter of an append consume buffer to a destination resource.
    The destination must be a buffer that holds at least 4 bytes (considering the offset).

    Parameters:
        source (Buffer): source Buffer object, must have is_append_consume flag to True.
        destination (Buffer): Destination buffer to hold value.
        destination_offset (int)(optional): integer value with the destination's buffer offset, in bytes.
)")

COALPY_FN(begin_marker, cmdBeginMarker, R"(
    Sets a string marker. Must be paired with a call to end_marker. Markers can also be nested.
    You can use renderdoc / pix to explore the resulting frame's markers.
    
    Parameters:
        name (str): string text of the marker to be used.
)")

COALPY_FN(end_marker, cmdEndMarker, R"(
    Finishes the scope of a marker. Must have a corresponding begin_marker.
)")

#undef COALPY_FN
