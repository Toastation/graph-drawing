from tulip import tlp
import pinning_weights
import multilevel
import fruchterman_reingold 

def main(root_graph):
    pinning_weights.run(root_graph)
    #graph = multilevel.run(root_graph, multilevel.MAX_ITERATIONS)
    #metanodes = [n for n in graph.getNodes() if graph.isMetaNode(n)]
    fruchterman_reingold.run(root_graph, fruchterman_reingold.ITERATIONS)
    
