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

class MyChannelRequester : public ChannelRequester
{
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
    cout << getRequesterName()
         << " message (" << getMessageTypeName(messageType) << "): "
         << message << endl;
}

void MyChannelRequester::channelCreated(const epics::pvData::Status& status, Channel::shared_pointer const & channel)
{
    if (status.isOK())
    {
        message(channel->getChannelName() + " created, I am so happy!", infoMessage);
    }
    else
        message(status.getMessage(), errorMessage);
}

void MyChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState)
{
    message(channel->getChannelName() + " state: " + Channel::ConnectionStateNames[connectionState], infoMessage);
}


void monitor(string const &name, string const &request)
{
    ChannelProvider::shared_pointer channelProvider =
            getChannelProviderRegistry()->getProvider("pva");
    if (! channelProvider)
        THROW_EXCEPTION2( std::runtime_error, "No channel provider");

    ChannelRequester::shared_pointer channelRequester(new MyChannelRequester());

    channelProvider->createChannel(name, channelRequester, ChannelProvider::PRIORITY_DEFAULT);
//    PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest("field(value)");
//
//    shared_ptr<ChannelRequesterImpl> channelRequester(new ChannelRequesterImpl());
//    Channel::shared_pointer channel = provider->createChannel("neutron", channelRequester);

}

void listProviders()
{
    cout << "Available channel providers:" << endl;
    std::auto_ptr<ChannelProviderRegistry::stringVector_t> providers =
            getChannelProviderRegistry()->getProviderNames();
    for (size_t i=0; i<providers->size(); ++i)
        cout << (i+1) << ") " << providers->at(i) << endl;
}

int main()
{
    try
    {
        ClientFactory::start();
        listProviders();
        monitor("neutrons", "field(pixel)");
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
