# TODO: create a set of randomly generated graphs via the tulip API

from tulip import tlp
import os
import profile
import pstats
import kd_tree_partitioning
import fruchterman_reingold

TESTING_GRAPHS_LOCATION = "../graphs/testing_dataset/"
PROFILING_LOCATION = "../profiling/"
COMPATIBLE_EXT = ["tlp", "tlp.gz", "tlpz", "tlpb", "tlpb.gz", "tlpbz", "json", "gexf", "net", "paj", "gml", "dot", "txt"]

profile_name = []

def load_graphs(graphs):
    files = os.listdir(TESTING_GRAPHS_LOCATION)
    for f in files:
        compatible = False
        for ext in COMPATIBLE_EXT:
            if f.endswith(ext): 
                compatible = True
                break
        if not compatible: 
            print("File \"{}\" not loaded: incompatible extension.".format(f))
            continue
        graph = tlp.loadGraph(TESTING_GRAPHS_LOCATION+f)
        if not graph:
            print("Failed to load graph \"{}\"".format(f))
        else: graphs.append(graph)  

def profile_kd_tree_partitioning(graphs):
    global profile_name
    save_path = PROFILING_LOCATION+"profiling_kd_tree_part_"
    max_partition_size = 10
    for i in graphs:
        save_name = save_path+"n{}_e{}_p{}".format(i.numberOfNodes(), i.numberOfEdges(), max_partition_size)
        profile_name.append(save_name)
        profile.runctx("kd_tree_partitioning.run(graph, max_partition_size)", globals(), {"graph":i, "max_partition_size":max_partition_size}, save_name)

def profile_fr(graphs):
    global profile_name
    save_path = PROFILING_LOCATION+"profiling_fr_"
    iterations = 100
    for i in graphs:
        save_name = save_path+"n{}_e{}".format(i.numberOfNodes(), i.numberOfEdges())
        profile_name.append(save_name)
        profile.runctx("fruchterman_reingold.run(graph, iterations)", globals(), {"graph":i, "iterations":iterations}, save_name)

def main():
    graphs = []
    load_graphs(graphs)
    
    profile_kd_tree_partitioning(graphs)
    profile_fr(graphs)

    # prints all profilings
    for i in profile_name: 
        p = pstats.Stats(i)
        p.strip_dirs().sort_stats("tottime").print_stats()

if __name__ == "__main__": main()