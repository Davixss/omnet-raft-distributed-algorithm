import pprint as pp
import matplotlib.pyplot as plt
import math

aggregated_stats = {}

def process_file(fname: str) -> None:
    par = {}
    nodes = {}
    scalarNodesFile = {}

    try:
        with open(fname, "r", encoding="utf-8") as file:
            for line in file.readlines():

                if fname not in scalarNodesFile:
                    scalarNodesFile[fname] = {}

                if line.startswith("par Network.client"):
                    lineElements = line.split()
                    if 'numNodes' not in par and lineElements[2] == 'numNodes':
                        par['numNodes'] = int(lineElements[3])
                    if 'clientRequestsSec' not in par and lineElements[2] == 'clientRequestsSec':
                        par['clientRequestsSec'] = int(lineElements[3])

                if line.startswith("scalar Network.node["):
                    lineElements = line.split()
                    node = int(lineElements[1].replace("Network.node[", "").replace("]", ""))   
                    param = lineElements[2]        
                    value = lineElements[3]

                    if node not in nodes:
                        nodes[node] = {}

                    if param == 'RequestsArrived':
                        nodes[node][param] = int(value)
                    elif param == 'RequestsProcessed':
                        nodes[node][param] = int(value)
                    elif param == 'MessagesExchanged':
                        nodes[node][param] = int(value) 
                    elif param == 'ElectionWonFirstRound':
                        nodes[node][param] = int(value) 
                    elif param == 'TotalElections':
                        nodes[node][param] = int(value) 
                    elif param == 'AverageTimeDeployService_Mean':
                        nodes[node][param] = round(float(value), 5)
                    elif param == 'AverageTimeDeployService_CI_lower':
                        nodes[node][param] = round(float(value), 5)
                    elif param == 'AverageTimeDeployService_CI_upper':
                        nodes[node][param] = round(float(value), 5)

        totalRequestsArrived = sum(node.get('RequestsArrived', 0) for node in nodes.values())
        totalRequestsProcessed = sum(node.get('RequestsProcessed', 0) for node in nodes.values())
        messagesExchanged = sum(node.get('MessagesExchanged', 0) for node in nodes.values())
        totalElections = sum(node.get('TotalElections', 0) for node in nodes.values())
        totalWonFirstRound = sum(node.get('ElectionWonFirstRound', 0) for node in nodes.values())

        electionWonFirstRoundPercentage = (totalWonFirstRound / totalElections) * 100 if totalElections > 0 else 0
        handlingRequestsEfficiencyPercentage = (totalRequestsProcessed / totalRequestsArrived) * 100 if totalRequestsArrived > 0 else 0

        print()
        print(f"=== Statistics from: {fname} ===")
        print()
        print(f"Nodes: {par['numNodes']}")
        print(f"Requests/s: {par['clientRequestsSec']}")
        print(f"Messages exchanged: {messagesExchanged}")
        print(f"Handling requests efficiency: {handlingRequestsEfficiencyPercentage:.2f}% ({totalRequestsProcessed}/{totalRequestsArrived})")
        print(f"Elections won first round: {electionWonFirstRoundPercentage:.2f}% ({totalWonFirstRound}/{totalElections})")
        print()
        pp.pprint(nodes)

        config_key = fname.split('_')[1] if '_' in fname else fname

        avg_times = [node.get('AverageTimeDeployService_Mean', 0) for node in nodes.values() if node.get('AverageTimeDeployService_Mean', 0) > 0]
        ci_uppers = [node.get('AverageTimeDeployService_CI_upper', 0) for node in nodes.values() if node.get('AverageTimeDeployService_CI_upper', 0) > 0]
        ci_lowers = [node.get('AverageTimeDeployService_CI_lower', 0) for node in nodes.values() if node.get('AverageTimeDeployService_CI_lower', 0) > 0]

        if avg_times and ci_uppers and ci_lowers:
            avg = round(sum(avg_times) / len(avg_times), 5)
            ci_margin = round((sum(ci_uppers) - sum(avg_times)) / len(avg_times), 5)
        else:
            avg = -1.0
            ci_margin = 0.0

        aggregated_stats[config_key] = {
            "RequestsArrived": totalRequestsArrived,
            "RequestsProcessed": totalRequestsProcessed,
            "MessagesExchanged": messagesExchanged,
            "TotalElections": totalElections,
            "WonFirstRound": totalWonFirstRound,
            "AvgTimeDeploy": avg,
            "AvgTimeDeploy_CI": ci_margin
        }

    except FileNotFoundError:
        print(f">> File '{fname}' not found.")
    except Exception as e:
        print(f">> Read error '{fname}': {e}")

