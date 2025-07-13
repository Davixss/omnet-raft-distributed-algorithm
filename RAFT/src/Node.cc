#include <omnetpp.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <queue>
#include "data_types.h"

using namespace omnetpp;

class Node : public cSimpleModule
{
    private:
        int myId;
        int requestId;
        int numNodes;
        int term = 0;
        int votesReceived = 0;
        nodeState state = FOLLOWER;
        physicalState pState = AVAILABLE;

        std::map<int, float> nodeIdAndU;
        std::map<int, int> nodeIdToGateIndex;

        // Requests queue
        std::queue<ServiceRequest*> requestQueue;
        bool isRequestActive = false;

        int C_ram;
        int C_cpu;
        int W_ram;
        int W_cpu;
        int R_ram;
        int R_cpu;

        // Statistics
        int requestsArrived = 0;
        int requestsProcessed = 0;
        int messagesExchanged = 0;
        int electionWonFirstRound = 0;
        int totalElections = 0;
        std::queue<simtime_t> AverageTimeDeployService;

        // Time
        simtime_t t_start;
        simtime_t t_stop;

    protected:
        virtual void nextRoundMsg();
        virtual void readNextRequest();
        virtual int randint(int min, int max);
        virtual void sendElectionRequest(int requestId, int candidateId, int term, int R_ram, int R_cpu);
        virtual void sendElectionResponse(int requestId, int responderId, int candidateId, bool vote, float U);
        virtual void sendVoteRequestToFollowers(int requestId, int leaderId, int candidateServerId, int term);
        virtual void sendVoteResponseToLeader(int requestId, int leaderId, int candidateServerId, bool vote);
        virtual void sendDeploymentTaskToHostNode(int requestId, int leaderId, int hostNodeId, int R_ram, int R_cpu);
        virtual void resetVariables();
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

    public:
        Node();
        virtual ~Node();
};


Define_Module(Node);


Node::Node() {}
Node::~Node() {
    resetVariables();
}

void Node::initialize()
{
    srand(time(nullptr) + getId());
    myId = getIndex();
    numNodes = par("numNodes");

    // Reset variables
    resetVariables();

    // Generate the capacity of the node
    C_cpu = randint(45, 100);
    C_ram = randint(32, 512);
    W_cpu = randint(5, 10);
    W_ram = randint(1, 8);

    // Graphic disposal of the nodes
    for (int i=0; i<numNodes; i++)
    {
        int y_offset = randint(0, 200);
        Node *node = check_and_cast<Node*>(getModuleByPath(("node[" + std::to_string(i) + "]").c_str()));
        node->getDisplayString().setTagArg("p", 0, std::to_string(100+i*85).c_str());
        node->getDisplayString().setTagArg("p", 1, std::to_string(100+y_offset).c_str());
    }

    EV << "[Node " << myId << "] Resources: (C_cpu: " << C_cpu << ", C_ram: " << C_ram << ") | Workload: (W_cpu: " << W_cpu << ", W_ram: " << W_ram << ")" << endl;

    // Map nodeId → gateIndex (useful for dynamic links)
    int gateIndex = 0;
    for (int i = 0; i < gateSize("gate"); ++i)
    {
        cGate *outGate = gate("gate$o", i);
        if (outGate->isConnected())
        {
            cGate *nextGate = outGate->getNextGate();
            cModule *targetModule = nextGate->getOwnerModule();
            std::string fullPath = targetModule->getFullPath();

            if (targetModule->isSimple() && std::string(targetModule->getName()).find("node") != std::string::npos)
            {
                int nodeId = targetModule->getIndex();
                nodeIdToGateIndex[nodeId] = i;
            }
        }
    }

    WATCH(requestId);
    WATCH(term);
    WATCH(state);
    WATCH(pState);
    WATCH(requestsArrived);
    WATCH(requestsProcessed);
}

