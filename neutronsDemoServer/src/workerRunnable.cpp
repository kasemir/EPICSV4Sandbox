/* WorkerRunnable.cpp
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * @author Kay Kasemir
 */
#include <workerRunnable.h>

namespace epics { namespace neutronServer {

void WorkerRunnable::run()
{
    while (do_run)
    {
        if (! new_work.wait(0.5))
            continue; // check is_running, wait again

        doWork();

        // Signal that we're done
        work_completed.signal();
    }
    thread_exited.signal();
}


void WorkerRunnable::shutdown()
{   // Request thread to exit
    do_run = false;
    thread_exited.wait(5.0);
}

}} // namespace neutronServer, epics