# === MAIN ===

filename = str(input("Write file name (scalar): "))
typeOpen = str(input("Would you open only this file or every 1/5/10 req files? [this-all]: "))

if typeOpen == 'this':
    process_file(filename)
else:
    base_name = filename.split('_')[0]  
    suffixes = ["_1Req-#0.sca", "_5Req-#0.sca", "_10Req-#0.sca"]
    for suffix in suffixes:
        full_name = base_name + suffix
        process_file(full_name)

# === PLOTS ===

def confidence_interval(p: float, n: int, z: float = 1.96) -> float:
    n = int(n)
    if n <= 0:
        return 0.0
    p = max(0.0, min(100.0, p))
    p = p / 100.0
    se = math.sqrt((p * (1 - p)) / n)
    return z * se * 100

if aggregated_stats:
    labels = list(aggregated_stats.keys())

    messages = [aggregated_stats[k]["MessagesExchanged"] for k in labels]
    avg_time = [aggregated_stats[k]["AvgTimeDeploy"] for k in labels]
    avg_time_ci = [aggregated_stats[k]["AvgTimeDeploy_CI"] for k in labels]

    handlingRequestsEfficiency_pct = []
    handlingRequestsEfficiency_ci = []

    election_pct = []
    election_ci = []

    for k in labels:
        arrived = aggregated_stats[k]["RequestsArrived"]
        processed = aggregated_stats[k]["RequestsProcessed"]
        totalElections = aggregated_stats[k]["TotalElections"]
        won = aggregated_stats[k]["WonFirstRound"]

        efficiency = (processed / arrived) * 100 if arrived > 0 else 0
        efficiency_ci = confidence_interval(efficiency, arrived)
        handlingRequestsEfficiency_pct.append(efficiency)
        handlingRequestsEfficiency_ci.append(efficiency_ci)

        won_pct = (won / totalElections) * 100 if totalElections > 0 else 0
        won_ci = confidence_interval(won_pct, totalElections)
        election_pct.append(won_pct)
        election_ci.append(won_ci)

    fig, axs = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle("Omnet++ simulations statistics (RAFT-like)", fontsize=16)

    axs[0, 0].bar(labels, handlingRequestsEfficiency_pct, color="steelblue", yerr=handlingRequestsEfficiency_ci, capsize=5)
    axs[0, 0].set_title("Handling requests efficiency (%)")
    axs[0, 0].set_ylim(0, 110)
    axs[0, 0].set_ylabel("% processed requests")

    axs[0, 1].bar(labels, election_pct, color="mediumseagreen", yerr=election_ci, capsize=5)
    axs[0, 1].set_title("Elections won first round (%)")
    axs[0, 1].set_ylim(0, 110)
    axs[0, 1].set_ylabel("% elections won")

    axs[1, 0].bar(labels, messages, color="salmon")
    axs[1, 0].set_title("Messages exchanged (n)")
    axs[1, 0].set_ylabel("Total messages")

    axs[1, 1].bar(labels, avg_time, color="orange", yerr=avg_time_ci, capsize=5)
    axs[1, 1].set_title("Service deploy average time (s)")
    axs[1, 1].set_ylabel("Seconds")

    for ax in axs.flat:
        ax.set_xlabel("Simulation configuration")

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.show()
else:
    print(">> Any statistics available")
