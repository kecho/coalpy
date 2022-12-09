#pragma once

namespace coalpy
{
namespace render
{

template<class T>
T alignByte(T offset, T alignment)
{
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

}
}
