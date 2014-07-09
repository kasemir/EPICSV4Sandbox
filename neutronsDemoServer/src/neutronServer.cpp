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
    while (true)
    {
        epicsThreadSleep(me->delay);
        me->lock();
        try
        {
            ++packets;
            me->beginGroupPut();
            me->process();
            me->endGroupPut();
            // Every 10 second, show how many updates we generated so far
            if (++loops >= loops_in_10_seconds)
            {
                loops = 0;
                std::cout << packets << " packets\n";
            }
        }
        catch(...)
        {
            me->unlock();
            throw;
        }
        me->unlock();
    }
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
            ->addNestedStructure("pulse")
                ->setId("uri:ev4:nt/2012/pwd:NTScalar")
                ->add("value", pvULong)
                ->add("protonCharge", pvDouble)
                ->add("timeStamp", standardField->timeStamp())
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
: PVRecord(recordName,pvStructure), delay(delay), event_count(event_count)
{
}

NeutronPVRecord::~NeutronPVRecord()
{
}

bool NeutronPVRecord::init()
{
    initPVRecord();

    // Fetch pointers into the records pvData which will be used to update the values
    pvPulseID = getPVStructure()->getULongField("pulse.value");
    if (pvPulseID.get() == NULL)
        return false;

    pvProtonCharge = getPVStructure()->getDoubleField("pulse.protonCharge");
    if (pvProtonCharge.get() == NULL)
        return false;

    if (!pvTimeStamp.attach(getPVStructure()->getSubField("pulse.timeStamp")))
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

    // Vary a fake 'charge', somewhat based on the changing ID
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
    pvTimeOfFlight->replace(freeze(tof));
    pvPixel->replace(freeze(pixel));

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
    PVRecord::destroy();
}


}}
