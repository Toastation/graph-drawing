from tulip import tlp
import math
import random

class Merger:

    def __init__(self):
        pass

    def run(graph):
        is_new_node = graph.getBooleanProperty("isNewNode")
        can_move = graph.getBooleanProperty("canMove")
        new_nodes = [n for n in graph.getNodes() if is_new_node[n]]
        
        for n in new_nodes:
            can_move[n] = True
            for neighbor in graph.getInOutNodes(n):
                pass
            

def main(graph):
    pass