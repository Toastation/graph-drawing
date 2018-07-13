from tulip import tlp
import math
import random

DESIRED_LENGTH = 10         # desired edge length
NEIGHBOR_INFLUENCE = 0.6    # [0, 1], a higher value will reduce the influence of the neighbors of a node on its displacement
PINNING_WEIGHT_INIT = 0.35  # determines the decay of the pinning weight
K = 0.5                     # determines how much a change should spread
TAU = 2 * math.pi

class MergerFMMM:

    def __init__(self):
        self._desired_length = DESIRED_LENGTH
    
    ## \brief Positions new nodes, and identify which nodes should move during the layout process, via the boolean property "canMove"
    # \param graph The graph from which to position new nodes
    def _position_nodes(self, graph):
        is_new_node = graph.getBooleanProperty("isNewNode")
        is_new_edge = graph.getBooleanProperty("isNewEdge")
        adjacent_deleted_edge = graph.getBooleanProperty("adjDeletedEdge")        
        positioned = graph.getBooleanProperty("positioned")
        can_move = graph.getBooleanProperty("canMove")
        pos = graph.getLayoutProperty("viewLayout")
        bounding_box = tlp.computeBoundingBox(graph)
        new_edges = (e for e in graph.getEdges() if is_new_edge[e])
        new_nodes = []

        for n in graph.getNodes():
            if adjacent_deleted_edge[n]:
                can_move[n] = True
            if not is_new_node[n]:
                positioned[n] = True # TODO: should be computed only on the first graph of the timeline 
            else:
                new_nodes.append(n)
        
        # position new nodes
        new_nodes_sg = graph.inducedSubGraph(new_nodes)
        connected_components = tlp.ConnectedTest.computeConnectedComponents(new_nodes_sg)
        for cc in connected_components:
            if len(cc) == 0: return
            cc_sg = new_nodes_sg.inducedSubGraph(cc)

            # find the node with the most positioned neighbors in the connected component, and position the new nodes in a bfs-way from this node
            max_positioned_neighbors = -1
            max_positioned_neighbors_node = cc[0]
            for n in cc:
                positioned_neighbors = [neighbor for neighbor in cc_sg.getInOutNodes(n) if positioned[neighbor]]
                if len(positioned_neighbors) > max_positioned_neighbors:
                    max_positioned_neighbors_node = n
                    max_positioned_neighbors = len(positioned_neighbors)

            for n in cc_sg.bfs(max_positioned_neighbors_node):
                can_move[n] = True
                positioned_neighbors = [neighbor for neighbor in cc_sg.getInOutNodes(n) if positioned[neighbor]]
                nb_positioned_neighbors = len(positioned_neighbors)
                if nb_positioned_neighbors ==  0:
                    pos[n] = tlp.Vec3f(bounding_box[0].x() + random.random() * bounding_box.width(), bounding_box[0].y() + random.random() * bounding_box.height())
                elif nb_positioned_neighbors == 1:
                    angle = random.random() * TAU
                    pos[n] = pos[positioned_neighbors[0]] + tlp.Vec3f(self._desired_length * math.cos(angle), self._desired_length * math.sin(angle))
                    can_move[positioned_neighbors[0]] = True
                else:
                    sum_pos = tlp.Vec3f()
                    for neighbor in positioned_neighbors:
                        sum_pos += pos[neighbor]
                        can_move[neighbor] = True
                    pos[n] = sum_pos / nb_positioned_neighbors
                positioned[n] = True

            new_nodes_sg.delSubGraph(cc_sg)
        
        for e in new_edges:
            can_move[graph.source(e)] = True
            can_move[graph.target(e)] = True

    def _debug(self, graph):
        can_move = graph.getBooleanProperty("canMove")
        color = graph.getColorProperty("viewColor")
        for n in graph.getNodes():
            if can_move[n]:
                color[n] = tlp.Color(255,127,80)

    def run(self, graph, debug=False):
        self._position_nodes(graph)
        if debug: self._debug(graph)

def main(graph):
    merger = MergerFMMM()
    merger.run(graph, True)
