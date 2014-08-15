/* neutronServer.cpp
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
#include <pv/standardPVField.h>
#include "neutronServer.h"
#include "nanoTimer.h"

using namespace epics::pvData;
using namespace epics::pvDatabase;
using namespace std;
using namespace std::tr1;

namespace epics { namespace neutronServer {

NeutronPVRecord::shared_pointer NeutronPVRecord::create(string const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();

    // Create the data structure that the PVRecord should use
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(
        fieldCreate->createFieldBuilder()
        ->add("timeStamp", standardField->timeStamp())
        ->add("pulse", standardField->scalar(pvULong, ""))
        // Demo for manual setup of structure, could use
        // add("protonCharge", standardField->scalar(pvDouble, ""))
        ->addNestedStructure("protonCharge")
            ->setId("uri:ev4:nt/2012/pwd:NTScalar")
            ->add("value", pvDouble)
        ->endNested()
        ->add("time_of_flight", standardField->scalarArray(pvUInt, ""))
        ->add("pixel", standardField->scalarArray(pvUInt, ""))
        ->createStructure()
        );

    NeutronPVRecord::shared_pointer pvRecord(new NeutronPVRecord(recordName, pvStructure));
    if (!pvRecord->init())
        pvRecord.reset();
    return pvRecord;
}

NeutronPVRecord::NeutronPVRecord(string const & recordName, PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

NeutronPVRecord::~NeutronPVRecord()
{
}

bool NeutronPVRecord::init()
{
    initPVRecord();

    // Fetch pointers into the records pvData which will be used to update the values
    if (!pvTimeStamp.attach(getPVStructure()->getSubField("timeStamp")))
        return false;

    pvPulseID = getPVStructure()->getULongField("pulse.value");
    if (pvPulseID.get() == NULL)
        return false;

    pvProtonCharge = getPVStructure()->getDoubleField("protonCharge.value");
    if (pvProtonCharge.get() == NULL)
        return false;

    pvTimeOfFlight = getPVStructure()->getSubField<PVUIntArray>("time_of_flight.value");
    if (pvTimeOfFlight.get() == NULL)
        return false;

    pvPixel = getPVStructure()->getSubField<PVUIntArray>("pixel.value");
    if (pvPixel.get() == NULL)
        return false;

    return true;
}

void NeutronPVRecord::process()
{
    // Update timestamp
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
}

void NeutronPVRecord::update(uint64 id, double charge,
                             shared_vector<const uint32> tof,
                             shared_vector<const uint32> pixel)
{
    lock();
    try
    {
        beginGroupPut();
        pvPulseID->put(id);
        pvProtonCharge->put(charge);
        pvTimeOfFlight->replace(tof);
        pvPixel->replace(pixel);

        // TODO Create server-side overrun by updating same field
        // multiple times within one 'group put'
        // pvPulseID->put(id);

        process();
        endGroupPut();
    }
    catch(...)
    {
        unlock();
        throw;
    }
    unlock();
}


// When creating a large demo data arrays,
// the two arrays can be filled in separate threads / CPU cores

/** Runnable that creates a time-of-flight array */
class TimeOfFlightRunnable : public epicsThreadRunable
{
public:
    TimeOfFlightRunnable();
    void run();

    /** Start collecting events (fill array with simulated data) */
    void createEvents(size_t event_count, uint64 id);

    /** Wait for data to be filled and return it */
    shared_vector<const uint32> getEvents();

    /** Exit the runnable and thus thread */
    void shutdown();
private:
    /** Should thread run? */
    bool do_run;
    /** Did thread exit? */
    epicsEvent thread_exited;

    /** Parameters for new data request: How many events */
    size_t event_count;
    /** Parameters for new data request: Used to create dummy events */
    uint64 id;
    /** Has new request been submitted? */
    epicsEvent new_request;

    /** Result of a request for data */
    shared_vector<const uint32> tof_data;
    /** Signaled when tof_data has been updated */
    epicsEvent done_processing;
};

TimeOfFlightRunnable::TimeOfFlightRunnable()
: do_run(true), event_count(0), id(0)
{
}

void TimeOfFlightRunnable::run()
{
    while (do_run)
    {
        if (! new_request.wait(0.5))
            continue; // check is_running, wait again

        // Create fake data
        shared_vector<uint32> tof(event_count);
        fill(tof.begin(), tof.end(), id);
        tof_data = freeze(tof);

        // Signal that we're done
        done_processing.signal();
    }
    thread_exited.signal();
}

