from tulip import tlp
from fmmm_layout import FMMMLayout, FMMMLayout2

class Refinement:

    def __init__(self):
        self._force_model = FMMMLayout2()
        self._high_energy_threshhold = 1
        self._iterations = 20
        self._init_temp = 1

    ## \brief Computes the energy of every node in graph and stores values in the "energy" property
    # \param graph The graph from which to compute the energy
    # \return The total energy of the graph
    def _compute_nodes_energy(self, graph):
        energy = graph.getDoubleProperty("energy")
        pos = graph.getLayoutProperty("viewLayout")
        total = 0
        for u in graph.getNodes(): # repulsive energy (TODO: use multipole strategy)
            for v in graph.getNodes():
                if u != v:
                    dist = pos[u] - pos[v]
                    energy_value = self._force_model.repulsive_force_intgr(dist)
                    energy[u] += energy_value
                    total += energy_value
        for e in graph.getEdges(): # attractive energy
            source = graph.source(e)
            target = graph.target(e)
            dist = pos[source] - pos[target]
            energy_value = self._force_model.attractive_force_intgr(dist) 
            energy[source] += energy_value
            energy[target] += energy_value
            total += 2 * energy_value   
        return total

    def _mark_high_energy_nodes(self, graph):
        energy = graph.getDoubleProperty("energy")        
        high_energy = graph.getBooleanProperty("highEnergy")
        total = self._compute_nodes_energy(graph)
        mean_energy = total / graph.numberOfNodes()
        for n in graph.getNodes():
            if abs(energy[n] - mean_energy) / mean_energy > self._high_energy_threshhold:
                high_energy[n] = True

    def _move_high_energy_nodes(self, graph):
        self._force_model.set_init_temp(self._init_temp)
        self._force_model.run(graph, self._iterations, graph.getBooleanProperty("highEnergy"))

    def run(self, graph):
        self._mark_high_energy_nodes(graph)
        self._move_high_energy_nodes(graph)
