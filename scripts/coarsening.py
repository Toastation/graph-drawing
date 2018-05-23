from tulip import tlp
import math

MAX_ITERATIONS = 4

def init_weights(graph):
    weights = graph.getDoubleProperty("weight")
    for n in graph.getNodes():
        weights[n] = 1
    for e in graph.getEdges():
        weights[e] = 1

## Collapse the extremities of the edge into 1 metanode
# Properties are updated in the following way:
#   the new pinning weight value is the geometric mean of its 2 children
#   the new weight is the sum of the weight of its 2 children
#   the new position is the weighted average position of its children
def edge_collapse(graph, edge):
    pinning_weights = graph.getDoubleProperty("pinningWeight")
    weights = graph.getDoubleProperty("weight")
    layout = graph.getLayoutProperty("viewLayout")    
    node1 = graph.source(edge)
    node2 = graph.target(edge)
    metanode = graph.createMetaNode([node1, node2])
    # TODO: create the metanode and the corresponding subgraphs
    pinning_weights[metanode] = math.sqrt(pinning_weights[node1] * pinning_weights[node2]) 
    weights[metanode] = weights[node1] + weights[node2]
    layout[metanode] = weights[node1] * layout[node1] + weights[node2] * layout[node2]
    return metanode

## find and return the best edge to collapse
# return None if there's no edge to collapse
def find_next_collapse(graph):
    weights = graph.getDoubleProperty("weight")    
    current_max = 0
    current_max_edge = None
    for e in graph.getEdges():
        source = graph.source(e)
        target = graph.target(e)
        measure = (weights[e] / weights[source]) + (weights[e] / weights[target])
        if  measure > current_max:
            current_max = measure
            current_max_edge = e
    return current_max_edge

def run(graph, iterations):
    init_weights(graph)
    coarsest_graph = graph
    for i in range(iterations):
        edge = find_next_collapse(coarsest_graph)
        if edge:
            edge_collapse(coarsest_graph, edge)
            coarsest_graph = graph.getSubGraph("groups") # "groups" contains only the metanodes and the nodes NOT inside a metanode
        

def main(graph):
    run(graph, MAX_ITERATIONS)
