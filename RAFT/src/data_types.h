using namespace omnetpp;

enum nodeState
{
    FOLLOWER = 0,
    CANDIDATE = 1,
    LEADER = 2
};

enum physicalState
{
    AVAILABLE = 0,
    DEAD = 1
};

class NextRoundMsg : public cMessage
{
    private:
        int destinationId;

    public:
        int getDestinationId() { return destinationId; }
        void setDestinationId(int destinationId) { this->destinationId = destinationId; }

        NextRoundMsg(const char *name = "NextRoundMsg") : cMessage(name) {}

        virtual NextRoundMsg *dup() const override {
            return new NextRoundMsg(*this);
        }
};

class ServiceRequest : public cMessage
{
    private:
        int requestId;
        int R_ram;
        int R_cpu;

    public:
        int getRequestId() { return requestId; }
        int getR_ram() { return R_ram; }
        int getR_cpu() { return R_cpu; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setR_ram(int R_ram) { this->R_ram = R_ram; }
        void setR_cpu(int R_cpu) { this->R_cpu = R_cpu; }

        ServiceRequest(const char *name = "ServiceRequest") : cMessage(name) {}

        virtual ServiceRequest *dup() const override {
            return new ServiceRequest(*this);
        }
};

class ElectionRequest : public cMessage
{
    private:
        int requestId;
        int candidateId;
        int term;
        int R_ram;
        int R_cpu;

    public:
        int getRequestId() { return requestId; }
        int getCandidateId() { return candidateId; }
        int getTerm() { return term; }
        int getR_ram() { return R_ram; }
        int getR_cpu() { return R_cpu; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setCandidateId(int candidateId) { this->candidateId = candidateId; }
        void setTerm(int term) { this->term = term; }
        void setR_ram(int R_ram) { this->R_ram = R_ram; }
        void setR_cpu(int R_cpu) { this->R_cpu = R_cpu; }

        ElectionRequest(const char *name = "ElectionRequest") : cMessage(name) {}

        virtual ElectionRequest *dup() const override {
            return new ElectionRequest(*this);
        }
};

class ElectionResponse : public cMessage
{
    private:
        int requestId;
        int responderId;
        int candidateId;
        bool vote;
        float U;

    public:
        int getRequestId() { return requestId; }
        int getResponderId() { return responderId; }
        int getCandidateId() { return candidateId; }
        bool getVote() { return vote; }
        float getU() { return U; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setResponderId(int responderId) { this->responderId = responderId; }
        void setCandidateId(int candidateId) { this->candidateId = candidateId; }
        void setVote(bool vote) { this->vote = vote; }
        void setU(float U) { this->U = U; }

        ElectionResponse(const char *name = "ElectionResponse") : cMessage(name) {}

        virtual ElectionResponse *dup() const override {
            return new ElectionResponse(*this);
        }
};

class VoteRequest : public cMessage
{
    private:
        int requestId;
        int leaderId;
        int candidateServerId;
        int term;

    public:
        int getRequestId() { return requestId; }
        int getLeaderId() { return leaderId; }
        int getCandidateServerId() { return candidateServerId; }
        int getTerm() { return term; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setLeaderId(int leaderId) { this->leaderId = leaderId; }
        void setCandidateServerId(int candidateServerId) { this->candidateServerId = candidateServerId; }
        void setTerm(int term) { this->term = term; }

        VoteRequest(const char *name = "VoteRequest") : cMessage(name) {}

        virtual VoteRequest *dup() const override {
            return new VoteRequest(*this);
        }
};

class VoteResponse : public cMessage
{
    private:
        int requestId;
        int leaderId;
        int candidateServerId;
        bool vote;

    public:
        int getRequestId() { return requestId; }
        int getLeaderId() { return leaderId; }
        int getCandidateServerId() { return candidateServerId; }
        bool getVote() { return vote; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setLeaderId(int leaderId) { this->leaderId = leaderId; }
        void setCandidateServerId(int candidateServerId) { this->candidateServerId = candidateServerId; }
        void setVote(int vote) { this->vote = vote; }

        VoteResponse(const char *name = "VoteResponse") : cMessage(name) {}

        virtual VoteResponse *dup() const override {
            return new VoteResponse(*this);
        }
};

class DeployService : public cMessage
{
    private:
        int requestId;
        int leaderId;
        int hostServerId;
        int R_ram;
        int R_cpu;

    public:
        int getRequestId() { return requestId; }
        int getLeaderId() { return leaderId; }
        int getHostServerId() { return hostServerId; }
        int getR_ram() { return R_ram; }
        int getR_cpu() { return R_cpu; }
        void setRequestId(int requestId) { this->requestId = requestId; }
        void setLeaderId(int leaderId) { this->leaderId = leaderId; }
        void setHostServerId(int hostServerId) { this->hostServerId = hostServerId; }
        void setR_ram(int R_ram) { this->R_ram = R_ram; }
        void setR_cpu(int R_cpu) { this->R_cpu = R_cpu; }

        DeployService(const char *name = "DeployService") : cMessage(name) {}

        virtual DeployService *dup() const override {
            return new DeployService(*this);
        }
};
