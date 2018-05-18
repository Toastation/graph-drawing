from tulip import tlp
from collections import deque
import math

ITERATIONS = 4
PARTITION_SIZE = 5

def main(graph):
    layout = graph.getLayoutProperty("viewLayout")
    partitions = deque([graph.nodes()])
    quit = False

    while not quit:
        nb_partitions = len(partitions)
        for i in range(nb_partitions):
            p = partitions.popleft()
            p_size = len(p)
            
            # sort the partition by x and y coord alternatively
            p.sort(key=lambda pos: layout[pos].x()) if count % 2 == 0 else p.sort(key=lambda pos: layout[pos].y()) 

            # cut the partition in two and push the 2 new partitions in the queue
            median_index = p_size // 2
            partitions.append(p[:median_index])
            partitions.append(p[median_index:])
            
            # terminate when we have reached the ideal partition size 
            if p_size // 2 <= PARTITION_SIZE: 
                quit = True
    
    partitions = list(partitions)
    print(partitions)
    