void Node::handleMessage(cMessage *msg)
{
    // When the precedent round advice me to run next round
    if (strcmp(msg->getName(), "NextRoundMsg") == 0)
    {
        if (dynamic_cast<NextRoundMsg*>(msg))
        {
            NextRoundMsg *nextRoundMsg = dynamic_cast<NextRoundMsg*>(msg);
            int destinationId = nextRoundMsg->getDestinationId();

            if (destinationId == myId) readNextRequest();
            delete nextRoundMsg;
        }
    }
    // A service request sent from the client
    else if (strcmp(msg->getName(), "ServiceRequest") == 0)
    {
        if (dynamic_cast<ServiceRequest*>(msg))
        {
            ServiceRequest *servreq = dynamic_cast<ServiceRequest*>(msg);
            int requestId = servreq->getRequestId();

            // Add to the queue
            requestQueue.push(servreq);

            // Statistics
            requestsArrived++;

            EV << "[Node " << myId << "] Queue length: " << requestQueue.size() << endl;

            // Only for the first time
            if (requestId == 1) readNextRequest();

        } else {
            EV_ERROR << "Dynamic_cast failed for ServiceRequest!" << endl;
            delete msg;
            return;
        }
    }
    else if (strcmp(msg->getName(), "ElectionRequest") == 0)
    {
        if (dynamic_cast<ElectionRequest*>(msg))
        {
            ElectionRequest *msgelereq = dynamic_cast<ElectionRequest*>(msg);
            int requestId = msgelereq->getRequestId();
            int candidateId = msgelereq->getCandidateId();
            int candidateTerm = msgelereq->getTerm();
            bool vote;
            float U = 0.1;

            R_ram = msgelereq->getR_ram();
            R_cpu = msgelereq->getR_cpu();

            if (state == FOLLOWER || state == CANDIDATE)
            {
                if (candidateTerm > term) {
                    term = candidateTerm;
                    vote = true;
                } else {
                    vote = false;
                }

                // Applying the formula
                if (R_ram != 0 && R_cpu != 0) {
                    // a. If the know the resources of the service request
                    U = U + ((R_cpu + W_cpu) / C_cpu) + ((R_ram + W_ram) / C_ram) / 1.01;
                } else {
                    // b. If the resources are not known
                    U = U + ((W_cpu / C_cpu) + (W_ram / C_ram)) / 1.01;
                }

                sendElectionResponse(requestId, myId, candidateId, vote, U);
            }

            delete msgelereq;

        } else {
            EV_ERROR << "Dynamic_cast failed for ElectionRequest!" << endl;
            delete msg;
            return;
        }
    }
    else if (strcmp(msg->getName(), "ElectionResponse") == 0)
    {
        if (dynamic_cast<ElectionResponse *>(msg))
        {
            ElectionResponse *msgeleresp = dynamic_cast<ElectionResponse*>(msg);
            int requestId = msgeleresp->getRequestId();
            int responderId = msgeleresp->getResponderId();
            int candidateId = msgeleresp->getCandidateId();
            bool vote = msgeleresp->getVote();
            float U = msgeleresp->getU();
            int minNodeId = -1; // Needed for first for loop check
            float minU = 100; // Needed for first for loop check

            if (candidateId == myId)
            {
                nodeIdAndU[responderId] = U;

                if (vote == true) votesReceived++;

                // If I received the votes from all the nodes (poll completed)
                if (votesReceived == numNodes - 1)
                {
                    // I vote myself
                    votesReceived++;
                }

                if (votesReceived >= (numNodes * 50 / 100) + 1) // 50% + 1
                {
                    EV << "{Request " << requestId << "} [Node " << myId << "] >> Now I'm the leader node! [Votes: " << votesReceived << "]" << endl;
                    state = LEADER; // I become leader!
                    votesReceived = 0; // Reset
                    electionWonFirstRound++;

                    // Iterating the map looking for min U associated to the nodeId
                    for (const auto& pair : nodeIdAndU) {
                        if (pair.second < minU) {
                            minNodeId = pair.first;
                            minU = pair.second;
                        }
                    }

                    // If the U calculated is <= 2.0 means that actual workload + request resources are less than total capacity (RAM + CPU)
                    if (minU <= 2.0) {
                        term++;
                        EV << "{Request " << requestId << "} [Node " << myId << "] >> I choose node[" << minNodeId << "] to host the service, with capacity(U): " << minU << endl;
                        sendVoteRequestToFollowers(requestId, myId, minNodeId, term);

                    // Otherwise means that the node with minimum U can't host the service because don't have enough RAM or CPU
                    } else {
                        EV_ERROR << "{Request " << requestId << "} [Node " << myId << "] >> The node (" << minNodeId << ") with minU (" << minU << ") calculated can't deploy the service because of low capacities (U<2)" << endl;

                        // Continue to the next request
                        isRequestActive = false;
                        nextRoundMsg();
                    }
                } else {
                    // If votation failed and all nodes replied, then cancel task and go to the next round
                    if (votesReceived == numNodes)
                    {
                        // Reset current state then next round
                        state = FOLLOWER;
                        votesReceived = 0;
                        nodeIdAndU.clear();
                        isRequestActive = false;
                        nextRoundMsg();
                    }
                }
            }

            delete msgeleresp;

        } else {
            EV_ERROR << "Dynamic_cast failed for ElectionResponse!" << endl;
            delete msg;
            return;
        }
    }
    else if (strcmp(msg->getName(), "VoteRequest") == 0)
    {
        if (dynamic_cast<VoteRequest *>(msg))
        {
            VoteRequest *msgvotereq = dynamic_cast<VoteRequest*>(msg);
            int requestId = msgvotereq->getRequestId();
            int leaderId = msgvotereq->getLeaderId();
            int candidateServerId = msgvotereq->getCandidateServerId();
            int leaderTerm = msgvotereq->getTerm();
            bool myVote;

            if (leaderTerm > term) {
                term = leaderTerm;
                myVote = true;
            } else {
                myVote = false;
            }

            sendVoteResponseToLeader(requestId, leaderId, candidateServerId, myVote);
            delete msgvotereq;

        } else {
            EV_ERROR << "Dynamic_cast failed for VoteRequest!" << endl;
            delete msg;
            return;
        }
    }
    else if (strcmp(msg->getName(), "VoteResponse") == 0)
    {
        if (dynamic_cast<VoteResponse *>(msg))
        {
            VoteResponse *msgvoteresp = dynamic_cast<VoteResponse*>(msg);
            int requestId = msgvoteresp->getRequestId();
            int leaderId = msgvoteresp->getLeaderId();
            int candidateServerId = msgvoteresp->getCandidateServerId();
            bool vote = msgvoteresp->getVote();

            if (state == LEADER && myId == leaderId)
            {

                if (vote == true) votesReceived++;
                if (votesReceived >= (numNodes * 50 / 100) + 1) // quorum = 50% + 1
                {
                    // Finally candidate node will deploy the service
                    state = FOLLOWER; // Not leader anymore
                    votesReceived = 0;
                    nodeIdAndU.clear();

                    // Statistics
                    requestsProcessed++;
                    t_stop = simTime();
                    simtime_t delta = t_stop - t_start;
                    AverageTimeDeployService.push(delta);

                    sendDeploymentTaskToHostNode(requestId, leaderId, candidateServerId, R_ram, R_cpu);
                }
            }

            delete msgvoteresp;

        } else {
            EV_ERROR << "Dynamic_cast failed for VoteResponse!" << endl;
            delete msg;
            return;
        }
    }
    else if (strcmp(msg->getName(), "DeployService") == 0)
    {
        if (dynamic_cast<DeployService *>(msg))
        {
            DeployService *msgdeploy = dynamic_cast<DeployService*>(msg);
            int requestId = msgdeploy->getRequestId();
            int hostServerId = msgdeploy->getHostServerId();
            int R_ram = msgdeploy->getR_ram();
            int R_cpu = msgdeploy->getR_cpu();

            // Checking if I am the will deploy the service
            if (hostServerId == myId)
            {
                // Increasing the current workload
                W_ram = W_ram + R_ram;
                W_cpu = W_cpu + R_cpu;

                EV << "{Request " << requestId << " COMPLETED} [Host Node " << myId << "] >> The service was deployed successfully! Actual workload [W_ram: " << W_ram << ", W_cpu: " << W_cpu << "]" << endl;

                isRequestActive = false;
                nextRoundMsg();
            }

            delete msgdeploy;

        } else {
            EV_ERROR << "Dynamic_cast failed for DeployService!" << endl;
            delete msg;
            return;
        }
    }

    // Statistics
    messagesExchanged++;
}


