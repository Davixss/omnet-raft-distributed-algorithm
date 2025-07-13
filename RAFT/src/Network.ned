package raft;

network Network
{
    parameters:
        int numNodes;
        int clientRequestsSec;

        @display("bgb=469,314");

    types:
        simple Client
        {
            parameters:
                int numNodes = parent.numNodes;
                int clientRequestsSec = parent.clientRequestsSec;
            gates:
                inout gate[];
        }
        simple Node
        {
            parameters:
                int numNodes = parent.numNodes;
                int clientRequestsSec = parent.clientRequestsSec;
            gates:
                inout gate[];
        }

    submodules:
        client: Client {
            @display("i=device/laptop;p=235,31");
        }
        node[numNodes]: Node {
            @display("i=device/server;p=100,230");
        }

    connections:
        // Connessioni bidirezionali client-nodo
        for i = 0..numNodes - 1 {
            client.gate++ <--> node[i].gate++;
        }
    
        // Connessioni bidirezionali tra i nodi (grafo completo)
        for i = 0..numNodes - 2, for j = i + 1..numNodes - 1 {
            node[i].gate++ <--> node[j].gate++;
        }

}
