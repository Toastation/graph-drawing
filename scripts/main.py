from tulip import tlp
import pinning_weights
import coarsening
import fruchterman_reingold

def main(graph):
    pinning_weights.run(graph)
    coarsening.run(graph)

    