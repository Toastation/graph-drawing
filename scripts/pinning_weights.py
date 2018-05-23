from tulip import tlp

NEIGHBOR_INFLUENCE = 0.6    # [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement
PINNING_WEIGHT_INIT = 0.35  # determines the decay of the pinning weight
K = 0.5                     # determines how much a change should spread

def compute_positioning_score(graph):
    # TODO: find a way to determine which node is new and which node had one of its neighbor removed.
    # or find a way to get the previous layout of the graph.
    pass

def compute_distance_to_node(graph):
    # TODO: if a BFS way, create a series of sets where D0 = [pinning_weights[n] < 1] U [nodes adjacent to a change]
    # and Di+1 = [nodes not in a set and ajdacent to Di]
    # should return the list of sets (?)
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
    sets = compute_distance_to_node(graph)
    d_cutoff = round(K * len(sets))

    for i in range(len(sets)):
        for n in sets[i]:
            if i <= d_cutoff:
                pinning_weights = PINNING_WEIGHT_INIT ** (1 - (i / d_cutoff))
            else:
                pinning_weights[n] = 1      

def run(graph):
    compute_positioning_score(graph)
    compute_pinning_weights(graph)

def main(graph):
    run(graph)