from tulip import tlp
import math
import kd_tree_partitioning

ITERATIONS = 1000

# computes a list of tuples (partition, partition_cg, partition_size) where partition is a list of nodes and partition_cg their center of gravity (double)
def compute_partitions_CG(partitions, layout):
    partitions_cg = []
    for p in partitions:
        cg = tlp.Vec3f()
        partition_size = len(p)
        for n in p:
            cg += layout[n]
        partitions_cg.append((p, cg/partition_size))
    return partitions_cg

def repulsive_force(dist_vec, K):
    dist_norm = dist_vec.norm()
    dist_norm_sq = dist_norm * dist_norm
    force = dist_vec * (K / dist_norm_sq)
    return force / dist_norm

def attractive_force(dist_vec, K, L):
    dist_norm = dist_vec.norm()
    force = dist_vec * K * (dist_norm - L)
    return force / dist_norm

def run(graph, iterations):
    # constants
    L = 10
    K_r = 6250
    K_s = 1
    R = 0.05
    N = graph.numberOfNodes()    
    t = 100 # cst = 0.04
    t_f = 0.9
    max_disp_sq = 100
    conv_threshold = 6.0 / N

    layout = graph.getLayoutProperty("viewLayout")
    disp = graph.getLayoutProperty("disp")
    quit = False
    it = 1

    while not quit:
        total_disp = 0
        # repulsive forces
        for u in graph.getNodes():
            for v in graph.getNodes():
                if u == v: continue
                dist = layout[u] - layout[v]
                force = repulsive_force(dist, K_r)
                disp[u] += force
        
        # attractive forces
        for e in graph.getEdges():
            u = graph.source(e)
            v = graph.target(e)
            dist = layout[u] - layout[v]
            force = attractive_force(dist, K_s, L)
            disp[u] -= force
            disp[v] += force

        # update positions
        for n in graph.getNodes():
            disp_norm = disp[n].norm()
            if disp_norm != 0: disp[n] = (disp[n] / disp_norm) * min(disp_norm, t)
            else: disp[n] = tlp.Vec3f()
            total_disp += min(disp_norm, t)
            layout[n] += disp[n]
            if total_disp < conv_threshold: quit = True
        quit = it > iterations or quit # maybe infinite if conv_threshold is too low...
        it += 1
        t *= t_f
    print("number if iterations done: {}".format(it))

def main(graph):
    run(graph, ITERATIONS)
    updateVisualization()   
