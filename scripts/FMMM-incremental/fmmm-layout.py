from tulip import tlp
from anytree import NodeMixin
import math

DEFAULT_ITERATIONS = 1000
REPL_CONST = 4.0
DL = 0.055
P = 4   # P-term multipole expansion

class KDTree(NodeMixin):
    _p = P

    def __init__(self, parent, vertices):
        super(NodeMixin, self).__init__()
        self.parent = parent
        self.vertices = vertices
        self._compute()

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

        # compute potential of vertices
        


    def _create_kd_tree(graph):
        root = KDTree(None, graph.nodes())


        
    def run(self, graph, iterations = self._default_iterations):
        pass