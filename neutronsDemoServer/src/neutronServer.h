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

#define NS_TOF_MAX 160000 /** Maximum TOF value for the -r option (realistic data)*/
#define NS_TOF_NORM 10 /** Number of random samples for each TOF to generate a normal distribution*/

#define NS_ID_MIN1 0    /** Min pixel ID for detector 1 */
#define NS_ID_MAX1 1023 /** Max pixel ID for detector 1 */
#define NS_ID_MIN2 2048 /** Min pixel ID for detector 2 */
#define NS_ID_MAX2 3072 /** Max pixel ID for detector 2 */

/** Record that serves this type of pvData:
 *
 *  structure
 *      // Time stamp for everything in this structure,
 *      // userTag is sequential number to check for missed
 *      // updates
 *      time_t  timeStamp
 *      NTScalar proton_charge
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
    epics::pvData::uint32         pulse_id;

    // Pointers in to the records' data structure
    epics::pvData::PVTimeStamp    pvTimeStamp;
    epics::pvData::PVDoublePtr    pvProtonCharge;
    epics::pvData::PVUIntArrayPtr pvTimeOfFlight;
    epics::pvData::PVUIntArrayPtr pvPixel;
};

/** Runnable for demo events */
class FakeNeutronEventRunnable : public epicsThreadRunable
{
public:
    FakeNeutronEventRunnable(NeutronPVRecord::shared_pointer record,
                             double delay, size_t event_count,  bool random_count, bool realistic, size_t skip_packets);
    void run();
    void setDelay(double seconds);
    void setCount(size_t count);
    void setRandomCount(bool random_count);
    void shutdown();
private:
    NeutronPVRecord::shared_pointer record;
    bool is_running;
    epicsEvent processing_done;
    double delay;
    size_t event_count;
    bool random_count;
    bool realistic;
    size_t skip_packets;
};

}}

#endif  /* NEUTRONSERVER_H */
