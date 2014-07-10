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
#include <pv/pvData.h>
#include <pv/clientFactory.h>
#include <pv/pvAccess.h>

using namespace std;
using namespace std::tr1;
using namespace epics::pvData;
using namespace epics::pvAccess;

// -- Requester Helper -----------------------------------------------------------------

static void messageHelper(Requester &requester, std::string const & message, MessageType messageType)
{
    cout << requester.getRequesterName()
         << " message (" << getMessageTypeName(messageType) << "): "
         << message << endl;
}

// -- ChannelRequester -----------------------------------------------------------------

class MyChannelRequester : public ChannelRequester
{
public:
    std::string getRequesterName();
    void message(std::string const & message,MessageType messageType);
    void channelCreated(const epics::pvData::Status& status, Channel::shared_pointer const & channel);
    void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);
};

std::string MyChannelRequester::getRequesterName()
{
    return "MyChannelRequester";
}

void MyChannelRequester::message(std::string const & message, MessageType messageType)
{
    messageHelper(*this, message, messageType);
}

void MyChannelRequester::channelCreated(const epics::pvData::Status& status, Channel::shared_pointer const & channel)
{
    cout << channel->getChannelName() << " created, " << status << endl;
}

void MyChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState)
{
    cout << channel->getChannelName() << " state: "
         << Channel::ConnectionStateNames[connectionState]
         << " (" << connectionState << ")" << endl;
}

// -- ChannelGetRequester -----------------------------------------------------------------
class MyChannelGetRequester : public ChannelGetRequester
{
public:
    std::string getRequesterName();
    void message(std::string const & message,MessageType messageType);
    void channelGetConnect(const epics::pvData::Status& status,
            ChannelGet::shared_pointer const & channelGet,
            epics::pvData::Structure::const_shared_pointer const & structure);
    void getDone(const epics::pvData::Status& status,
            ChannelGet::shared_pointer const & channelGet,
            epics::pvData::PVStructure::shared_pointer const & pvStructure,
            epics::pvData::BitSet::shared_pointer const & bitSet);
};

std::string MyChannelGetRequester::getRequesterName()
{
    return "MyChannelGetRequester";
}

void MyChannelGetRequester::message(std::string const & message, MessageType messageType)
{
    messageHelper(*this, message, messageType);
}

void MyChannelGetRequester::channelGetConnect(const epics::pvData::Status& status,
        ChannelGet::shared_pointer const & channelGet,
        epics::pvData::Structure::const_shared_pointer const & structure)
{
    // Could inspect or memorize the channel's structure...
    if (status.isSuccess())
    {
        cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
             << " connected, " << status << endl;
        cout << "Channel structure:" << endl;
        structure->dump(cout);

        channelGet->get();
    }
    else
        cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
             << " problem, " << status << endl;
}

void MyChannelGetRequester::getDone(const epics::pvData::Status& status,
        ChannelGet::shared_pointer const & channelGet,
        epics::pvData::PVStructure::shared_pointer const & pvStructure,
        epics::pvData::BitSet::shared_pointer const & bitSet)
{
    cout << "ChannelGet for " << channelGet->getChannel()->getChannelName()
         << " finished, " << status << endl;

    if (status.isSuccess())
        pvStructure->dumpValue(cout);
}

// -- Stuff -----------------------------------------------------------------

void monitor(string const &name, string const &request)
{
    ChannelProvider::shared_pointer channelProvider =
            getChannelProviderRegistry()->getProvider("pva");
    if (! channelProvider)
        THROW_EXCEPTION2( std::runtime_error, "No channel provider");

    ChannelRequester::shared_pointer channelRequester(new MyChannelRequester());
    Channel::shared_pointer channel = channelProvider->createChannel(name, channelRequester, ChannelProvider::PRIORITY_DEFAULT);

    ChannelGetRequester::shared_pointer channelGetRequester(new MyChannelGetRequester());
    PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);
    channel->createChannelGet(channelGetRequester, pvRequest);

    // TODO: Should wait for things to happen
}

void listProviders()
{
    cout << "Available channel providers:" << endl;
    std::auto_ptr<ChannelProviderRegistry::stringVector_t> providers =
            getChannelProviderRegistry()->getProviderNames();
    for (size_t i=0; i<providers->size(); ++i)
        cout << (i+1) << ") " << providers->at(i) << endl;
}

static void help(const char *name)
{
    cout << "USAGE: " << name << " [options] [channel]" << endl;
    cout << "  -h        : Help" << endl;
    cout << "  -r request: Request" << endl;
}

int main(int argc,char *argv[])
{
    string channel = "neutrons";
    string request = "field(pulse,time_of_flight,pixel)";

    int opt;
    while ((opt = getopt(argc, argv, "r:h")) != -1)
    {
        switch (opt)
        {
        case 'r':
            request = optarg;
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

    try
    {
        ClientFactory::start();
        listProviders();
        monitor(channel, request);
        epicsThreadSleep(5.0);
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
