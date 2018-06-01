from tulip import tlp
import random

COARSET_ITERATIONS = 250
FINEST_ITERATIONS = 30
VERTEX_NB_THRESHOLD = 50

class Multilevel():

    def __init__(self):
        self._coarsest_iterations = COARSET_ITERATIONS
        self._finest_iterations = FINEST_ITERATIONS
        self._vertex_nb_threshold = VERTEX_NB_THRESHOLD

    ## \brief Computes a maximum independent set from a list of nodes
    # \param nodes The list of nodes to compute the independant set from
    def _compute_maximum_indepentent_set(self, graph):
        nodes = graph.nodes()
        ind_set = []
        while len(nodes) != 0:
            node = random.choice(nodes)
            ind_set.append(node)
            nodes.remove(node)
            for n in graph.getInOutNodes(node):
                if n in nodes: nodes.remove(n)
        return ind_set
    
    def _interpolate_init_pos(graph):
          pass

    def run(self, graph):
        current_level = graph
        level = 0
        while current_level.numberOfNodes() > self._vertex_nb_threshold:
            current_level = current_level.inducedSubGraph(self._compute_maximum_indepentent_set(current_level), None, "level_{}".format(level))
            level += 1
            break

# Tulip script
def main(graph):
    m = Multilevel()
    m.run(graph)
