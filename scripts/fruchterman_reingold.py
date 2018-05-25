from tulip import tlp
import math
import kd_tree_partitioning

ITERATIONS = 1000

## computes the center of gravity for each partition (added as graph attribute "centerOfGravity")
def compute_partitions_CG(partitions, layout):
    for p in partitions:
        cg = tlp.Vec3f()
        for n in p.getNodes():
            cg += layout[n]
        p.setAttribute("centerOfGravity", cg / p.numberOfNodes())

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
    t = 100 # cst = 0.04
    t_f = 0.9
    max_disp_sq = 100
    conv_threshold = 6.0 / N
    min_partition_size = 10

    layout = graph.getLayoutProperty("viewLayout")
    disp = graph.getLayoutProperty("disp")
    quit = False
    it = 1

    if N > 20:
        partitions = kd_tree_partitioning.run(graph, min_partition_size)
    else: 
        partitions = [graph.nodes()]
    partitions = compute_partitions_CG(partitions, layout)

    while not quit:
        total_disp = 0
        
        if N > 20 and (it < 20 or (it > 20 and it % 5 == 0)): # TODO: find a better rate of partitioning
            partitions = compute_partitions_CG(kd_tree_partitioning.run(graph, min_partition_size), layout)
        
        for (p, p_cg) in partitions:
            for u in p:
                # repulsive forces between nodes and partitions
                for (p2, p2_cg) in partitions:
                    if p2 == p: continue
                    dist = layout[u] - p2_cg
                    force = repulsive_force(dist, K_r)
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
        print("it: {}\ntemp: {}\ntotal disp: {}\n------------------".format(it, t, total_disp))
    print("number if iterations done: {}".format(it))

def main(graph):
    run(graph, ITERATIONS)
    updateVisualization()   
