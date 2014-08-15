/* nanoTimer.h
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * @author Kay Kasemir
 */
#ifndef __NANO_TIMER_H__
#define __NANO_TIMER_H__

#include <sys/time.h>
#include <iostream>

class NanoTimer
{
    uint64_t total_ns;
    uint64_t total_runs;
    uint64_t start_ns;

public:
    NanoTimer()
    : total_ns(0), total_runs(0)
    {
        start();
    }

    void start()
    {
        start_ns = getCurrentNanosecs();
    }

    void stop()
    {
        uint64_t ns = getCurrentNanosecs() - start_ns;
        total_ns += ns;
        ++total_runs;
    }

    uint64_t getAverageNanosecs() const
    {
        if (total_runs <= 0)
            return 0;
        return total_ns / total_runs;
    }

    static uint64_t getCurrentNanosecs()
    {   // Compare pvCommonCPP/mbSrc/mb.[h,cpp]
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * 1000000000 + static_cast<uint64_t>(ts.tv_nsec);
    }
};

std::ostream& operator<<(std::ostream& out, const NanoTimer& timer)
{
    double avg = timer.getAverageNanosecs();
    if (avg < 1000.0)
    {
        out << avg << " nanoseconds";
        return out;
    }
    avg /= 1000.0;
    if (avg < 1000.0)
    {
        out << avg << " microseconds";
        return out;
    }
    avg /= 1000.0;
    if (avg < 1000.0)
    {
        out << avg << " milliseconds";
        return out;
    }
    avg /= 1000.0;
    out << avg << " seconds";
    return out;
}

#endif // __NANO_TIMER_H__
