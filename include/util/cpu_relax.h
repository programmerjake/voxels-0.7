#ifndef CPU_RELAX_H_INCLUDED
#define CPU_RELAX_H_INCLUDED

#if defined(__i386__) || defined(__x86_64__)
#if defined(__GNUC__)
inline void cpu_relax()
{
    __builtin_ia32_pause();
}
#elif
#include <xmmintrin.h>
inline void cpu_relax()
{
    _mm_pause();
}
#else
inline void cpu_relax()
{
}
#endif
#else
inline void cpu_relax()
{
}
#endif

#endif // CPU_RELAX_H_INCLUDED
