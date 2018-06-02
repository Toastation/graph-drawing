from tulip import tlp
from abc import ABC, abstractmethod
import random

COARSET_ITERATIONS = 250
FINEST_ITERATIONS = 30
NB_NODES_THRESHOLD = 50
LINKING_MAX_DIST = 2

class Merger(ABC):

    @abstractmethod
    def build_next_level():
        pass

class MIVSMerger(Merger):
    
    def __init__(self):
        self._linking_max_dist = LINKING_MAX_DIST

    ## \brief Computes a maximum independent set from a list of nodes
    # \param nodes The list of nodes to compute the independant set from
    def _compute_maximum_indepentent_set(self, graph):
        nodes = graph.nodes()
        ind_set = []
        while len(nodes) != 0:
            node = random.choice(nodes)
            ind_set.append(node)
            nodes.remove(node)
            for n in graph.getInOutNodes(node):
                if n in nodes: nodes.remove(n)
        return ind_set

    ## \brief Link the nodes of a MIVS if their distance is less than or equal to 3
    # \param root The root graph (finest)
    # \param graph The current coarsest graph
    def _link_maximum_independent_set(self, root, graph):
        done_list = []
        for n in graph.getNodes():
            reachable = tlp.reachableNodes(root, n, self._linking_max_dist, tlp.UNDIRECTED)
            for n2 in reachable:
                if not n2 in done_list and n2 in graph.getNodes():
                    graph.addEdge(n, n2)
            done_list.append(n)

    def build_next_level(self, graph, current_level):
        current_level += 1
        mivs = root.inducedSubGraph(self._compute_maximum_indepentent_set(graph), None, "level_{}".format(current_level))
        self._link_maximum_independent_set(root, mivs)

class MIESMerger(Merger):
    
    def __init__(self):
        pass

    ## \brief Merge two adjacent nodes into one metanode. The metanode is positioned at the center of the nodes and it is able
    # to move if at least one of its inner nodes can. The coarsening weight of the metanode is the sum of its 2 inner nodes
    # \param graph The graph containing the nodes, and in which the metanode will be added
    # \param n1, n2 The nodes to merge
    def _merge(self, graph, n1, n2):
        pos = graph.getLayoutProperty("viewLayout")
        weights = graph.getIntegerProperty("cWeights")        
        can_move = graph.getBooleanProperty("canMove")
        merged_node = graph.getLocalBooleanProperty("mergedNode")
        metanode_can_move = can_move[n1] or can_move[n2]
        metanode_pos = (pos[n1] + pos[n2]) / 2
        metanode = graph.createMetaNode([n1, n2])
        pos[metanode] = metanode_pos
        can_move[metanode] = metanode_can_move
        merged_node[metanode] = True

    ## \brief Compute a coarser represention of the graph
    # \param graph The graph from which to compute the coarser representation
    def build_next_level(self, graph):
        matched = graph.getLocalBooleanProperty("matched")
        weights = graph.getIntegerProperty("cWeights")                
        unmatched_nodes = graph.nodes()
        matched_nodes = []
        for n in unmatched_nodes:
            neighbors = list(graph.getInOutNodes(n))
            neighbors.sort(key = lambda node : weights[node])
            found_match = False
            for neighbor in neighbors:
                if not matched[neighbor] and n != neighbor:
                    found_match = True
                    matched[n] = True
                    matched[neighbor] = True
                    matched_nodes.append((n, neighbor))
                    unmatched_nodes.remove(n)
                    unmatched_nodes.remove(neighbor)
                    break
            if not found_match: # the node will not be merged
                matched[n]
                unmatched_nodes.remove(n)      
        for (n1, n2) in matched_nodes:
            self._merge(graph, n1, n2)

    ## \brief Initialises the coarsening weights of the graph, should only be called once on the finest graph
    # \param graph The graph on which to compute the weights
    def init_weight(self, graph):
        weights = graph.getIntegerProperty("cWeights")
        for n in graph.getNodes():
            weights[n] = 1

class Multilevel:

    def __init__(self):
        self._coarsest_iterations = COARSET_ITERATIONS
        self._finest_iterations = FINEST_ITERATIONS
        self._nb_nodes_threshold = NB_NODES_THRESHOLD
        self._merger = MIESMerger()

    def _compute_coarser_graph_series(self, root):
        current_level = 0
        graph = root.addCloneSubGraph("level_0")
        self._merger.init_weight(graph)
        coarser_graph_series = []
        coarser_graph_series.append(graph)
        while True:
            current_level += 1
            self._merger.build_next_level(graph)
            if graph.numberOfNodes() <= self._nb_nodes_threshold: 
                break
            else: 
                graph = graph.addCloneSubGraph("level_{}".format(current_level))
                coarser_graph_series.append(graph)

    def _interpolate_to_higher_level(self, graph):
        merged_node = graph.getLocalBooleanProperty("mergedNode")        
        for n in graph.getNodes():
            if merged_node[n] and graph.isMetaNode(n) :
                graph.openMetaNode(n)

    def run(self, root):
        coarser_graph_series = self._compute_coarser_graph_series(root)
        if len(coarser_graph_series) == 0:
            print("Error while computening coarser graph series")
            return False
        current_level = len(coarser_graph_series) - 1
        for i in range(current_level, -1, -1):
            pass #compute layout and interpolate
        return True
        

# Tulip script
def main(graph):
    m = Multilevel()
    m.run(graph)
