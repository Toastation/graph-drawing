from tulip import tlp
import math
import kd_tree_partitioning

ITERATIONS = 100

def repulsive_force(dist, k):
    return (k * k) / dist
  
def attractive_force(dist, k):
    return (dist * dist) / k

def cool(temp, dt):
    temp -= dt      # TODO: experiment with non-linear cooling
    return temp if temp >= 0 else 0

def normalize_vec3f(vec):
    return vec / vec.norm()

# computes a list of tuples (partition, partition_cg, partition_size) where partition is a list of nodes and partition_cg their center of gravity (double)
def compute_partitions_CG(partitions, layout):
    partitions_cg = []
    for p in partitions:
        cg = tlp.Vec3f()
        partition_size = len(p)
        for n in p:
            cg += layout[n]
        partitions_cg.append((p, cg/partition_size), len(p))
    return partitions_cg
        
def fr_2(graph, iterations):
    layout = graph.getLayoutProperty("viewLayout")
    forces = graph.getLayoutProperty("forces")
    pinning_weights = graph.getLayoutProperty("pinningWeight")

    min_partition_size = 20
    frac_done = 0
    K = 0.1 # optimal geometric node distance
    temp = K * math.sqrt(graph.numberOfNodes())
    temp_decay = 0.9 
    
    partitions = compute_partitions_CG(kd_tree_partitioning.run(graph, min_partition_size), layout)

    for i in range(iterations):
        for (p, cg, s) in partitions:
            for n in p:
                if frac_done > pinning_weights[n]:
                    # computing repulsive forces for neighbor inside the partition
                    for n2 in p:
                        if n != n2:
                            delta_pos = layout[n] - layout[n2]
                            dist = delta_pos.norm()
                            forces[n] += (K * K) * (delta_pos / (dist * dist))
                
                    # computing repulsive forces between the node and the other partitions
                    for (p2, cg2, s2) in partitions:
                        if p2 != p:
                            delta_pos = layout[n] - cg2
                            dist = delta_pos.norm()
                            forces[n] += K * K * s2 * (delta_pos / (dist * dist))

        # computing attractives forces for every edge
        for e in graph.getEdges():
            source = graph.source(e)
            target = graph.target(e)
            delta_pos = layout[source] - layout[target]
            force = (dist / K) * delta_pos
            if frac_done > pinning_weights[source]:
                forces[source] -= force
            if frac_done > pinning_weights[target]:            
                forces[target] += force 

        # updating positions
        for n in graph.getNodes():
            force = forces[n].norm()
            layout[n] += (forces[n] / force) * min(temp, force)
        temp *= temp_decay
        frac_done = iterations / i


# basic implementation of the Fruchterman-Reingold algorithm, not efficient
def run(graph, iterations):
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
    dt = temp / (iterations + 1)

    # init forces
    for n in graph.nodes():
        forces[n] = tlp.Vec3f()

    for i in range(iterations):
        # computing repulsive forces for every pair of nodes
        for n in graph.getNodes():
            for n2 in graph.getNodes():
                if n != n2:
                    delta_pos = layout[n] - layout[n2]
                    dist = delta_pos.norm()
                    if dist != 0: # TODO: push the nodes if dist == 0
                        forces[n] += delta_pos * (repulsive_force(dist, k) / dist)

        # computing attractive forces for every edge
        for e in graph.getEdges():
            source = graph.source(e)
            target = graph.target(e)
            delta_pos = layout[source] - layout[target]
            dist = delta_pos.norm()
            if dist != 0: # randomly generated graphs may have edges that connect the same node...
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

def main(graph):
    run(graph, ITERATIONS)