// --- Functions

void Node::nextRoundMsg()
{
    int destinationId;
    do {
        destinationId = intuniform(0, numNodes - 1);
    } while (destinationId == myId);

    NextRoundMsg *nextRoundMsg = new NextRoundMsg();
    nextRoundMsg->setDestinationId(destinationId);

    int gateIndex = nodeIdToGateIndex[destinationId];
    sendDelayed(nextRoundMsg, uniform(0.01, 0.05), "gate$o", gateIndex);
    // No delete needed, Omnet++ handle it
}

void Node::readNextRequest()
{
    // if (!requestQueue.empty() && isRequestActive == false)
    if (!requestQueue.empty())
    {
        //isRequestActive = true;

        // Get next request in queue
        ServiceRequest *servreq = requestQueue.front();
        requestId = servreq->getRequestId();
        R_ram = servreq->getR_ram();
        R_cpu = servreq->getR_cpu();

        term++;
        votesReceived = 0;
        state = CANDIDATE;

        // Statistics
        t_start = simTime();

        EV << "{Request " << requestId << " START} [Node " << myId << "] >> Service Deployment Received [RAM: " << R_ram << " | CPU: " << R_cpu << "]" << endl;
        sendElectionRequest(requestId, myId, term, R_ram, R_cpu);

        requestQueue.pop();
        delete servreq;

    } else {
        // I don't have any request in queue, then next round to other node
        nextRoundMsg();
    }
}

