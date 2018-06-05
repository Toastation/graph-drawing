from tulip import tlp
from multipole_expansion import KDTree, MultipoleExpansion

DEFAULT_ITERATIONS = 1000
REPL_CONST = 4.0
DL = 0.055

class FMMMLayout:

    def __init__(self):
        self._default_iterations = DEFAULT_ITERATIONS
        self._repl_const = REPL_CONST
        self._desired_edge_length = DL
        self._multipole_exp = MultipoleExpansion()

    def _repulsive_force(self, dist):
        return (self._repl_const * dist) / (dist.norm() ** 3)

    def _attractive_force(self, dist):
        dist_norm = dist.norm()
        return dist_norm * math.log(dist_norm / self._desired_edge_length) * dist

    def _compute_repulsive_forces(graph, vertex, node):
        pos = graph.getLayoutProperty("viewLayout")
        disp = graph.getLayoutProperty("disp")
        repl_sum = tlp.Vec3f()
        def aux(node):
            z = complex(coord.x(), coord.y())
            dist = abs(z - node.center)
            if dist > node.radius: # compute approximate repl forces
                disp += node.subgraph.numberOfNodes() * self._repulsive_force(dist)

            else:
                if node.is_leaf: # compute exact repl forces
                    for v in node.vertices:
                        if v != vertex:
                            d = pos[vertex] - pos[v]
                            disp[vertex] += self._repulsive_force(d)
                else:
                    repl_sum += aux(node.children[0])
                    repl_sum += aux(node.children[1])

    def run(self, graph, iterations = self._default_iterations):
        pos = graph.getLayoutProperty()

        for i in range(iterations):
            if i <= 3 or i % 20 == 0: # recompute the tree for the first 4 iterations and then every 20 iterations
                kd_tree = self._multipole_exp.build_tree(graph)
        
            for n in graph.getNodes():

        else: # compute exact force
                    