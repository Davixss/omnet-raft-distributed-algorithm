# omnet-raft-distributed-algorithm
University of Messina (www.unime.it) <br>
A.Y. 2024/2025 <br>
Advanced Algorithms and Computational Models Project <br>
Prof: Maria Fazio (maria.fazio@unime.it) <br>

Modeling a distributed algorithm RAFT-like in Omnet++ simulator in C++

# Project description
The aim of the project is to model a cluster of nodes of a dynamic
distributed system, in such a way that at each simulation the number of
nodes is variable. A cluster in the context of computing and distributed
systems is a collection of interconnected computers (also known as nodes)
that work together as a single cohesive system. These nodes communicate
and coordinate their actions by passing messages, allowing them to share
resources and distribute workloads efficiently.
Finally, a RAFT-like consensus algorithm (similar to RAFT behavior but
not RAFT) is modeled and implemented in the node cluster to elect a
leader node that handles a request and obtain consensus for service de-
ployment.

# Project features
- Omnet++ 6
- RAFT-like distributed algorithm
- Dynamic network with 1 client and n nodes
- Read simulation plot and data results with Python

# Configuration
- Download Omnet++ 6
- Install all reccomended libraries
- Open "samples" folder in Omnet++
- Import RAFT folder from this repository
- Clear the project
- Compile the project
- Run simulation
