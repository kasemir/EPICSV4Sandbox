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

using namespace std;
using namespace std::tr1;
using namespace epics::pvData;
using namespace epics::pvAccess;

// -- Requester Helper -----------------------------------------------------------------
static void messageHelper(Requester &requester, string const & message, MessageType messageType)
{
    cout << requester.getRequesterName()
         << " message (" << getMessageTypeName(messageType) << "): "
         << message << endl;
}

// -- ChannelRequester -----------------------------------------------------------------
class MyChannelRequester : public ChannelRequester
{
public:
    MyChannelRequester()
    {
        cout << "MyChannelRequester" << endl;
    }

    ~MyChannelRequester()
    {
        cout << "~MyChannelRequester" << endl;
    }

    string getRequesterName()
    {   return "MyChannelRequester";  }

    void message(string const & message,MessageType messageType)
    {   messageHelper(*this, message, messageType); }

    void channelCreated(const Status& status, Channel::shared_pointer const & channel);

    void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);

    boolean waitUntilConnected(double timeOut)
    {
        return connect_event.wait(timeOut);
    }

private:
    Event connect_event;
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

// -- ChannelGetRequester -----------------------------------------------------------------
class MyChannelGetRequester : public ChannelGetRequester
{
public:
    MyChannelGetRequester()
    {
        cout << "MyChannelGetRequester" << endl;
    }

    ~MyChannelGetRequester()
    {
        cout << "~MyChannelGetRequester" << endl;
    }

    string getRequesterName()
    {   return "MyChannelGetRequester";  }

    void message(string const & message,MessageType messageType)
    {   messageHelper(*this, message, messageType); }

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

private:
    Event done_event;
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

// --------------------------------------------------------------------------
void getValue(string const &name, string const &request, double timeout)
{
    ChannelProvider::shared_pointer channelProvider =
            getChannelProviderRegistry()->getProvider("pva");
    if (! channelProvider)
        THROW_EXCEPTION2(std::runtime_error, "No channel provider");

    shared_ptr<MyChannelRequester> channelRequester(new MyChannelRequester());
    shared_ptr<Channel> channel(channelProvider->createChannel(name, channelRequester));
    channelRequester->waitUntilConnected(timeout);

    shared_ptr<MyChannelGetRequester> channelGetRequester(new MyChannelGetRequester());
    PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);

    // This took me 3 hours to figure out:
    shared_ptr<ChannelGet> channelGet = channel->createChannelGet(channelGetRequester, pvRequest);
    // We don't care about the value of 'channelGet', so why assign it to a variable?
    // But when we _don't_ assign it to a shared_ptr<>, the one
    // returned from channel->createChannelGet() will be deleted
    // right away, and then the server(!) crashes because it receives a NULL GetRequester...

    channelGetRequester->waitUntilDone(timeout);
}

static void help(const char *name)
{
    cout << "USAGE: " << name << " [options] [channel]" << endl;
    cout << "  -h        : Help" << endl;
    cout << "  -r request: Request" << endl;
    cout << "  -w seconds: Wait timeout" << endl;
}

int main(int argc,char *argv[])
{
    string channel = "neutrons";
    string request = "field()";
    double timeout = 2.0;

    int opt;
    while ((opt = getopt(argc, argv, "r:w:h")) != -1)
    {
        switch (opt)
        {
        case 'r':
            request = optarg;
            break;
        case 'w':
            timeout = atof(optarg);
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

    cout << "Channel: " << channel << endl;
    cout << "Request: " << request << endl;
    cout << "Wait:    " << timeout << " sec" << endl;

    try
    {
        ClientFactory::start();
        getValue(channel, request, timeout);
        ClientFactory::stop();
    }
    catch (std::exception &ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.what());
        PRINT_EXCEPTION2(ex, stderr);
        cout << SHOW_EXCEPTION(ex);
    }

    return 0;
}
