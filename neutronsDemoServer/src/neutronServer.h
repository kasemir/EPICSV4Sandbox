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

#ifdef USE_PVXS
#    include <pvxs/data.h>
#    include <pvxs/server.h>
#    include <pvxs/sharedpv.h>
#else
#    include <pv/pvDatabase.h>
#    include <pv/timeStamp.h>
#    include <pv/pvTimeStamp.h>
#endif

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
#ifdef USE_PVXS
struct Neutrons {
    // We don't have to define Neutrons structure here,
    // but we do it for completness and comparison with NeutronPVRecord

    //! A TypeDef which can be appended
    PVXS_API
    pvxs::TypeDef build() const
    {
        using namespace pvxs;
        using namespace pvxs::members;

        TypeDef def(
            TypeCode::Struct,
            {
                Struct("timeStamp", "time_t", {
                    Int64("secondsPastEpoch"),
                    Int32("nanoseconds"),
                    Int32("userTag"),
                }),
                Struct("time_of_flight", "epics:nt/NTScalarArray:1.0", {
                    UInt32A("value")
                }),
                Struct("pixel", "epics:nt/NTScalarArray:1.0", {
                    UInt32A("value")
                }),
                Struct("proton_charge", "epics:nt/NTScalar:1.0", {
                    Float64("value")
                }),
            }
        );

        return def;
    }
    //! Instanciate
    inline pvxs::Value create() const {
        return build().create();
    }
};

#else // !defined(USE_PVXS)

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
#endif // USE_PVXS

/** Runnable for demo events */
class FakeNeutronEventRunnable : public epicsThreadRunable
{
public:
    FakeNeutronEventRunnable(const std::string& record_name,
                             double delay, size_t event_count,  bool random_count, bool realistic, size_t skip_packets);
    void run();
    void setDelay(double seconds);
    void setCount(size_t count);
    void setID(size_t id);
    void setRandomCount(bool random_count);
    void shutdown();
#ifdef USE_PVXS
    pvxs::server::SharedPV& getRecord()
    {
        return record;
    }
#else
    NeutronPVRecord::shared_pointer getRecord()
    {
        return record;
    }
#endif
private:
#ifdef USE_PVXS
    pvxs::server::SharedPV record;
    pvxs::TypeDef recordDef;
#else
    NeutronPVRecord::shared_pointer record;
#endif
    bool is_running;
    epicsEvent processing_done;
    double delay;
    size_t event_count;
    bool random_count;
    bool realistic;
    size_t skip_packets;
    uint64_t id;
};

}}

#endif  /* NEUTRONSERVER_H */
