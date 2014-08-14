/* neutronServer.cpp
 *
 * Copyright (c) 2014 Oak Ridge National Laboratory.
 * All rights reserved.
 * See file LICENSE that is included with this distribution.
 *
 * @author Kay Kasemir
 */
#include <iostream>

#include <epicsThread.h>
#include <pv/epicsException.h>
#include <pv/createRequest.h>
#include <pv/event.h>
#include <pv/pvData.h>
#include <pv/clientFactory.h>
#include <pv/pvAccess.h>
#include <pv/monitor.h>

using namespace std;
using namespace std::tr1;
using namespace epics::pvData;
using namespace epics::pvAccess;

/** Requester implementation,
 *  used as base for all the following *Requester
 */
class MyRequester : public virtual Requester
{
    string requester_name;
public:
    MyRequester(string const &requester_name)
    : requester_name(requester_name)
    {}

    string getRequesterName()
    {   return requester_name; }

    void message(string const & message, MessageType messageType);
};

void MyRequester::message(string const & message, MessageType messageType)
{
    cout << getMessageTypeName(messageType) << ": "
         << requester_name << " "
         << message << endl;
}

/** Requester for channel and status updates */
class MyChannelRequester : public virtual MyRequester, public virtual ChannelRequester
{
    Event connect_event;
public:
    MyChannelRequester() : MyRequester("MyChannelRequester")
    {}

    void channelCreated(const Status& status, Channel::shared_pointer const & channel);
    void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);

    boolean waitUntilConnected(double timeOut)
    {
        return connect_event.wait(timeOut);
    }
};

void MyChannelRequester::channelCreated(const Status& status, Channel::shared_pointer const & channel)
{
    cout << channel->getChannelName() << " created, " << status << endl;
}

void MyChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState)
{
    cout << channel->getChannelName() << " state: "
         << Channel::ConnectionStateNames[connectionState]
         << " (" << connectionState << ")" << endl;
    if (connectionState == Channel::CONNECTED)
        connect_event.signal();
}

/** Requester for 'getting' a single value */
class MyChannelGetRequester : public virtual MyRequester, public virtual ChannelGetRequester
{
    Event done_event;
public:
    MyChannelGetRequester() : MyRequester("MyChannelGetRequester")
    {}

    void channelGetConnect(const Status& status,
            ChannelGet::shared_pointer const & channelGet,
            Structure::const_shared_pointer const & structure);

    void getDone(const Status& status,
            ChannelGet::shared_pointer const & channelGet,
            PVStructure::shared_pointer const & pvStructure,
            BitSet::shared_pointer const & bitSet);

    boolean waitUntilDone(double timeOut)
    {
        return done_event.wait(timeOut);
    }
};

void MyChannelGetRequester::channelGetConnect(const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        Structure::const_shared_pointer const & structure)
{
    // Could inspect or memorize the channel's structure...
    if (status.isSuccess())
    {
        cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
             << " connected, " << status << endl;
        structure->dump(cout);

        channelGet->get();
    }
    else
    {
        cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
             << " problem, " << status << endl;
        done_event.signal();
    }
}

void MyChannelGetRequester::getDone(const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        PVStructure::shared_pointer const & pvStructure,
        BitSet::shared_pointer const & bitSet)
{
    cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
         << " finished, " << status << endl;

    if (status.isSuccess())
    {
        pvStructure->dumpValue(cout);
        done_event.signal();
    }
}

/** Requester for 'monitoring' value changes of a channel */
class MyMonitorRequester : public virtual MyRequester, public virtual MonitorRequester
{
    bool quiet;
    Event done_event;

    size_t value_offset;
    uint64 updates;
    uint64 last_pulse_id;
    uint64 missing_pulses;

    void checkUpdate(shared_ptr<PVStructure> const &structure);
public:
    MyMonitorRequester(bool quiet)
    : MyRequester("MyMonitorRequester"), quiet(quiet),
      value_offset(-1), updates(0), last_pulse_id(0), missing_pulses(0)
    {}

    void monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure);
    void monitorEvent(MonitorPtr const & monitor);
    void unlisten(MonitorPtr const & monitor);

    boolean waitUntilDone()
    {
        return done_event.wait();
    }
};

void MyMonitorRequester::checkUpdate(shared_ptr<PVStructure> const &pvStructure)
{
    if (value_offset == (size_t)-1)
        value_offset = pvStructure->getULongField("pulse.value")->getFieldOffset();
    shared_ptr<PVULong> value = dynamic_pointer_cast<PVULong>(pvStructure->getSubField(value_offset));

//    shared_ptr<PVULong> value = pvStructure->getULongField("pulse.value");
    if (! value)
    {
        cout << "No 'pulse.value'" << endl;
        return;
    }

    uint64 pulse_id = value->get();
    if (last_pulse_id != 0)
    {
        int missing = pulse_id - 1 - last_pulse_id;
        if (missing > 0)
            missing_pulses += missing;
    }
    last_pulse_id = pulse_id;
}

void MyMonitorRequester::monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure)
{
    cout << "Monitor connects, " << status << endl;
    if (status.isSuccess())
    {
        // Check the structure
        // TODO Remember the structure, obtain pulse.value offset from introspection API instead of pvValue API?
        StructureConstPtr pulse = dynamic_pointer_cast<const Structure>(structure->getField("pulse"));
        if (! pulse)
        {
            cout << "No 'pulse' structure" << endl;
            return;
        }
        shared_ptr<const Scalar> value = dynamic_pointer_cast<const Scalar>(pulse->getField("value"));
        if (! value  &&  value->getScalarType() == pvULong)
        {
            cout << "No 'pulse.value' ULong" << endl;
            return;
        }

        monitor->start();
    }
}

