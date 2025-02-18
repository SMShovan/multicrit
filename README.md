# Engineering Parallel Biobjective Shortest Path Search

## Abstract

The multi-criteria shortest path problem is concerned with the minimization of multiple, possibly conflicting objectives such as travel time and highway tolls. Sanders and Mandow present a parallel algorithm for this problem. They generalize Dijkstra’s single-criterion shortest path algorithm to multiple criteria, while parallelizing all additional work induced by this generalization. For the bi-criteria case they even show that their algorithm performs less work than classic bi-criteria algorithms.

We discuss in how far these theoretical results translate into an efficient implementation for modern shared-memory multiprocessors. In particular, we focus on a cache-efficient implementation of the bi-criteria case and present an extensive evaluation of the algorithm and its underlying data structures.

The evaluation indicates that parallel bi-criteria shortest path search is feasible in practice. Our implementation exhibits significant speedups when solving difficult problem instances in parallel. In addition, even a sequential implementation is competitive to classic bi-criteria shortest path algorithms.

As an additional contribution of independent interest, we present parallel bulk updates for weight-balanced B-trees. We use this technique to implement the Pareto queue, a multi-dimensional generalization of a priority queue proposed by Sanders and Mandow and required by their algorithm.


## Code Overview

`BiCritShortestPathAlgorithm.hpp` is the shortest path algorithm interface to be used by test and benchmarking clients. The actual algorithm is set and configured in `options.hpp`.

* `msp_classic/`: Various versions of a sequential state-of the art label setting algorithm.
* `msp_pareto/`: Sequential and parallel implementation of the PaPaSearch algorithm of Sanders and Mandow [2013], i.e., a label setting algorithm that is based on the novel idea to process all non-domianted labels within a single iteration. The algorithm depends on the corresponding `ParetoQueue` implementation.
* `datastructures/btree/`: In-memory B-tree with parallel and sequential bulk updates (insertions and deletions). The B-tree is central to our `ParetoQueue` implementation. 


## Build 

Within a bash shell:

    source ../lib/tbb/bin/tbbvars.sh intel64

Modify `src/options.hpp` to configure the algorithm / data structures. Parallel versions of the different algorithm and timing programs end in `.par`, e.g., `./bin/time_road_instances1.par`.
  
    make configure      # only needed once
    make 
    make all DEBUG=yes

    make my_program     # to build a specific executable
    make my_program.par # to build the parallel version of a specific executable


## Running benchmarks

Within `scripts/` run `build_binaries.sh` to configure &andgenerate all binaries used for benchmarking. To run benchmarks, execute the corresponding scripts within the `scripts/` folder. Benchmarking scripts write their timing values to  `timings/`. To produce new graphs, run the makefiles in the different subdirectories of the `gnuplot/` folder.

The benchmarking scripts execute the different benchmarks `src/time_*.cpp`:

* Road1: Simple (time/distance) maps of [Raith, Ehrgott 2009]. To run, extract the DC, RI and NJ tar files in `instances/`
* Road2: Hard (time/economic cost) map of [Machuca 2012]. Extract the NY tar file in `instances/`.
* Grid1: Random grid graphs of [Raith, Ehrgott 2009]. Costs in range [1, 10]
* Grid2: Random grid graphs with tunable difficulties. Important options: `-q X` configures the correlation of the edge weights (e.g., 0.8, 0.4, 0, -0,4, -0.8) and `-m X` sets the upper bound of the cost range [0, X]. We refer to `-m 10` as simple and to `-m 10` as hard.

Common options the shortest path benchmarks:

    -c number of repetitions
    -v enable statistics
    -p PE count

Common options of the BTree / Pareto Queue benchmarks  (e.g., `time_pq_btree.cpp`):

    -c number of repetitions
    -k number of elements to insert
    -r ratio where n stands for the pre-existing elements within the treee for n = k * r. So, r=0 is a bulk construction.
    -p PE count


## Known Issues

* __BTree Implementation Bug__: Subtrees of an inner-node are rebalanced by reconstruction if they either underflow or overflow. If there is an underflow which prevents us from recreating a new subtree with a sufficient number of elements, we also include the elements of a neighboring subtree that does not need to be rebalanced otherwise. However, in our implementation, we can only include subtrees to the _right_, so we might fail to create a properly balanced subtree if the _last_ subtree of a node underflows without any directly preceding subtrees to be rebuild as well. This bug can lead to slightly imbalanced trees, but we have not noticed any performance or correctness problems. However, please note that the tree might throw assertion errors if run in debug mode. 

* __BTree Code Duplication__: The `BTree_base` class is duplicated. One copy is used as the basis of label sets the other one used by pareto queue. We should probably make the slight implementation differences configurable via template parameters (e.g., to only have the COMPUTE_PARETO_MIN macro dependency if needed).

* __Outdated code__: Certain experimental implementations and options turned out not to be competitive (e.g., to use an std::set as a label set implementation). They have been abandoned long ago and might no longer work or even compile. 

* __Failing tests__: Tests have not been adapted to work with all different implementation options. For example, the tests are know to fail for the `SplittedNaiveLabelSet`.

* __Inconsistent Parameter Locations__: When updating the B-tree parameters, not only the code but also the build script has to be updated.

* __Inconsitent Results__:  Concerning the NY road map, we got slightly different results than Enrique Machucha and Lawrance Mandow in their publications (NY7 274 =/= 272, NY8 7397 =/= 7391, NY12 1571 =/= 1573, NY14 2871 =/= 2957). A discussion is ongoing (latest sync on Wed, 09 Oct 2013).
                         
