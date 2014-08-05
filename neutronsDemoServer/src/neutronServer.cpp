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
#include <epicsThread.h>
#include <pv/standardPVField.h>
#include "neutronServer.h"

using namespace epics::pvData;
using namespace epics::pvDatabase;
using std::tr1::static_pointer_cast;
using std::string;

namespace epics { namespace neutronServer {

/** Called by thread to 'process' the record,
 *  generating new data
 */
void NeutronPVRecord::neutronProcessor(void *me_parm)
{
    NeutronPVRecord *me = (NeutronPVRecord *)me_parm;
    int loops = 0;
    int loops_in_10_seconds = int(10.0 / me->delay);
    size_t packets = 0;
    while (me->isRunning())
    {
        epicsThreadSleep(me->delay);
        ++packets;
        // Every 10 second, show how many updates we generated so far
        if (++loops >= loops_in_10_seconds)
        {
            loops = 0;
            std::cout << packets << " packets\n";
        }

        me->lock();
        try
        {
            me->beginGroupPut();
            me->process();
            me->endGroupPut();
        }
        catch(...)
        {
            me->unlock();
            throw;
        }
        me->unlock();
    }
    std::cout << "Processing thread exits\n";
    me->processing_done.signal();
}


NeutronPVRecord::shared_pointer NeutronPVRecord::create(string const & recordName,
        double delay, size_t event_count)
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

    NeutronPVRecord::shared_pointer pvRecord(new NeutronPVRecord(recordName, pvStructure, delay, event_count));
    if (!pvRecord->init())
        pvRecord.reset();
    return pvRecord;
}

NeutronPVRecord::NeutronPVRecord(
    string const & recordName,
    PVStructurePtr const & pvStructure,
    double delay, size_t event_count)
: PVRecord(recordName,pvStructure), is_running(true), delay(delay), event_count(event_count)
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

void NeutronPVRecord::start()
{
    epicsThreadCreate("processor", epicsThreadPriorityScanHigh,
            epicsThreadGetStackSize(epicsThreadStackMedium),
            neutronProcessor, this);
}

void NeutronPVRecord::generateFakeValues()
{
    // Increment the 'ID' of the pulse
    uint64 id = pvPulseID->get() + 1;
    pvPulseID->put(id);

    // Vary a fake 'charge' based on the ID
    pvProtonCharge->put( (1 + id % 10)*1e8 );

    // Create fake { time-of-flight, pixel } events,
    // using the ID to get changing values
    shared_vector<uint32> tof(event_count);
    shared_vector<uint32> pixel(event_count);
    for (size_t i=0; i<event_count; ++i)
    {
        tof[i] = id;
        pixel[i] = 10*id;
    }

    if (! tof.unique()) // TODO Remove once crashes have been resolved
        std::cout << "tof is not unique?!\n";
    if (! pixel.unique())
        std::cout << "pixel is not unique?!\n";

    shared_vector<const uint32> tof_data(freeze(tof));
    shared_vector<const uint32> pixel_data(freeze(pixel));
    pvTimeOfFlight->replace(tof_data);
    pvPixel->replace(pixel_data);

    // Update timestamp
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
}

void NeutronPVRecord::process()
{
    generateFakeValues();
}

void NeutronPVRecord::destroy()
{
    // Stop the processing thread
    is_running = false;
    processing_done.wait(5.0);
    PVRecord::destroy();
}

}} // namespace neutronServer, epics
