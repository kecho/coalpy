#include <Config.h>
#if ENABLE_METAL
#include "MSLData.h"

namespace coalpy
{

MSLData::~MSLData()
{
}

void MSLData::AddRef()
{
    ++m_refCount;
}

void MSLData::Release()
{
    --m_refCount;
    if (m_refCount == 0)
        delete this;
}

}

#endif // ENABLE_METAL