void MyMonitorRequester::monitorEvent(MonitorPtr const & monitor)
{
    shared_ptr<MonitorElement> update;
    while ((update = monitor->poll()))
    {
        ++updates;
        checkUpdate(update->pvStructurePtr);
        if (quiet)
        {
            if ((updates % 1000) == 0)
            {
                size_t expected = updates + missing_pulses;
                double received_perc = 100.0 * (expected - missing_pulses) / expected;
                cout << updates << " updates, " << missing_pulses << " missing pulses, received " << fixed << setprecision(1) << received_perc << "%" << endl;
                missing_pulses = 0;
                updates = 0;
            }
        }
        else
        {
            cout << "Monitor:\n";

            cout << "Changed: " << *update->changedBitSet.get() << endl;
            cout << "Overrun: " << *update->overrunBitSet.get() << endl;

            update->pvStructurePtr->dumpValue(cout);
            cout << endl;
        }
        monitor->release(update);
    }
}

void MyMonitorRequester::unlisten(MonitorPtr const & monitor)
{
    cout << "Monitor unlistens" << endl;
}


/** Connect, get value, disconnect */
void getValue(string const &name, string const &request, double timeout)
{
    ChannelProvider::shared_pointer channelProvider =
            getChannelProviderRegistry()->getProvider("pva");
    if (! channelProvider)
        THROW_EXCEPTION2(runtime_error, "No channel provider");

    shared_ptr<MyChannelRequester> channelRequester(new MyChannelRequester());
    shared_ptr<Channel> channel(channelProvider->createChannel(name, channelRequester));
    channelRequester->waitUntilConnected(timeout);

    shared_ptr<PVStructure> pvRequest = CreateRequest::create()->createRequest(request);
    shared_ptr<MyChannelGetRequester> channelGetRequester(new MyChannelGetRequester());

    // This took me 3 hours to figure out:
    shared_ptr<ChannelGet> channelGet = channel->createChannelGet(channelGetRequester, pvRequest);
    // We don't care about the value of 'channelGet', so why assign it to a variable?
    // But when we _don't_ assign it to a shared_ptr<>, the one
    // returned from channel->createChannelGet() will be deleted
    // right away, and then the server(!) crashes because it receives a NULL GetRequester...

    channelGetRequester->waitUntilDone(timeout);
}

/** Monitor values */
void doMonitor(string const &name, string const &request, double timeout, short priority, bool quiet)
{
    ChannelProvider::shared_pointer channelProvider =
            getChannelProviderRegistry()->getProvider("pva");
    if (! channelProvider)
        THROW_EXCEPTION2(runtime_error, "No channel provider");

    shared_ptr<MyChannelRequester> channelRequester(new MyChannelRequester());
    shared_ptr<Channel> channel(channelProvider->createChannel(name, channelRequester, priority));
    channelRequester->waitUntilConnected(timeout);

    shared_ptr<PVStructure> pvRequest = CreateRequest::create()->createRequest(request);
    shared_ptr<MyMonitorRequester> monitorRequester(new MyMonitorRequester(quiet));

    shared_ptr<Monitor> monitor = channel->createMonitor(monitorRequester, pvRequest);

    // Wait forever..
    monitorRequester->waitUntilDone();
}


static void help(const char *name)
{
    cout << "USAGE: " << name << " [options] [channel]" << endl;
    cout << "  -h         : Help" << endl;
    cout << "  -m         : Monitor instead of get" << endl;
    cout << "  -q         : .. quietly monitor, don't print data" << endl;
    cout << "  -r request : Request" << endl;
    cout << "  -w seconds : Wait timeout" << endl;
    cout << "  -p priority: Priority, 0..99, default 0" << endl;
}

int main(int argc,char *argv[])
{
    string channel = "neutrons";
    string request = "record[queueSize=100]field()";
    double timeout = 2.0;
    bool monitor = false;
    bool quiet = false;
    short priority = ChannelProvider::PRIORITY_DEFAULT;

    int opt;
    while ((opt = getopt(argc, argv, "r:w:p:mqh")) != -1)
    {
        switch (opt)
        {
        case 'r':
            request = optarg;
            break;
        case 'w':
            timeout = atof(optarg);
            break;
        case 'p':
            priority = atoi(optarg);
            break;
        case 'm':
            monitor = true;
            break;
        case 'q':
            quiet = true;
            break;
        case 'h':
            help(argv[0]);
            return 0;
        default:
            help(argv[0]);
            return -1;
        }
    }
    if (optind < argc)
        channel = argv[optind];

    cout << "Channel:  " << channel << endl;
    cout << "Request:  " << request << endl;
    cout << "Wait:     " << timeout << " sec" << endl;
    cout << "Priority: " << priority << endl;

    try
    {
        ClientFactory::start();
        if (monitor)
            doMonitor(channel, request, timeout, priority, quiet);
        else
            getValue(channel, request, timeout);
        ClientFactory::stop();
    }
    catch (exception &ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.what());
        PRINT_EXCEPTION2(ex, stderr);
        cout << SHOW_EXCEPTION(ex);
    }

    return 0;
}
