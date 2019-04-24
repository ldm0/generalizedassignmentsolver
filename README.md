WORK IN PROGRESS

# Generalized Assignment Problem

Algorithm implementations for the Generalized Assignment Problem.

Compile:
```
bazel build --cxxopt='-std=c++14' --compilation_mode=opt -- //...
```

Solve:
```
./bazel-bin/lib/main -v -a milp -i data/a05100 -o out.ini -c sol.txt
```

Unit tests:
```
bazel test --cxxopt='-std=c++14' --compilation_mode=opt -- //...
```

## Lower bounds

- Lagrangian relaxation solved with volume method :x:
- Lagrangian relaxation solved with bundle method :x:
- Column generation :x:

## Upper bounds

- Random initial solution `-a random` :heavy_check_mark:
- Simple hill climbing `-a lssimple` :heavy_check_mark:

## Exact algorithms

- MILP solved with CBC `-a milp` :heavy_check_mark:
- Branch-and-price :x:

## Results


`lssimple` does not compete with state of the art heuristics like the ones presented in "A path relinking approach with ejection chains for the generalized assignment problem" (Yagiura, 2006) or "Variable-fixing then subgradient optimization guided very large scale neighborhood search for the generalized assignment problem" (Haddadi, 2018). However:
- on a short time run (2 minutes, Processor Intel® Core™ i5-8500 CPU @ 3.00GHz × 6), it provides good solutions (less than 1% gap from optimal for most instances of the literature, between 1% and 2% for some hard instances)
- it is very simple and the implementation is very short
- it is available and free (MIT License)

