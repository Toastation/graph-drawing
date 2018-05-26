from tulip import tlp
import math
import kd_tree_partitioning

ITERATIONS = 1000

def compute_partitions_CG(partitions, layout):
    result = []
    for p in partitions:
        cg =  tlp.Vec3f()
        for n in p:
            cg += layout[n]
        result.append((p, cg / len(p)))
    return result

def repulsive_force(dist_vec, K):
    dist_norm = dist_vec.norm()
    if dist_norm == 0: return tlp.Vec3f()
    dist_norm_sq = dist_norm * dist_norm
    force = dist_vec * (K / dist_norm_sq)
    return force / dist_norm

def attractive_force(dist_vec, K, L):
    dist_norm = dist_vec.norm()
    if dist_norm == 0: return tlp.Vec3f()
    force = dist_vec * K * (dist_norm - L)
    return force / dist_norm

def run(graph, iterations):
    # constants
    L = 10
    K_r = 6250
    K_s = 1
    R = 0.05
    N = graph.numberOfNodes()    
    t = 200 # cst = 0.04
    t_f = 0.9
    conv_threshold = 6.0 / N
    max_partition_size = 20

    layout = graph.getLayoutProperty("viewLayout")
    disp = graph.getLayoutProperty("disp")
    quit = False
    it = 1

    partitions = compute_partitions_CG(kd_tree_partitioning.run(graph, max_partition_size), layout)

    while not quit:
        total_disp = 0
        
        if N > max_partition_size and (it < 20 or (it > 20 and it % 5 == 0)): # TODO: find a better rate of partitioning
           partitions = compute_partitions_CG(kd_tree_partitioning.run(graph, max_partition_size), layout)
        
        for (p, cg) in partitions:
            for u in p:
                # repulsive forces between nodes and partitions
                for (p2, cg2) in partitions:
                    if p2 == p: continue
                    dist = layout[u] - cg2
                    force = len(p2) * repulsive_force(dist, K_r)
                    disp[u] += force    # TODO: add the size of p2 as a factor (?)
                
                # repulsive forces between nodes of the same partition
                for v in p:
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
        #print("it: {}\ntemp: {}\ntotal disp: {}\n------------------".format(it, t, total_disp))

    print("number if iterations done: {}".format(it))

def main(graph):
    run(graph, ITERATIONS)
    updateVisualization()
