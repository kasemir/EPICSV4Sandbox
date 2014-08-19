/* WorkerRunnable.h
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * @author Kay Kasemir
 */
#ifndef __WORKER_RUNNABLE_H__
#define __WORKER_RUNNABLE_H__
#include <epicsEvent.h>
#include <epicsThread.h>

namespace epics { namespace neutronServer {

/** Runnable that performs work */
class WorkerRunnable : public epicsThreadRunable
{
public:
    WorkerRunnable()
    : do_run(true)
    {}

    void run();

    /** Exit the runnable and thus thread */
    void shutdown();

protected:
    void startWork()
    {
        new_work.signal();
    }
    virtual void doWork() = 0;
    void waitForCompletion()
    {
        work_completed.wait();
    }

private:
    /** Should thread run? */
    bool do_run;
    /** Did thread exit? */
    epicsEvent thread_exited;

    /** Has new work request been submitted? */
    epicsEvent new_work;

    /** Signaled when work has completed */
    epicsEvent work_completed;
};

}} // namespace neutronServer, epics
#endif // __WORKER_RUNNABLE_H__
