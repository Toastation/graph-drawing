from tulip import tlp
import time

def lerp(a, b, t):
    return a * (1.0 - t) + b * t
        
def main(graph):
    #graph.applyAlgorithm("Incremental")
    pos = graph.getLayoutProperty("viewLayout")
    previous_pos = tlp.LayoutProperty(graph)
    color = graph.getColorProperty("viewColor")
    i = 0
    steps = 5000
    for g in graph.getSubGraphs():
        print(g)
        sg_pos = g.getLocalLayoutProperty("viewLayout")
        is_new_node = g.getLocalBooleanProperty("isNewNode")
        is_new_edge = g.getLocalBooleanProperty("isNewEdge")
        sg_color = g.getLocalColorProperty("viewColor")
        for n in graph.getNodes():
          if not n in g.getNodes():
            color[n] = tlp.Color(0, 0, 0, 0) 
            pos[n] = sg_pos[g.getOneNode()]
          else:
            color[n] = sg_color[n]
            pos[n] = sg_pos[n]
        for e in graph.getEdges():
          if not e in g.getEdges():
            color[e] = tlp.Color(0, 0, 0, 0) 
          else:
            color[e] = sg_color[e]
        if i > 0:
            for n in is_new_node.getNodesEqualTo(True):
                pos[n] = sg_pos[n]
                color[n].setA(0)
            for e in is_new_edge.getEdgesEqualTo(True):
                color[e].setA(0)   
            for i in range(1, steps + 1):
                t = 1.0 * i / steps # "/" operator <=> integer division in python 2.7, hence the float constant
                for n in g.getNodes():
                    if is_new_node[n]:
                        alpha = lerp(0, 255, t)
                        c = color[n]
                        color[n] = tlp.Color(c.getR(), c.getG(), c.getB(), int(alpha))
                    else:
                        pos[n] = lerp(previous_pos[n], sg_pos[n], t)
                for e in g.getEdges():
                    if is_new_edge[e]:
                        alpha = lerp(0, 255, t)
                        c = color[e]
                        color[e] = tlp.Color(c.getR(), c.getG(), c.getB(), int(alpha))
                updateVisualization(True)
        updateVisualization(True) 
        pauseScript()             
        previous_pos.copy(sg_pos)
        i += 1
    
