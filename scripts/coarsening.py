from tulip import tlp
import math

def init_weight(graph):
    weights = graph.getDoubleProperty("weight")
    for n in graph.getNodes():
        weights[n] = 1
    for e in graph.getEdges():
        weights[e] = 1

## Collapse the 2 given nodes into 1 metanode
# Properties are updated in the following way:
#   the new pinning weight value is the geometric mean of its 2 children
#   the new weight is the sum of the weight of its 2 children
#   the new position is the weighted average position of its children
def edge_collapse(graph, node1, node2):
    pinning_weights = graph.getDoubleProperty("pinningWeight")
    weights = graph.getDoubleProperty("weight")
    layout = graph.getLayoutProperty("viewLayout")    
    metanode = graph.createMetaNode([node1, node2])
    pinning_weights[metanode] = math.sqrt(pinning_weights[node1] * pinning_weights[node2]) 
    weights[metanode] = weights[node1] + weights[node2]
    layout[metanode] = weights[node1] * layout[node1] + weights[node2] * layout[node2]
    return metanode

def main(graph):
    pass