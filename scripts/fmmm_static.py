from fmmm_multilevel import Multilevel
from layout import FMMMLayout2

class FMMMStatic:
      
    def __init__(self):
        self._multilevel = Multilevel()
        self._layout = FMMMLayout2()
        self.init_constants()

    def init_constants(self):
        self._layout.init_constants()
        self._iterations = 300
        self._condition = None
        self._const_temp = False
        self._temp_init_factor = 0.2

    def run(self, graph, multilevel=False):
        if multilevel:
            self._multilevel.run(graph)
        else:
            self._layout.run(graph, self._iterations, self._condition, self._const_temp, self._temp_init_factor)

def main(graph):
    static_layout = FMMMStatic()
    static_layout.run(graph, False)
