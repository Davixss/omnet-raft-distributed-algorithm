#include <omnetpp.h>
#include <cstdlib>
#include <ctime>
#include <queue>
#include "data_types.h"

using namespace omnetpp;

class Client : public cSimpleModule
{
    private:
        int numNodes;
        int clientRequestsSec;
        int requestId = 1;
        cMessage *sendRequestEvent;

    protected:
        int randint(int min, int max);
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void sendNextRequest();

    public:
        Client();
        virtual ~Client();
};

Define_Module(Client);

Client::Client() {
    sendRequestEvent = nullptr;
}

Client::~Client() {
    cancelAndDelete(sendRequestEvent);
}

void Client::initialize()
{
    srand(time(nullptr) + getId());
    numNodes = par("numNodes");
    clientRequestsSec = par("clientRequestsSec");

    sendRequestEvent = new cMessage("sendRequestEvent");

    // Start scheduling first request
    scheduleAt(simTime() + (1.0 / clientRequestsSec), sendRequestEvent);
}

void Client::handleMessage(cMessage *msg)
{
    if (msg == sendRequestEvent)
    {
        sendNextRequest();

        // Re-schedule next request
        scheduleAt(simTime() + (1.0 / clientRequestsSec), sendRequestEvent);
    }
    else
    {
        delete msg;
    }
}

void Client::sendNextRequest()
{
    ServiceRequest *request = new ServiceRequest("ServiceRequest");
    request->setRequestId(requestId);
    request->setR_ram(randint(1, 2));
    request->setR_cpu(randint(2, 9));

    if (numNodes > 0)
    {
        int randomIndex = randint(0, numNodes - 1);
        send(request, "gate$o", randomIndex);
        EV << "[Client] Request[" << requestId << "] sent to node[" << randomIndex << "]" << endl;
        requestId++;
    }
    else
    {
        EV_ERROR << "[Client] Any node available. Verify numNodes > 0." << endl;
        delete request;
    }
}

int Client::randint(int min, int max) {
    return min + (rand() % (max - min + 1));
}
