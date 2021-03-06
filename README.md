# Generalized Assignment Solver

A solver for the Generalized Assignment Problem.

This problem is interesting because many different optimization methods can and have been applied to solve it (Branch-and-cut, Branch-and-price, Branch-and-relax, Local search, Constraint programming, Column generation heuristics...). Thus, the main goal of this repository is for me to have reference implementations for classical algorithms and optimization solvers.

## Implemented algorithms

### Lower bounds

- Linear relaxation
  - solved with CLP `-a linrelax_clp` :heavy_check_mark:
  - solved with Gurobi `-a "branchandcut_gurobi --only-linear-relaxation"` :heavy_check_mark:
  - solved with Cplex `-a "branchandcut_cplex --only-linear-relaxation"` :heavy_check_mark:

- Lagrangian relaxation of knapsack constraints. The value of this relaxation is the same as the value of the linear relaxation. However, it might be cheaper to compute, especially on large instances.
  - solved with volume method `-a lagrelax_knapsack_volume` :heavy_check_mark:
  - solved with L-BFGS method `-a lagrelax_knapsack_lbfgs` :heavy_check_mark:

- Lagrangian relaxation of assignment constraints
  - solved with volume method `-a lagrelax_assignment_volume` :heavy_check_mark:
  - solved with L-BFGS method `-a lagrelax_assignment_lbfgs` :heavy_check_mark:

- Column generation `-a "columngeneration --lp-solver clp"` :heavy_check_mark: `-a "columngeneration --lp-solver cplex"` :heavy_check_mark:

### Upper bounds

Polynomial algorithms from "Generalized Assignment Problems" (Martello et al., 1992), options `-f cij` `-f wij` `-f cij*wij` `-f -pij/wij` `-f wij/ti`:
- Basic greedy `-a "greedy -f wij"` :heavy_check_mark:
- Greedy with regret measure `-a "greedyregret -f wij"` :heavy_check_mark:
- MTHG, basic greedy (+ n shifts) `-a "mthg -f wij"` :heavy_check_mark:
- MTHG, greedy with regret measure (+ n shifts) `-a "mthgregret -f wij"` :heavy_check_mark:

Classical local search algorithms based on the shift-swap neighborhood:
- Local search `-a "localsearch --iteration-limit 1000000 --iteration-without-improvment-limit 100000"` :heavy_check_mark:
- Tabu search `-a "tabusearch -l 10000 --iteration-limit 1000000 --iteration-without-improvment-limit 100000"` :heavy_check_mark:
- Simulated annealing `-a "simulatedannealing --beta 0.999 -l 1000000 --iteration-limit 1000000 --iteration-without-improvment-limit 100000"` :heavy_check_mark:

Tree search algorithms based on the Dantzig-Wolfe reformulation branching scheme (i.e. column generation heuristics) inspired by "Primal Heuristics for Branch and Price: The Assets of
Diving Methods" (Sadykov et al., 2019):
- Greedy `-a "cgh_greedy --lp-solver cplex"` :heavy_check_mark:
- Limited discrepency search `-a "cgh_limiteddiscrepencysearch --lp-solver cplex"` :heavy_check_mark:

Others heuristics:
- Random feasible solution found with a Local search `-a random` :heavy_check_mark:
- Local search with LocalSolver `-a localsolver` :heavy_check_mark:

### Exact algorithms

- Branch-and-cut
  - with CBC `-a branchandcut_cbc` :heavy_check_mark:
  - with CPLEX `-a branchandcut_cplex` :heavy_check_mark:
  - with Gurobi `-a branchandcut_gurobi` :heavy_check_mark:

- Branch-and-price
  - `-a "branchandprice --lp-solver cplex --tree-search-algorithm bfs --branching-rule most-fractional"` :heavy_check_mark:
  - `-a "branchandprice --lp-solver cplex --tree-search-algorithm dfs --branching-rule most-integer"` :heavy_check_mark:
  - `-a "branchandprice --lp-solver cplex --tree-search-algorithm lds --branching-rule most-integer"` :heavy_check_mark:

- Constraint programming
  - with Gecode `-a constraintprogramming_gecode` :heavy_check_mark:
  - with CPLEX `-a constraintprogramming_cplex` :heavy_check_mark:

## Usage (command line)

The only required dependency is Boost:
```shell
sudo apt-get install libboost-all-dev
```

Compile:
```shell
bazel build -- //...
```

However, most algorithms require additional libraries (see below).
Compile with additional libraries (or just uncomment the corresponding lines in `.bazelrc`):
```shell
bazel build \
    --define coinor=true \
    --define cplex=true \
    --define gurobi=true \
    --define gecode=true \
    --define dlib=true \
    --define localsolver=true \
    -- //...
```

Solve:
```shell
./bazel-bin/generalizedassignmentsolver/main -v \
    -a 'mthg -f -pij/wij' \
    -i "data/a05100" \
    -o "a05100_output.json" \
    -c "a05100_solution.txt"
```

Unit tests:
```shell
bazel test --compilation_mode=dbg -- //...
```

Checker:
```shell
./bazel-bin/generalizedassignmentsolver/checker data/a05100 output/best/a05100_solution.txt
```

Run benchmarks:
```shell
python3 ../optimizationtools/optimizationtools/bench_run.py --algorithms "mthg -f cij" "mthg -f wij" "mthg -f cij*wij" "mthg -f -pij/wij" "mthg -f wij/ti" "mthgregret -f cij" "mthgregret -f wij" "mthgregret -f cij*wij" "mthgregret -f -pij/wij" "mthgregret -f wij/ti" "random"
python3 ../optimizationtools/optimizationtools/bench_process.py --benchmark heuristicshort --labels "mthg -f cij" "mthg -f wij" "mthg -f cij*wij" "mthg -f -pij/wij" "mthg -f wij/ti" "mthgregret -f cij" "mthgregret -f wij" "mthgregret -f cij*wij" "mthgregret -f -pij/wij" "mthgregret -f wij/ti" "random" --timelimit 0.1
```

![heuristicshort](img/heuristicshort.png?raw=true "heuristicshort")

## Optional dependencies

To enable a dependency, uncomment the corresponding line in the `.bazelrc` file and adapt its path in the `WORKSPACEi` file.

Here are some notes for their installations:

### COIN-OR (CLP, CBC, VOL, DIP)

Install (https://coin-or.github.io/coinbrew/):
```shell
git clone https://www.github.com/coin-or/coinbrew
cd coinbrew
./coinbrew fetch build Vol --no-prompt
```

### Gecode

Download latest version: download https://www.gecode.org/download.html

Compile (more info https://www.gecode.org/doc/2.2.0/reference/PageComp.html):
```shell
./configure
make
```

### Gurobi

After installing, execute the following commands:
```shell
cd ${GUROBI_HOME}/linux64/src/build/
make
cp libgurobi_c++.a ../../lib/
```

