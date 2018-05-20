# Basic version of the Fruchterman-Reingold algorithm
from tulip import tlp
import math

ITERATIONS = 1000

def repulsive_force(dist, k):
    return (k * k) / dist
  
def attractive_force(dist, k):
    return (dist * dist) / k

def cool(temp, dt):
    temp -= dt      # TODO: experiment with non-linear cooling
    return temp if temp >= 0 else 0

def normalize_vec3f(vec):
    return vec / vec.norm()

def main(graph):
    boundingbox = tlp.computeBoundingBox(graph) 
    width = boundingbox.width()
    height = boundingbox.height()
    area = width * height
    min_coord = boundingbox[0]
    max_coord = boundingbox[1]
    
    layout = graph.getLayoutProperty("viewLayout")
    forces = graph.getLayoutProperty("forces")          # node:tlp.Vec3f dict representing the total forces applied on each node 
    nb_nodes = graph.numberOfNodes()
    
    k = math.sqrt(area / nb_nodes)                      # ideal length between nodes given the above force model 
    temp = min(width, height) / 10
    dt = temp / (ITERATIONS + 1)

    # init forces
    for n in graph.nodes():
        forces[n] = tlp.Vec3f()

    for i in range(ITERATIONS):
        # computing repulsive forces for every pair of nodes
        for n in graph.getNodes():
            for n2 in graph.getNodes():
                if n != n2:
                    delta_pos = layout[n] - layout[n2]
                    dist = delta_pos.norm()
                    if dist != 0:
                        forces[n] += delta_pos * (repulsive_force(dist, k) / dist)

        # computing attractive forces for every edge
        for e in graph.getEdges():
            source = graph.source(e)
            target = graph.target(e)
            delta_pos = layout[source] - layout[target]
            dist = delta_pos.norm()
            force = attractive_force(dist, k) / dist 
            forces[source] -= delta_pos * force
            forces[target] += delta_pos * force

        # updating nodes position and keeping them in-bound
        for n in graph.getNodes():
            force = forces[n].norm()
            if force != 0:
                displacement = min(force, temp) / force
                layout[n] += forces[n] * displacement
                layout[n].setX(min(min_coord.x(), max(max_coord.x(), layout[n].x())))
                layout[n].setY(min(min_coord.y(), max(max_coord.y(), layout[n].y())))                
            forces[n].fill(0)
        temp = cool(temp, dt)
