from tulip import tlp
import math

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

    def run(self, graph, iterations=self._default_iterations):
        pass

    