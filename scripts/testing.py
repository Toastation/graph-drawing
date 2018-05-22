# TODO: create a set of randomly generated graphs via the tulip API

from tulip import tlp
import profile
import pstats
import kd_tree_partitioning
import fruchterman_reingold

TESTING_GRAPHS_LOCATION = "../graphs/testing_dataset/"
PROFILING_LOCATION = "../profiling/"
GRAPHS_NAME = ["rgg_500_1000.tlp", "rgg_50_100.tlp"]

profile_name = []

def load_graphs(graphs):
    for i in GRAPHS_NAME:
        graph = tlp.loadGraph(TESTING_GRAPHS_LOCATION+i)
        if not graph:
            print("FAILED TO LOAD GRAPH \"{}\"".format(i))
        else: graphs.append(graph)

def profile_kd_tree_partitioning(graphs):
    global profile_name
    save_path = PROFILING_LOCATION+"profiling_kd_tree_part_"
    min_partition_size = 5
    for i in graphs:
        save_name = save_path+"n{}_e{}_p{}".format(i.numberOfNodes(), i.numberOfEdges(), min_partition_size)
        profile_name.append(save_name)
        profile.runctx("kd_tree_partitioning.run(graph, min_partition_size)", globals(), {"graph":i, "min_partition_size":min_partition_size}, save_name)

def profile_fr(graphs):
    global profile_name
    save_path = PROFILING_LOCATION+"profiling_fr_"
    iterations = 100
    for i in graphs:
        save_name = save_path+"n{}_e{}".format(i.numberOfNodes(), i.numberOfEdges())
        profile_name.append(save_name)
        profile.runctx("fruchterman_reingold.main(graph, iterations)", globals(), {"graph":i, "iterations":iterations}, save_name)

def main():
    graphs = []
    load_graphs(graphs)
    
    profile_kd_tree_partitioning(graphs)
    #profile_fr(graphs) too slow for this set of graphs TODO:create a better one

    # prints all profilings
    for i in profile_name: 
        p = pstats.Stats(i)
        p.strip_dirs().sort_stats(-1).print_stats()

if __name__ == "__main__": main()