from tulip import tlp
import math

NEIGHBOR_INFLUENCE = 0.6    # [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement
PINNING_WEIGHT_INIT = 0.35  # determines the decay of the pinning weight
K = 0.5                     # determines how much a change should spread

## \brief Computes the positioning score of each node, and place new nodes according to their number of positioned neighbors.
# These scores indicate the "confidence" in the node's position (comprised between [0, 1]).
# A score of 1 is assigned to nodes already in the graph, 0.25 to new nodes with 2 or more positioned neighbors,
# 0.1 to to new nodes with 1 positioned neighbor and 0 to new nodes with no neighbors.
# New nodes with 2 more positioned neighbors are place at their weighted barycenter, new nodes with 1 positioned
# neighbor are place between their neighbor and the center of the graph, and new nodes with no positioned neighbors
# are placed in a circle around the center of the graph.
# \param graph The graph on which to compute the scores
def compute_positioning_score(graph):
    positioned = graph.getLocalBooleanProperty("positioned")    
    is_new_node = graph.getBooleanProperty("isNewNode")
    positioning_scores = graph.getDoubleProperty("positioningScore")    
    layout = graph.getLayoutProperty("viewLayout")
    center_coord = tlp.computeBoundingBox(graph).center()
    dalpha = 360 / graph.numberOfNodes()
    alpha = 0
    radius = 1
    for n in is_new_node.getNodesEqualTo(False):
        positioning_scores[n] = 1
        positioned[n] = True
    for n in graph.bfs():
        if is_new_node[n]:
            positioned_neighbors = [neighbor for neighbor in graph.getInOutNodes(n) if positioned[neighbor]]
            nb_positioned = len(positioned_neighbors)    

            if nb_positioned >= 2:
                positioning_scores[n] = 0.25
                layout[n] = tlp.Vec3f()
                for neighbor in positioned_neighbors: 
                    layout[n] += layout[neighbor]
                layout[n] /=  nb_positioned
                positioned[n] = True
            elif nb_positioned == 1:
                positioning_scores[n] = 0.1
                layout[n] = (layout[positioned_neighbors[0]] + center_coord) / 2             
            else:
                positioning_scores[n] = 0            
                layout[n] = tlp.Vec3f(center_coord.x() + radius * math.cos(alpha), center_coord.x() + radius * math.sin(alpha))
                alpha += dalpha
    graph.delLocalProperty("positioned")    

## \brief Computes a series of sets in the following manner:
# D_0 = the set of new nodes and nodes adjacent to a removed node
# D_i+1 = the set of nodes adjacent to a node in D_i and not yet in a set
# \param graph The graph from which to computes the sets
# \return A list of list of tlp.node
def compute_distance_to_node(graph):
    in_set = graph.getLocalBooleanProperty("inSet") # has a node been added to a set
    is_adjacent_to_changed = graph.getBooleanProperty("isAdjacentToChanged")
    pinning_weights = graph.getDoubleProperty("pinningWeight")    
    distance_to_node = [[n for n in graph.getNodes() if pinning_weights[n] < 1 or is_adjacent_to_changed[n]]] # D0
    current_set_index = 0
    for n in distance_to_node[0]:
        in_set[n] = True
    while True:
        next_set = []
        for n in distance_to_node[current_set_index]:
            for neighbor in graph.getInOutNodes(n):
                if not in_set[neighbor]:
                    next_set.append(neighbor)
                    in_set[neighbor] = True
        if len(next_set) == 0: break 
        distance_to_node.append(next_set)
        current_set_index += 1
    graph.delLocalProperty("inSet")
    return distance_to_node

## \brief Computes and assign a pinning weight to every node of the graph.
# Pinning weights indicate how much a node will be able to move during the layout algo.
# Pinning weights are comprised in the interval [0, 1]. A node with a pinning weight of 1
# will not move during the layout algo whereas a node with a pinning weight of 0 will be 
# completely free to move.
# \param graph The graph on which to compute the pinning weights
# \param neighbor_influence [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement
def compute_pinning_weights(graph, neighbor_influence):
    compute_positioning_score(graph)  
    positioning_scores = graph.getDoubleProperty("positioningScore")
    pinning_weights = graph.getDoubleProperty("pinningWeight")
    neighbor_influence_complement = 1 - neighbor_influence  
    # local pass
    for n in graph.getNodes():
        neighbor_sum = 0
        for neighbor in graph.getInOutNodes(n):
            neighbor_sum += positioning_scores[neighbor]
        pinning_weights[n] = neighbor_influence * positioning_scores[n] + neighbor_influence_complement * (1.0 / graph.deg(n)) * neighbor_sum
    # global pass
    sets = compute_distance_to_node(graph)
    print(len(sets))
    if len(sets) == 0: 
        print("Unable to compute distance-to-node sets")
        return
    d_cutoff = round(K * len(sets) - 1)
    for i in range(len(sets)):
        for n in sets[i]:
            if i < d_cutoff:
                pinning_weights[n] = PINNING_WEIGHT_INIT ** (1 - (i / d_cutoff))
            else:
                pinning_weights[n] = 1      

def debug(graph):
    pinning_weights = graph.getDoubleProperty("pinningWeight")
    vc = graph.getColorProperty("viewColor")
    for n in graph.getNodes():
        g = round(pinning_weights[n] * 255)
        vc[n] = tlp.Color(0, int(g), 0)        

def run(graph):
    compute_pinning_weights(graph, NEIGHBOR_INFLUENCE)
    debug(graph)

def main(graph):
    run(graph)
