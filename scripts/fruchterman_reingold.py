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
        
def fr_2(graph, iterations):
    layout = graph.getLayoutProperty("viewLayout")
    forces = graph.getLayoutProperty("forces")
    pinning_weights = graph.getDoubleProperty("pinningWeight")

    min_partition_size = 1000
    frac_done = 0
    delta_frac = 1 / (iterations)
    K = 10 # optimal geometric node distance
    temp = K * math.sqrt(graph.numberOfNodes())
    temp_decay = 0.9 
    
    
    if graph.numberOfNodes() > 1000:
        partitions = compute_partitions_CG(kd_tree_partitioning.run(graph, min_partition_size), layout)
    else:
        partitions = [(graph.getNodes(), tlp.Vec3f())]

    for i in range(iterations):
        for (p, cg) in partitions:
            for n in p:
                if frac_done > pinning_weights[n]:
                    # computing repulsive forces for neighbor inside the partition
                    #for n2 in p:
                    #   if n != n2:
                    #       delta_pos = layout[n] - layout[n2]
                    #       dist = delta_pos.norm()
                    #       forces[n] += (K * K) * (delta_pos / (dist * dist))
                
                    # computing repulsive forces between the node and the other partitions
                    for (p2, cg2) in partitions:
                        if p2 != p:
                            delta_pos = layout[n] - cg2
                            dist = delta_pos.norm()
                            forces[n] += K * K * len(p2) * (delta_pos / (dist * dist))

        # computing attractives forces for every edge
        for e in graph.getEdges():
            source = graph.source(e)
            target = graph.target(e)
            delta_pos = layout[source] - layout[target]
            force = (delta_pos.norm() / K) * delta_pos
            if frac_done > pinning_weights[source]:
                forces[source] -= force
            if frac_done > pinning_weights[target]:            
                forces[target] += force

        # updating positions
        for n in graph.getNodes():
            force = forces[n].norm()
            if force != 0:
                layout[n] += (forces[n] / force) * min(temp, force)  
        temp *= temp_decay
        frac_done += delta_frac
        #print("frac done {}".format(frac_done))
        #print("temp {}".format(temp))      

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
    max_disp_sq = 100
    conv_threshold = 1

    N = graph.getNodes()
    layout = graph.getLayoutProperty("viewLayout")
    disp = graph.getLayoutProperty("disp")
    bounding_box = tlp.computeBoundingBox(graph)
    t = 100 # cst = 0.04
    t_f = 0.9

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
            total_disp += disp_norm
            disp[n] = (disp[n] / disp_norm) * min(disp_norm, t)
            # disp_sq = disp_norm * disp_norm
            # if disp_sq > max_disp_sq:
            #     s = math.sqrt(max_disp_sq / disp_sq)
            #     disp[n] *= s
            layout[n] += disp[n]
            if total_disp < conv_threshold: quit = True
        quit = it > iterations or quit # maybe infinite if conv_threshold is too low...
        it += 1
        t *= t_f
        print(t)
    print("number if iterations done: {}".format(it))

def main(graph):
    run(graph, ITERATIONS)
    updateVisualization()   
