//#if defined(_MSFT_WIN)
#if defined(_MSC_VER)
#  define NOMINMAX // this is for a bug in windows.h related to the macros
                   // "min" and "max" and their conflict with STL.
#  include <windows.h>
#else
#  include <sys/time.h>
#endif

#ifndef _MYTIMER_H_
#define _MYTIMER_H_



class myTimer
{

private:
#if defined(_MSC_VER)
    LARGE_INTEGER m_depart;
#else
    timeval m_depart;
#endif

public:

inline void start()
{
#if defined(_MSC_VER)
    QueryPerformanceCounter(&m_depart);
#else
    gettimeofday(&m_depart, 0);
#endif
};


inline float GetSeconds() const
{
#if defined(_MSC_VER)
    LARGE_INTEGER now;
    LARGE_INTEGER freq;

    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);

    return (now.QuadPart - m_depart.QuadPart) / static_cast<float>(freq.QuadPart);
#else
    timeval now;
    gettimeofday(&now, 0);

    return now.tv_sec - m_depart.tv_sec + (now.tv_usec - m_depart.tv_usec) / 1000000.0f;
#endif
};

};

#endif // _MYTIMER_H_
