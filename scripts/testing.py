from tulip import tlp
import profile
import pstats
import kd_tree_partitioning
import fruchterman_reingold

TESTING_GRAPHS_LOCATION = "../graphs/testing/"
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
    for i in graphs:
        save_name = save_path+"n{}_e{}".format(i.numberOfNodes(), i.numberOfEdges())
        profile_name.append(save_name)
        profile.runctx("kd_tree_partitioning.main(graph)", globals(), {"graph":i}, save_name)

def profile_fr(graphs):
    global profile_name
    save_path = PROFILING_LOCATION+"profiling_fr_"
    for i in graphs:
        save_name = save_path+"n{}_e{}".format(i.numberOfNodes(), i.numberOfEdges())
        profile_name.append(save_name)
        profile.runctx("fruchterman_reingold.main(graph)", globals(), {"graph":i}, save_name)

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