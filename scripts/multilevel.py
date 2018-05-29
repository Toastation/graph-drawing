from tulip import tlp
import math

MAX_ITERATIONS = 4

def init_weights(graph):
    weights = graph.getDoubleProperty("weight")
    for n in graph.getNodes():
        weights[n] = 1
    for e in graph.getEdges():
        weights[e] = 1

## \brief Collapses the extremities of the edge into 1 metanode
# Properties are updated in the following way: the new pinning weight 
# value is the geometric mean of its 2 children, the new weight is the
# sum of the weight of its 2 children, the new position is the weighted 
# average position of its children
# \param parent_graph The root graph
# \param graph A clone sub-graph of parent_graph
# \param edge The edge to collapse
# \return the new metanode 
def edge_collapse(parent_graph, graph, edge):
    pinning_weights = parent_graph.getDoubleProperty("pinningWeight")
    weights = parent_graph.getDoubleProperty("weight")
    layout = parent_graph.getLayoutProperty("viewLayout")
    node1 = graph.source(edge)
    node2 = graph.target(edge)
    metanode = graph.createMetaNode([node1, node2])    
    pinning_weights[metanode] = math.sqrt(pinning_weights[node1] * pinning_weights[node2]) 
    weights[metanode] = weights[node1] + weights[node2]
    layout[metanode] = weights[node1] * layout[node1] + weights[node2] * layout[node2]
    return metanode

## \brief Finds and return the best edge to collapse
# \param graph The graph on which to search for the next metanode
# \return the next edge to collapse, None if there's none
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

## \brief Deletes a metanode and puts its inner nodes at their new weighted positions
# \param graph The graph containing the metanode
# \param metanode The metanode to delete
def del_metanode(graph, metanode):
    layout = graph.getLayoutProperty("viewLayout")
    pinning_weights = graph.getLayoutProperty("pinningWeight")    
    bounding_box_old = tlp.computeBoundingBox(graph)
    metanode_it = graph.getNodeMetaInfo(metanode).getNodes()
    metanode_coord = layout[metanode]
    graph.openMetaNode(metanode)
    bounding_box_new = tlp.computeBoundingBox(graph)
    area_old = bounding_box_old.width() * bounding_box_old.height()
    area_new = bounding_box_new.width() * bounding_box_new.height()    
    for n in metanode_it:
        layout[n] = (1 - pinning_weights[n]) * (area_old / area_new) * (layout[n] - metanode_coord)

def run(parent_graph, iterations):
    graph = parent_graph.addCloneSubGraph("graph")
    init_weights(parent_graph)
    for i in range(iterations):
        edge = find_next_collapse(graph)
        if edge:
            edge_collapse(parent_graph, graph, edge)
    return graph

def main(graph):
    run(graph, MAX_ITERATIONS)
