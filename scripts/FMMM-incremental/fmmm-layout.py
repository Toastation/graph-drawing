from tulip import tlp

DEFAULT_ITERATIONS = 1000
REPL_CONST = 4.0
DL = 0.055

class FMMMLayout:

    def __init__(self):
        self._default_iterations = DEFAULT_ITERATIONS
        self._repl_const = REPL_CONST
        self._desired_edge_length = DL

    def _repulsive_force(self, dist):
        return (self._repl_const * dist) / (dist.norm() ** 3)

    def _attractive_force(self, dist):
        dist_norm = dist.norm()
        return dist_norm * math.log(dist_norm / self._desired_edge_length) * dist

    def _compute_node(graph, node):
        pos = graph.getLayoutProperty("viewLayout")
        if node.vertices:
                print("Cannot compute kd tree node {}: no vertices".format(node))
            return False
        (center, farthest_point) = tlp.computeBoundingRadius(graph)
        node.center = center
        node.radius = self.center.dist(farthest_point)



    def _create_kd_tree(graph):
        root = KDTree(None, graph.nodes())


        
    def run(self, graph, iterations = self._default_iterations):
        pass