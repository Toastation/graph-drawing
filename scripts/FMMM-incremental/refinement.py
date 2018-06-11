from tulip import tlp
from fmmm_layout import FMMMLayout, FMMMLayout2

class Refinement:

    def __init__(self):
        self._force_model = FMMMLayout2()

    ## \brief Computes the energy of every node in graph and stores values in the "energy" property
    # \param graph The graph from which to compute the energy
    def _compute_nodes_energy(self, graph):
        energy = graph.getDoubleProperty("energy")
        pos = graph.getLayoutProperty("viewLayout")
        for u in graph.getNodes(): # repulsive energy (TODO: use multipole strategy)
            for v in graph.getNodes():
                if u != v:
                    dist = pos[u] - pos[v]
                    energy[u] += self._force_model.repulsive_force_intgr(dist)
        for e in graph.getEdges(): # attractive energy
            source = graph.source(e)
            target = graph.target(e)
            dist = pos[source] - pos[target]
            energy_value = self._force_model.attractive_force_intgr(dist) 
            energy[source] -= energy_value
            energy[target] += energy_value

    def _mark_high_energy_nodes(self, graph):
        self._compute_nodes_energy(graph)

    def _move_high_energy_nodes(self, graph):
        pass

    def run(self, graph):
        pass
