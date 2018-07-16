Description
---
The project is made of 2 [Tulip](http://tulip.labri.fr/) plugins. They can be run on all types of 2D graphs, but for now their paralellized version can only be run on simple graphs.

* _Incremental_

   This plugin computes the layout of a dynamic graph. The format of a dynamic graph is specified in the later section "Dynamic graph format".

* _Custom Layout_

   This plugin computes the static layout of a graph.

You can also find a Python script `src\morph.py` that runs an animation of a dynamic graph after it has been processed by the Incremental plugin.


Compiling the plugins
---
To compile the plugins, place the files ```src/custom_layout.cpp```, ```src/custom_layout.h```, ```src/incremental.cpp``` and ```src/incremental.h``` in the externalplugins directory of the tulip sources, and recompile. You can also use the shell scripts ```src/comp_*.sh```.

Dynamic graph format (graph hierarchy): 
---
In our case a dynamic graph is represented via a certain graph hierarchy in Tulip.  
The root graph must contain all the nodes and edges of the timeline. The subgraphs of this root graph represent the differents steps of the timeline.  
If a node or edge exists in both step i and i+1 of the timeline then it must be the same shared tulip node/edge.  
For now, the direction of the timeline follows the increasing order of the subgraphs id.  

![Graph hierarchy example](https://i.imgur.com/1Sj3qLQ.png "Graph hierarchy example")

How to use:
---
* Use the plugin "Incremental" on the root graph of a timeline. This will compute the layout of all of its subgraphs. The layout of each step of the timeline is stored in the local "viewLayout" property of the subgraphs.

* Use the script ```scripts/morph.py``` on the root graph of a timeline already processed by the _Incremental_ plugin to run an animation of the dynamic graph. The animation stops at each steps so be sure to press continue.