void TimeOfFlightRunnable::createEvents(size_t event_count, uint64 id)
{
    this->event_count = event_count;
    this->id = id;
    new_request.signal();
}

shared_vector<const uint32> TimeOfFlightRunnable::getEvents()
{
    // Wait that we're done
    done_processing.wait();
    return tof_data;
}

void TimeOfFlightRunnable::shutdown()
{   // Request exit from thread
    do_run = false;
    thread_exited.wait(5.0);
}



FakeNeutronEventRunnable::FakeNeutronEventRunnable(NeutronPVRecord::shared_pointer record,
                                                   double delay, size_t event_count)
: record(record), is_running(true), delay(delay), event_count(event_count)
{
}

FakeNeutronEventRunnable::~FakeNeutronEventRunnable()
{
}

void FakeNeutronEventRunnable::run()
{
    shared_ptr<TimeOfFlightRunnable> tof_runnable(new TimeOfFlightRunnable());
    shared_ptr<epicsThread> tof_thread(new epicsThread(*tof_runnable, "tof_processor", epicsThreadGetStackSize(epicsThreadStackMedium)));
    tof_thread->start();

    uint64 id = 0;
    size_t packets = 0, slow = 0;

    epicsTime last_run(epicsTime::getCurrent());
    epicsTime next_log(last_run);
    epicsTime next_run;

    NanoTimer timer;
    while (is_running)
    {
        // Compute time for next run
        next_run = last_run + delay;

        // Wait until then
        double sleep = next_run - epicsTime::getCurrent();
        if (sleep >= 0)
            epicsThreadSleep(sleep);
        else
            ++slow;

        // Mark this run
        last_run = epicsTime::getCurrent();
        ++packets;

        // Every 10 second, show how many updates we generated so far
        if (last_run > next_log)
        {
            next_log = last_run + 10.0;
            cout << packets << " packets, " << slow << " times slow";
            cout << ", array values set in " << timer;
            cout << endl;
            slow = 0;
        }

        // Increment the 'ID' of the pulse
        ++id;

        // Vary a fake 'charge' based on the ID
        double charge = (1 + id % 10)*1e8;

        // Create fake { time-of-flight, pixel } events,
        // using the ID to get changing values
        // TOF created in parallel thread, pixel in this thread
        tof_runnable->createEvents(event_count, id);

        shared_vector<uint32> pixel(event_count);

        // In reality, each event would have a different value,
        // which is simulated a little bit by actually looping over
        // each element.
        uint32 value = id * 10;

        // Set elements via [] operator of shared_vector
        // This takes about 1.5 ms for 200000 elements
        // timer.start();
        // for (size_t i=0; i<event_count; ++i)
        //   pixel[i] = value;
        // timer.stop();

        // This is much faster, about 0.6 ms, but less realistic
        // because our code no longer accesses each array element
        // to deposit a presumably different value
        // timer.start();
        // fill(pixel.begin(), pixel.end(), value);
        // timer.stop();

        // Set elements via direct access to array memory.
        // Speed almost as good as std::fill(), about 0.65 ms,
        // and we could conceivably put different values into
        // each array element.
        timer.start();
        uint32 *data = pixel.dataPtr().get();
        for (size_t i=0; i<event_count; ++i)
            *(data++) = value;
        timer.stop();

        shared_vector<const uint32> pixel_data(freeze(pixel));

        record->update(id, charge, tof_runnable->getEvents(), pixel_data);
        // TODO Overflow the server queue by posting several updates.
        // For client request "record[queueSize=2]field()", this causes overrun.
        // For queueSize=3 it's fine.
        // record->update(id, charge, tof_data, pixel_data);
    }

    tof_runnable->shutdown();
    cout << "Processing thread exits\n";
    processing_done.signal();
}

void FakeNeutronEventRunnable::setDelay(double seconds)
{   // No locking..
    delay = seconds;
}

void FakeNeutronEventRunnable::setCount(size_t count)
{   // No locking..
    event_count = count;
}

void FakeNeutronEventRunnable::shutdown()
{   // Request exit from thread
    is_running = false;
    processing_done.wait(5.0);
}


}} // namespace neutronServer, epics
