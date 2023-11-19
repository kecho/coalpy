#include <Config.h>
#if ENABLE_METAL

#include <iostream>
#include <string>
#include <Metal/Metal.h>
#include "MetalDevice.h"
#include "MetalShaderDb.h" 
#include "MSLData.h" 

namespace coalpy
{

MetalShaderDb::MetalShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

MetalShaderDb::~MetalShaderDb()
{
}

void MetalShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    if (shaderState.mslData == nullptr)
    {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "No Metal Shading Language data found.");
        return;
    }

    ShaderGPUPayload oldPayload = shaderState.payload;
    auto* oldMetalPayload = (MetalPayload*)oldPayload;
    if (oldMetalPayload != nullptr)
    {
        // TODO: Implement freeing of Metal resources, if needed.
        CPY_ASSERT(false);
        delete oldMetalPayload;
    }

    auto* payload = new MetalPayload;
    shaderState.payload = payload;

    render::MetalDevice& metalDevice = *static_cast<render::MetalDevice*>(m_parentDevice);

    // Create a Metal library from the MSL source code
    NSString* mslSource = [NSString stringWithUTF8String:shaderState.mslData->mslSource.c_str()];
    NSError* error = nil;
    id<MTLLibrary> library = [metalDevice.mtlDevice() newLibraryWithSource:mslSource options:nil error:&error];
    if (!library) {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "Could not create a Metal Library from the Metal Shading Language source.");
        return;
    }
    payload->mtlLibrary = library;

    // Get the entry point function from the library
    NSString* mainFn = [NSString stringWithUTF8String:shaderState.mslData->mainFn.c_str()];
    id<MTLFunction> computeFunction = [library newFunctionWithName:mainFn];
    if (!computeFunction) {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "Could not retrieve the entry point function from the Metal Library.");
        return;
    }

    // Create a compute pipeline state
    id<MTLComputePipelineState> pipelineState = [metalDevice.mtlDevice() newComputePipelineStateWithFunction:computeFunction error:&error];
    if (!pipelineState) {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "Error creating compute pipeline state.");
        return;
    }
    payload->mtlPipelineState = pipelineState;
}

}

#endif // ENABLE_METAL