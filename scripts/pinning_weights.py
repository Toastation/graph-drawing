from tulip import tlp
import math

NEIGHBOR_INFLUENCE = 0.6    # [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement
PINNING_WEIGHT_INIT = 0.35  # determines the decay of the pinning weight
K = 0.5                     # determines how much a change should spread

## Computes the positioning score of each node, and place new nodes according to their number of positioned neighbors.
# These scores indicate the "confidence" in the node's position (comprised between [0, 1]).
# A score of 1 is assigned to nodes already in the graph, 0.25 to new nodes with 2 or more positioned neighbors,
# 0.1 to to new nodes with 1 positioned neighbor and 0 to new nodes with no neighbors.
# New nodes with 2 more positioned neighbors are place at their weighted barycenter, new nodes with 1 positioned
# neighbor are place between their neighbor and the center of the graph, and new nodes with no positioned neighbors
# are placed in a circle around the center of the graph.
def compute_positioning_score(graph):
    is_new_node = graph.getBooleanProperty("isNewNode")
    positioned = graph.getBooleanProperty("positioned")    
    positioning_scores = graph.getDoubleProperty("positioningScore")    
    layout = graph.getLayoutProperty("viewLayout")
    center_coord = tlp.computeBoundingBox(graph).center()
    dalpha = 360 / graph.numberOfNodes()
    alpha = 0
    radius = 1

    for n in is_new_node.getNodesEqualTo(False):
        positioning_scores[n] = 1  

    new_nodes_graph = graph.addSubGraph(is_new_node, "newNodes")        

    for n in new_nodes_graph.bfs():
        positioned_neighbors = [neighbor for neighbor in graph.getInOutNodes(n) if positioned[neighbor]]
        nb_positioned = len(unpositioned_neighbors)       
        if nb_positioned >= 2:
            positioning_scores[n] = 0.25
            layout[n] = tlp.Vec3f()
            for neighbor in positioned_neighbors: layout[n] += layout[neighbor]
            layout[n] /=  nb_positioned
            positioned[n] = True
        elif nb_positioned == 1:
            positioning_scores[n] = 0.1
            layout[n] = (layout[positioned_neighbors[0]] + center_coord) / 2             
        else:
            positioning_scores[n] = 0            
            layout[n] = tlp.Vec3f(center_coord.x() + radius * math.cos(alpha), center_coord.x() + radius * math.sin(alpha))
            alpha += dalpha
    
    graph.delSubGraph(new_nodes_graph)        
    

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