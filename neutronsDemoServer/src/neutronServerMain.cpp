/* NeutronServerMain.cpp
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * Based on MRK pvDataBaseCPP exampleServer
 *
 * @author Kay Kasemir
 */
#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>
#include <unistd.h>

#include <epicsThread.h>

#include "neutronServer.h"

using namespace epics::neutronServer;
using namespace std;
#ifndef USE_PVXS
#   include <pv/standardField.h>
#   include <pv/standardPVField.h>
#   include <pv/channelProviderLocal.h>
#   include <pv/serverContext.h>
#   include <pv/createRequest.h>
    using namespace epics::pvData;
    using namespace epics::pvAccess;
    using namespace epics::pvDatabase;
#endif

static void help(const char *name)
{
    cout << "USAGE: " << name << " [options]" << endl;
    cout << "  -h        : Help" << endl;
    cout << "  -d seconds: Delay between packages (default 0.01)" << endl;
    cout << "  -e count  : Max event count per packet (default 10)" << endl;
    cout << "  -m : Random event count, using 'count' as maximum" << endl;
    cout << "  -r : Generate normally distributed data which looks semi realistic." << endl;
    cout << "  -s Nth : Don't send every N'th packet to simulate losing data packets (default 0 which means disabled)." << endl;
}

int main(int argc,char *argv[])
{
    double delay = 0.01;
    size_t event_count = 10;
    bool random_count = false;
    bool realistic = false;
    size_t skip_packets = 0;

    int opt;
    while ((opt = getopt(argc, argv, "d:e:h:mrs:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            delay = atof(optarg);
            break;
        case 'e':
            event_count = (size_t)atol(optarg);
            break;
        case 'h':
            help(argv[0]);
            return 0;
        case 'm':
        	random_count = true;
            break;
        case 'r':
        	realistic = true;
                break;
        case 's':
                skip_packets = (size_t)atol(optarg);
                break;
        default:
            help(argv[0]);
            return -1;
        }
    }

    cout << "Delay : " << delay << " seconds" << endl;
    cout << "Events: " << event_count << endl;
    cout << "Realistic: " << realistic << endl;
    if (skip_packets > 0) {
      cout << "Skipping every " << skip_packets << " packets." << endl;
    }

    std::shared_ptr<FakeNeutronEventRunnable> runnable(new FakeNeutronEventRunnable("neutrons", delay, event_count, random_count, realistic, skip_packets));
    auto neutrons(runnable->getRecord());

#ifdef USE_PVXS
    // Nothing required for PVXS
#else
    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();

    if (! master->addRecord(neutrons))
        throw std::runtime_error("Cannot add record " + neutrons->getRecordName());
#endif

    shared_ptr<epicsThread> thread(new epicsThread(*runnable, "processor", epicsThreadGetStackSize(epicsThreadStackMedium)));
    thread->start();

#ifdef USE_PVXS
    pvxs::server::Server serv = pvxs::server::Config::from_env().build().addPV("neutrons", neutrons);
    serv.start();
#else
    ServerContext::shared_pointer pvaServer = startPVAServer(PVACCESS_ALL_PROVIDERS,0,true,true);
#endif

    cout << "neutronServer running\n";
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    runnable->shutdown();
#ifdef USE_PVXS
    serv.stop();
#else
    pvaServer->shutdown();
#endif
    epicsThreadSleep(1.0);
    // pvaServer->destroy();
    // channelProvider->destroy();

    return 0;
}

