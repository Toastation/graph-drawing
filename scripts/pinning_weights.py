from tulip import tlp

NEIGHBOR_INFLUENCE = 0.6    # [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement

def compute_positioning_score(graph):
    # TODO: find a way to determine which node is new and which node had one of its neighbor removed.
    # or find a way to get the previous layout of the graph.
    pass

def compute_pinning_weights(graph, neighbor_influence):
    positioning_scores = graph.getDoubleProperty("positioningScore")
    pinning_weights = graph.getDoubleProperty("pinningWeight")
    
    neighbor_influence_complement = 1 - neighbor_influence
    
    # local pass
    for n in graph.getNodes():
        neighbor_sum = 0
        for neighbor in graph.getInOutNodes(n):
            neighbor_sum += positioning_scores[neighbor]
        pinning_weights[n] = neighbor_influence * positioning_scores[n] * neighbor_influence_complement * (1 / graph.deg(n)) * neighbor_sum

    # global pass
    # TODO: need the previous layout info to determine the first set


def run(graph):
    compute_positioning_score(graph)
    compute_pinning_weights(graph)

def main(graph):
    run(graph)