void Node::sendElectionRequest(int requestId, int candidateId, int term, int R_ram, int R_cpu)
{
    EV << "{Request " << requestId << "} [Node " << myId << "] >> Now I ran for leader" << endl;

    ElectionRequest *elereq = new ElectionRequest();
    elereq->setRequestId(requestId);
    elereq->setCandidateId(candidateId);
    elereq->setTerm(term);
    elereq->setR_ram(R_ram);
    elereq->setR_cpu(R_cpu);

    // Statistics
    totalElections++;

    int totalGates = gateSize("gate");

    // I send the message to all gates except to client (gate 0) and myself
    for (int gateIndex = 1; gateIndex < totalGates; ++gateIndex)
    {
        cGate *outGate = gate("gate$o", gateIndex);
        cGate *inGate = outGate->getNextGate();
        cModule *connectedModule = inGate->getOwnerModule();

        // If myself, skip
        if (connectedModule == this) continue;

        sendDelayed(elereq->dup(), uniform(0.01, 0.05), "gate$o", gateIndex);
    }

    delete elereq;
}

void Node::sendElectionResponse(int requestId, int responderId, int candidateId, bool vote, float U)
{
    ElectionResponse *eleresp = new ElectionResponse();
    eleresp->setRequestId(requestId);
    eleresp->setResponderId(responderId);
    eleresp->setCandidateId(candidateId);
    eleresp->setVote(vote);
    eleresp->setU(U);

    int gateIndex = nodeIdToGateIndex[candidateId];
    sendDelayed(eleresp, uniform(0.01, 0.05), "gate$o", gateIndex);

    EV << "{Request " << requestId << "} [Node " << myId << "] >> I voted! [candidateId: " << candidateId
       << ", vote: " << vote << ", capacity(U): " << U << "]" << endl;

    // No delete needed, Omnet++ handle it
}

void Node::sendVoteRequestToFollowers(int requestId, int leaderId, int candidateServerId, int term)
{
    EV << "{Request " << requestId << "} [Node " << myId << "] >> Asking votes to followers. Candidate to host the service is node[" << candidateServerId << "]" << endl;

    VoteRequest *votereq = new VoteRequest();
    votereq->setRequestId(requestId);
    votereq->setLeaderId(leaderId);
    votereq->setCandidateServerId(candidateServerId);
    votereq->setTerm(term);

    int totalGates = gateSize("gate");

    // I send the message to all gates except to client (gate 0) and myself
    for (int gateIndex = 1; gateIndex < totalGates; ++gateIndex)
    {
        cGate *outGate = gate("gate$o", gateIndex);
        cGate *inGate = outGate->getNextGate();
        cModule *connectedModule = inGate->getOwnerModule();

        // If myself, skip
        if (connectedModule == this) continue;

        sendDelayed(votereq->dup(), uniform(0.01, 0.05), "gate$o", gateIndex);
    }

    delete votereq;
}

