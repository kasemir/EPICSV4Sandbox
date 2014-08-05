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
#include <epicsEvent.h>
#include <epicsThread.h>

#include <pv/pvDatabase.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>

namespace epics { namespace neutronServer {

/** Record that serves this type of pvData:
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
class NeutronPVRecord : public epics::pvDatabase::PVRecord
{
public:
    POINTER_DEFINITIONS(NeutronPVRecord);

    // PVRecord methods
    static NeutronPVRecord::shared_pointer create(std::string const & recordName);
    virtual ~NeutronPVRecord();
    virtual bool init();
    virtual void process();

    /** Update the values of the record */
    void update(epics::pvData::uint64 id, double charge,
                epics::pvData::shared_vector<const epics::pvData::uint32> tof,
                epics::pvData::shared_vector<const epics::pvData::uint32> pixel);

private:
    NeutronPVRecord(std::string const & recordName,
                    epics::pvData::PVStructurePtr const & pvStructure);

    // Time of last process() call
    epics::pvData::TimeStamp      timeStamp;

    // Pointers in to the records' data structure
    epics::pvData::PVTimeStamp    pvTimeStamp;
    epics::pvData::PVULongPtr     pvPulseID;
    epics::pvData::PVDoublePtr    pvProtonCharge;
    epics::pvData::PVUIntArrayPtr pvTimeOfFlight;
    epics::pvData::PVUIntArrayPtr pvPixel;
};

/** Runnable for demo events */
class FakeNeutronEventRunnable : public epicsThreadRunable
{
public:
    FakeNeutronEventRunnable(NeutronPVRecord::shared_pointer record,
                             double delay, size_t event_count);
    ~FakeNeutronEventRunnable();
    void run();
    void shutdown();

private:
    NeutronPVRecord::shared_pointer record;
    bool is_running;
    epicsEvent processing_done;
    double delay;
    size_t event_count;
};

}}

#endif  /* NEUTRONSERVER_H */
