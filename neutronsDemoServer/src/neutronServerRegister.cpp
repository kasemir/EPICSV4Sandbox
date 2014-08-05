/* neutronServerRegister.cpp
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * Based on MRK pvDataBaseCPP exampleServer
 *
 * @author Kay Kasemir
 */
#include <iostream>

#include <iocsh.h>
#include <epicsExport.h>

#include <pv/pvDatabase.h>

#include <neutronServer.h>

using namespace std;
using namespace epics::pvDatabase;
using namespace epics::neutronServer;

static const iocshArg createArg0 = { "recordName", iocshArgString };
static const iocshArg createArg1 = { "updateDelaySecs", iocshArgDouble };
static const iocshArg createArg2 = { "eventCount", iocshArgInt };
static const iocshArg *createArgs[] = { &createArg0, &createArg1, &createArg2 };
static const iocshFuncDef createFuncDef = { "neutronServerCreateRecord", 3, createArgs};
static void createFunc(const iocshArgBuf *args)
{
    char *name = args[0].sval;
    double delay = args[1].dval;
    size_t event_count = args[2].ival;

    PVRecordPtr record = NeutronPVRecord::create(name, delay, event_count);
    if (! PVDatabase::getMaster()->addRecord(record))
        cout << "Cannot create neutron record '" << name << "'" << endl;
}

static void neutronServerRegister(void)
{
    static int times = 0;
    if (++times == 1)
        iocshRegister(&createFuncDef, createFunc);
    else
        cout << "neutronServerRegister called " << times << " times" << endl;
}

extern "C"
{
    epicsExportRegistrar(neutronServerRegister);
}
