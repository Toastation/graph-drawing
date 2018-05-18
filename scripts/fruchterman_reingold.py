# Basic version of the Fruchterman-Reingold algorithm
from tulip import tlp
import math

# TODO : get width and height from the user
WIDTH = 100
HEIGHT = 100
AREA = WIDTH * WIDTH
ITERATIONS = 1000  

def repulsive_force(pos, k):
    return (k * k) / pos
  
def attractive_force(pos, k):
    return (pos * pos) / k

def cool(temp):
    temp -= 1 / temp
    return temp if temp >= 0 else 0

def normalize_vec3f(vec):
    return vec / vec.norm()

def main(graph):  
    layout = graph.getLayoutProperty("viewLayout")
    forces = graph.getLayoutProperty("forces")          # node:tlp.Vec3f dict representing the total forces applied on each node 
    nb_nodes = graph.numberOfNodes()
    k = math.sqrt(AREA / nb_nodes)                      # ideal length between nodes given the above force model 
    temp = min(WIDTH, HEIGHT) / 10

    for i in range(ITERATIONS):
        # computing repulsive forces for every pair of nodes
        for index_n, n in enumerate(graph.nodes()[:nb_nodes]):
            forces[n] = tlp.Vec3f()
            for n2 in graph.nodes()[index_n+1:]:
                delta_pos = layout[n] - layout[n2]
                force = normalize_vec3f(delta_pos) * repulsive_force(delta_pos.norm(), k)
                forces[n] += force
                forces[n2] -= force

        # computing attractive forces for every edge
        for e in graph.getEdges():
            source = graph.source(e)
            target = graph.target(e)
            delta_pos = layout[source] - layout[target]
            force = normalize_vec3f(delta_pos) * attractive_force(delta_pos.norm(), k)
            forces[source] -= force
            forces[target] += force
    
        # updating nodes position and keeping them in-bound
        for n in graph.getNodes():
            layout[n] += normalize_vec3f(forces[n]) * min(forces[n].norm(), temp)
            layout[n].setX(min(WIDTH, max(0, layout[n].x())))  
            layout[n].setY(min(HEIGHT, max(0, layout[n].y())))
            
        temp = cool(temp)
        if temp == 0: break
