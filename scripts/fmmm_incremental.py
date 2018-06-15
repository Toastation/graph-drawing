from tulip import tlp
from fmmm_multilevel import Multilevel
from fmmm_merger import MergerFMMM
from layout import FMMMLayout2
from fmmm_static import FMMMStatic

IDEAL_EDGE_LENGTH = 10

class FMMMIncremental:
    
    def __init__(self):
        self._ideal_edge_length = IDEAL_EDGE_LENGTH
        self._merger = MergerFMMM()
        self._multilevel = Multilevel()
        self._layout = FMMMLayout2()
        self._static = FMMMStatic()
        self.init_constants()

    def init_constants(self):
        self._iterations = 300        

    def _compute_difference(self, old_graph, new_graph):
        is_new_node = new_graph.getBooleanProperty("isNewNode")
        is_new_edge = new_graph.getBooleanProperty("isNewEdge")
        adjacent_deleted_edge = new_graph.getBooleanProperty("adjDeletedEdge")
        is_new_node.setAllNodeValue(False)
        is_new_edge.setAllEdgeValue(False)
        adjacent_deleted_edge.setAllNodeValue(False)

        for n in new_graph.getNodes():
            if not n in old_graph.getNodes():
                is_new_node[n] = True

        for e in new_graph.getEdges():
            if not e in old_graph.getEdges():
                is_new_edge[e] = True

        for e in old_graph.getEdges():
            if not e in new_graph.getEdges():
                source = old_graph.source(e)
                target = old_graph.target(e)
                if source in new_graph.getNodes(): 
                    adjacent_deleted_edge[source] = True                                                
                if target in new_graph.getNodes(): 
                    adjacent_deleted_edge[target] = True

    def _debug(self, graph):
        color = graph.getColorProperty("viewColor")
        is_new_node = graph.getBooleanProperty("isNewNode")
        is_new_edge = graph.getBooleanProperty("isNewEdge")    
        adjacent_deleted_edge = graph.getBooleanProperty("adjDeletedEdge")    
        for n in graph.getNodes():
            if is_new_node[n] or adjacent_deleted_edge[n]: color[n] = tlp.Color(200, 20, 0)
        for e in graph.getEdges():
            if is_new_edge[e]: color[e] = tlp.Color(200, 20, 0)

    def run(self, root, multilevel=True, debug=False):
        subgraphs = list(root.getSubGraphs())
        subgraphs.sort(key = lambda g : g.getId())
        if len(subgraphs) == 0:
            sg = root.addCloneSubGraph("timeline_0")
            self._static.run(sg, multilevel)
        elif len(subgraphs) == 1:
            self._static.run(subgraphs[0], multilevel)
        else:
            previous_graph = subgraphs[-2]
            new_graph = subgraphs[-1]
            self._compute_difference(previous_graph, new_graph)
            self._merger.run(new_graph)
            if multilevel:
                self._multilevel.run(new_graph)
            else:
                self._layout.run(new_graph, self._iterations, new_graph.getBooleanProperty("canMove"))

def main(graph):
    layout = FMMMIncremental()
    layout.run(graph, False)
            
