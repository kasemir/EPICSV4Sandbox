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

using namespace epics::pvData;
using namespace epics::pvDatabase;
using std::tr1::static_pointer_cast;
using std::string;

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
    uint64 id = 0;
    int loops = 0;
    int loops_in_10_seconds = int(10.0 / delay);
    size_t packets = 0;
    while (is_running)
    {
        epicsThreadSleep(delay);
        ++packets;
        // Every 10 second, show how many updates we generated so far
        if (++loops >= loops_in_10_seconds)
        {
            loops = 0;
            std::cout << packets << " packets\n";
        }

        // Increment the 'ID' of the pulse
        ++id;

        // Vary a fake 'charge' based on the ID
        double charge = (1 + id % 10)*1e8;

        // Create fake { time-of-flight, pixel } events,
        // using the ID to get changing values
        shared_vector<uint32> tof(event_count);
        shared_vector<uint32> pixel(event_count);
        for (size_t i=0; i<event_count; ++i)
        {
            tof[i] = id;
            pixel[i] = 10*id;
        }
        shared_vector<const uint32> tof_data(freeze(tof));
        shared_vector<const uint32> pixel_data(freeze(pixel));

        record->update(id, charge, tof_data, pixel_data);
    }
    std::cout << "Processing thread exits\n";
    processing_done.signal();
}

void FakeNeutronEventRunnable::shutdown()
{   // Request exit from thread
    is_running = false;
    processing_done.wait(5.0);
}


}} // namespace neutronServer, epics
