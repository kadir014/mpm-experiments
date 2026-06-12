/*

  This file is a part of the Nova Physics Engine
  project and distributed under the MIT license.

  Copyright © Kadir Aksoy
  https://github.com/kadir014/nova-physics

*/

#ifndef NOVAPHYSICS_PROFILER_H
#define NOVAPHYSICS_PROFILER_H


/**
 * @file profiler.h
 * 
 * @brief Built-in performance profiler.
 */


/**
 * @brief Timings for parts of single space simulation step in seconds.
 */
typedef struct {
    double step; /**< Time spent in one simulation step. */
    double clear_grid;
    double p2g0;
    double p2g1;
    double update_grid;
    double g2p;
} nvProfiler;


static inline void nvProfiler_reset(nvProfiler *profiler) {
    profiler->step = 0.0;
    profiler->clear_grid = 0.0;
    profiler->p2g0 = 0.0;
    profiler->p2g1 = 0.0;
    profiler->update_grid = 0.0;
    profiler->g2p = 0.0;
}


#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)

    #include <windows.h>

    typedef struct {
        double elapsed;
        LARGE_INTEGER _start;
        LARGE_INTEGER _end;
    } nvPrecisionTimer;

    static inline void nvPrecisionTimer_start(nvPrecisionTimer *timer) {
        QueryPerformanceCounter(&timer->_start);
    }

    static inline double nvPrecisionTimer_stop(nvPrecisionTimer *timer ) {
        QueryPerformanceCounter(&timer->_end);

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        timer->elapsed = (double)(timer->_end.QuadPart - timer->_start.QuadPart) / (double)frequency.QuadPart;
        return timer->elapsed;
    }

#else

    #include <time.h>
    #include <unistd.h>

    // TODO: On OSX, frequency can be milliseconds instead of nanoseconds
    #define NS_PER_SECOND 1e9

    typedef struct {
        double elapsed;
        struct timespec _start;
        struct timespec _end;
        struct timespec _delta;
    } nvPrecisionTimer;

    static inline void nvPrecisionTimer_start(nvPrecisionTimer *timer) {
        clock_gettime(CLOCK_REALTIME, &timer->_start);
    }

    static inline double nvPrecisionTimer_stop(nvPrecisionTimer *timer) {
        clock_gettime(CLOCK_REALTIME, &timer->_end);

        timer->_delta.tv_nsec = timer->_end.tv_nsec - timer->_start.tv_nsec;
        timer->_delta.tv_sec = timer->_end.tv_sec - timer->_start.tv_sec;

        if (timer->_delta.tv_sec > 0 && timer->_delta.tv_nsec < 0) {
            timer->_delta.tv_nsec += NS_PER_SECOND;
            timer->_delta.tv_sec--;
        }
        else if (timer->_delta.tv_sec < 0 && timer->_delta.tv_nsec > 0) {
            timer->_delta.tv_nsec -= NS_PER_SECOND;
            timer->_delta.tv_sec++;
        }

        timer->elapsed = (double)timer->_delta.tv_nsec / NS_PER_SECOND;
        return timer->elapsed;
    }

#endif


#endif