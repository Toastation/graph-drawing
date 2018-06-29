# see README.md
# How to use:
#    Graph hierarchy: 
#        This script should only be called on the root graph. The root graph must contain all the nodes/edges of every graph of the timeline.
#        The subgraphs of the root graph correspond to the differents steps of the timeline. They must be ordered by their id.
#
#    /!\ Currently this script only works for online graph drawing, i.e when the script is called, it will compute the layout from the latest graph in the timeline
#    based on the previous one.

from tulip import tlp
from fmmm_multilevel import Multilevel
from fmmm_merger import MergerFMMM
from layout import FMMMLayout2
from fmmm_static import FMMMStatic
import random

IDEAL_EDGE_LENGTH = 10

def lerp(a, b, t):
    return a * (1.0 - t) + b * t

def morph(graph, steps=500):
    is_new_node = graph.getBooleanProperty("isNewNode")
    is_new_edge = graph.getBooleanProperty("isNewEdge")
    color = graph.getColorProperty("viewColor")
    result = graph.getLayoutProperty("result")
    pos = graph.getLayoutProperty("viewLayout")
    previous_pos = graph.getLayoutProperty("previousPos")
    
    for n in is_new_node.getNodesEqualTo(True):
        pos[n] = result[n]
        color[n].setA(0)
    for e in is_new_edge.getEdgesEqualTo(True):
        color[e].setA(0)            

    for i in range(1, steps + 1):
        t = 1.0 * i / steps # "/" operator is the integer division in python 2.7, hence the float constant

        for n in graph.getNodes():
            if is_new_node[n]:
                alpha = lerp(0, 255, t)
                c = color[n]
                color[n] = tlp.Color(c.getR(), c.getG(), c.getB(), int(alpha))
            else:
                pos[n] = lerp(previous_pos[n], result[n], t)
        for e in graph.getEdges():
            if is_new_edge[e]:
                alpha = lerp(0, 255, t)
                c = color[e]
                color[e] = tlp.Color(c.getR(), c.getG(), c.getB(), int(alpha))
        updateVisualization(True)

class FMMMIncremental:
    
    def __init__(self):
        self._ideal_edge_length = IDEAL_EDGE_LENGTH
        self._merger = MergerFMMM()
        self._multilevel = Multilevel()
        self._layout = FMMMLayout2()
        self._static = FMMMStatic()
        self._layout.set_result("result")
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
        self._debug(new_graph)

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
            previous_pos = sg.getLayoutProperty("previousPos")
            previous_pos.copy(sg.getLayoutProperty("viewLayout"))
            self._static.run(sg, multilevel)
            result = sg.getLayoutProperty("result")
            layout = sg.getLayoutProperty("viewLayout")
            result.copy(layout)
            layout.copy(previous_pos)
            morph(sg)
            # ds = tlp.getDefaultPluginParameters("GEM (Frick)", sg)
            # sg.applyLayoutAlgorithm("GEM (Frick)", ds)
        elif len(subgraphs) == 1:
            # ds = tlp.getDefaultPluginParameters("GEM (Frick)", subgraphs[0])
            # subgraphs[0].applyLayoutAlgorithm("GEM (Frick)", ds)
            previous_pos = subgraphs[0].getLayoutProperty("previousPos")
            previous_pos.copy(subgraphs[0].getLayoutProperty("viewLayout"))
            self._static.run(subgraphs[0], multilevel)
            result = subgraphs[0].getLayoutProperty("result")
            layout = subgraphs[0].getLayoutProperty("viewLayout")
            result.copy(layout)
            layout.copy(previous_pos)
            morph(subgraphs[0])
        else:
            previous_graph = subgraphs[-2]
            new_graph = subgraphs[-1]
            previous_pos = new_graph.getLayoutProperty("previousPos")
            previous_pos.copy(new_graph.getLayoutProperty("viewLayout"))
            self._compute_difference(previous_graph, new_graph)
            self._merger.run(new_graph)
            if multilevel:
                self._multilevel.run(new_graph)
            else:
                # ds = tlp.getDefaultPluginParameters("GEM (Frick)", new_graph)
                # ds["initial layout"] = new_graph.getLayoutProperty("viewLayout")
                # canMove = new_graph.getBooleanProperty("canMove")
                # canMoveCompl = new_graph.getBooleanProperty("canMoveCompl")
                # for n in graph.getNodes():
                #     canMoveCompl[n] = not canMove[n]
                # ds["unmovables nodes"] = canMoveCompl
                # subgraphs[0].applyLayoutAlgorithm("GEM (Frick)", ds)
                self._layout.run(new_graph, self._iterations, new_graph.getBooleanProperty("canMove"))
            result = new_graph.getLayoutProperty("result")
            layout = new_graph.getLayoutProperty("viewLayout")
            result.copy(layout)
            layout.copy(previous_pos)
            morph(new_graph)            

def main(graph):
    layout = FMMMIncremental()
    layout.run(graph, False)
            
