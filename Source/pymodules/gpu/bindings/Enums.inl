//Global declaration of enums.

COALPY_ENUM_BEGIN(DeviceFlags, "Flags utilized for device creation.")
COALPY_ENUM(EnableDebug, render::DeviceFlags::EnableDebug, "Enable debug drivers for current device.")
COALPY_ENUM_END(DeviceFlags)

COALPY_ENUM_BEGIN(ShaderModel, "Available shader models. Specify a shader model in shader_model argument of set_current_adapter.")
COALPY_ENUM(Sm6_0, ShaderModel::Sm6_0, "Shader model 6.0 enum value.") 
COALPY_ENUM(Sm6_1, ShaderModel::Sm6_1, "Shader model 6.1 enum value.") 
COALPY_ENUM(Sm6_2, ShaderModel::Sm6_2, "Shader model 6.2 enum value.") 
COALPY_ENUM(Sm6_3, ShaderModel::Sm6_3, "Shader model 6.3 enum value.") 
COALPY_ENUM(Sm6_4, ShaderModel::Sm6_4, "Shader model 6.4 enum value.") 
COALPY_ENUM(Sm6_5, ShaderModel::Sm6_5, "Shader model 6.5 enum value.") 
COALPY_ENUM_END(ShaderModel)

COALPY_ENUM_BEGIN(TextureType, "Type of texture enumeration. Use enum values located at coalpy.gpu.TextureType")
COALPY_ENUM(k1d,          render::TextureType::k1d, "One dimensional texture. Width & depth defaults to 1.")
COALPY_ENUM(k2d,          render::TextureType::k2d, "Two dimensional texture. Depth defaults to 1.")
COALPY_ENUM(k2dArray,     render::TextureType::k2dArray, "Two dimensional array texture.")
COALPY_ENUM(k3d,          render::TextureType::k3d, "Volume texture type." )
COALPY_ENUM(CubeMap,      render::TextureType::CubeMap, "Cube map texture.")
COALPY_ENUM(CubeMapArray, render::TextureType::CubeMapArray, "Array of cube maps. Use the Depth as the number of cube maps supported in an array.")
COALPY_ENUM_END(TextureType)

COALPY_ENUM_BEGIN(BufferType, "Type of buffer. Use enum values located at coalpy.gpu.BufferType")
COALPY_ENUM(Standard,       render::BufferType::Standard, "Standard buffer. Obeys the format specified when sampling / writting into it.")
COALPY_ENUM(Structured,     render::BufferType::Structured, "Structured buffer, requires stride parameter.")
COALPY_ENUM(Raw,            render::BufferType::Raw, "Raw byte buffer.")
COALPY_ENUM_END(BufferType)

COALPY_ENUM_BEGIN(MemFlags, "Memory access enumerations. Use enum values located at coalpy.gpu.MemFlags")
COALPY_ENUM(GpuRead,  render::MemFlag_GpuRead, "Specify flag for resource read access from InResourceTable / SRVs")
COALPY_ENUM(GpuWrite, render::MemFlag_GpuWrite, "Specify flag for resource write access from an OutResourceTable / UAV")
COALPY_ENUM_END(MemFlags)

COALPY_ENUM_BEGIN(FilterType, "Filter types for samplers enumeration. Use enum values located at coalpy.gpu.FilterType")
COALPY_ENUM(Point,        render::FilterType::Point, "Point (nearest neighbor) filter type.")
COALPY_ENUM(Linear,       render::FilterType::Linear, "Bilinear/trilinear filter type." )
COALPY_ENUM(Min,          render::FilterType::Min, "Max value of neighborhood filter type.")
COALPY_ENUM(Max,          render::FilterType::Max, "Min value of neighborhood filter type.")
COALPY_ENUM(Anisotropic,  render::FilterType::Anisotropic, "High quality anisotropic filter type.")
COALPY_ENUM_END(FilterType)

COALPY_ENUM_BEGIN(TextureAddressMode,  "Address behaviour of texture coordinates enumeration. Use enum values located at coalpy.gpu.TextureAddressMode")
COALPY_ENUM(Wrap,   render::TextureAddressMode::Wrap, "Repeats samples when UV coordinates exceed range.")
COALPY_ENUM(Mirror, render::TextureAddressMode::Mirror, "Applies UV mirroring when UV coordinates go into the next edge. ")
COALPY_ENUM(Clamp,  render::TextureAddressMode::Clamp, "Clamps the UV coordinates at the edges. ")
COALPY_ENUM(Border, render::TextureAddressMode::Border, "Samples a border when UVs are in the edge. Set the border color in the sampler object.")
COALPY_ENUM_END(TextureAddressMode)

#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_ENUM_END
