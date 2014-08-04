/* neutronServer.h
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * Based on MRK pvDataBaseCPP exampleServer
 *
 * @author Kay Kasemir
 */
#ifndef NEUTRONSERVER_H
#define NEUTRONSERVER_H

#include <shareLib.h>

#include <pv/pvDatabase.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>

namespace epics { namespace neutronServer {

/** Serves this type of pvData:
 *
 *  structure
 *      time_t  timeStamp // For everything in this structure
 *      NTScalar pulse
 *          ulong   value
 *      NTScalar protonCharge
 *          double  value
 *      NTScalarArray time_of_flight
 *          uint[]  value
 *      NTScalarArray pixel
 *          uint[]  value
 */
class epicsShareClass NeutronPVRecord :
    public epics::pvDatabase::PVRecord
{
public:
    POINTER_DEFINITIONS(NeutronPVRecord);

    static NeutronPVRecord::shared_pointer create(std::string const & recordName,
                                                  double delay, size_t event_count);
    virtual ~NeutronPVRecord();
    virtual bool init();
    virtual void start();
    void generateFakeValues();
    virtual void process();
    virtual void destroy();

private:
    NeutronPVRecord(std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,
        double delay, size_t event_count);

    double delay;
    size_t event_count;

    epics::pvData::TimeStamp      timeStamp;
    epics::pvData::PVTimeStamp    pvTimeStamp;
    epics::pvData::PVULongPtr     pvPulseID;
    epics::pvData::PVDoublePtr    pvProtonCharge;
    epics::pvData::PVUIntArrayPtr pvTimeOfFlight;
    epics::pvData::PVUIntArrayPtr pvPixel;

    static void neutronProcessor(void *me_parm);
};

}}

#endif  /* NEUTRONSERVER_H */
