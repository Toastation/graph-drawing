from tulip import tlp
from FMMM_incremental.fmmm_layout import FrishmanLayout
from frishman_merger import FrishmanMerger

class FrishmanTal():
    
    def __init__(self):
        self._merger = FrishmanMerger()
        self._layout = FrishmanLayout()

    def init_constants(self, graph):
        self._iterations = 300
        self._const_temp = False
        self._temp_init_factor = 0.2

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

    def run(self, graph, multilevel=True, debug=False):
        subgraphs = list(root.getSubGraphs())
        subgraphs.sort(key = lambda g : g.getId())
        if len(subgraphs) == 0:
            sg = root.addCloneSubGraph("timeline_0")
            sg.applyAlgorithm(static_layout_algo)
        elif len(subgraphs) == 1:
            subgraphs[0].applyAlgorithm(static_layout_algo)
        else:
            previous_graph = subgraphs[-2]
            new_graph = subgraphs[-1]
            self._compute_difference(previous_graph, new_graph)
            self._merger.run(new_graph)
            if multilevel:
                pass
            else:
                self._layout.run(graph, self._iterations, self._const_temp, self._temp_init_factor)    

def main(graph):
    f = FrishmanTal()
    f.run(graph, False)