void Node::sendVoteResponseToLeader(int requestId, int leaderId, int candidateServerId, bool vote)
{
    VoteResponse *voteresp = new VoteResponse();
    voteresp->setRequestId(requestId);
    voteresp->setLeaderId(leaderId);
    voteresp->setCandidateServerId(candidateServerId);
    voteresp->setVote(vote);

    int gateIndex = nodeIdToGateIndex[leaderId];
    sendDelayed(voteresp, uniform(0.01, 0.05), "gate$o", gateIndex);

    EV << "{Request " << requestId << "} [Node " << myId << "] >> I voted! [candidateServerId: " << candidateServerId << ", vote: " << vote << "]" << endl;
    // No delete needed, Omnet++ handle it
}

void Node::sendDeploymentTaskToHostNode(int requestId, int leaderId, int hostNodeId, int R_ram, int R_cpu)
{
    DeployService *deploy = new DeployService();
    deploy->setRequestId(requestId);
    deploy->setLeaderId(leaderId);
    deploy->setHostServerId(hostNodeId);
    deploy->setR_ram(R_ram);
    deploy->setR_cpu(R_cpu);

    int gateIndex = nodeIdToGateIndex[hostNodeId];
    sendDelayed(deploy, uniform(0.01, 0.05), "gate$o", gateIndex);

    EV << "{Request " << requestId << "} [Leader Node " << myId << "] >> Deployment task sent to host node " << endl;
    // No delete needed, Omnet++ handle it
}

void Node::resetVariables()
{
    term = 0;
    votesReceived = 0;
    state = FOLLOWER;
    pState = AVAILABLE;
    isRequestActive = false;

    nodeIdAndU.clear();
    nodeIdToGateIndex.clear();

    while (!requestQueue.empty()) {
        delete requestQueue.front();
        requestQueue.pop();
    }
}

void Node::finish()
{
    std::queue<simtime_t> copyQueue = AverageTimeDeployService;
    int ATDS_size = copyQueue.size();

    if (ATDS_size > 0)
    {
        std::vector<double> times;
        times.reserve(ATDS_size);

        simtime_t sum = 0;
        while (!copyQueue.empty()) {
            simtime_t t = copyQueue.front();
            sum += t;
            times.push_back(t.dbl());
            copyQueue.pop();
        }

        double mean = sum.dbl() / ATDS_size;

        // Variance
        double sq_diff_sum = 0.0;
        for (double v : times) {
            double diff = v - mean;
            sq_diff_sum += diff * diff;
        }

        double variance = (ATDS_size > 1) ? sq_diff_sum / (ATDS_size - 1) : 0.0;
        double stddev = std::sqrt(variance);

        // Confidence interval 95% (z=1.96 per n > 30)
        double z = 1.96; // t critic for more precision
        double margin = z * stddev / std::sqrt(ATDS_size);

        double ci_lower = mean - margin;
        double ci_upper = mean + margin;

        recordScalar("AverageTimeDeployService_Mean", mean);
        recordScalar("AverageTimeDeployService_StdDev", stddev);
        recordScalar("AverageTimeDeployService_CI_lower", ci_lower);
        recordScalar("AverageTimeDeployService_CI_upper", ci_upper);
    } else {
        recordScalar("AverageTimeDeployService_Mean", -1);
        recordScalar("AverageTimeDeployService_StdDev", -1);
        recordScalar("AverageTimeDeployService_CI_lower", -1);
        recordScalar("AverageTimeDeployService_CI_upper", -1);
    }

    recordScalar("RequestsArrived", requestsArrived);
    recordScalar("RequestsProcessed", requestsProcessed);
    recordScalar("MessagesExchanged", messagesExchanged);
    recordScalar("ElectionWonFirstRound", electionWonFirstRound);
    recordScalar("TotalElections", totalElections);
}

int Node::randint(int min, int max)
{
    return min + (rand() % (max - min + 1));
}
