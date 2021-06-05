#include <coalpy.core/Stopwatch.h>

namespace coalpy
{

void Stopwatch::start()
{
    m_timeStart = std::chrono::high_resolution_clock::now();
}

unsigned Stopwatch::timeMicroSeconds() const
{
    return
    (unsigned)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_timeStart).count();
}

}
