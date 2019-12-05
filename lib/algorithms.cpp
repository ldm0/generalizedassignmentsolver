#include "generalizedassignment/lib/algorithms.hpp"

#include "generalizedassignment/lb_linrelax_clp/linrelax_clp.hpp"
#include "generalizedassignment/lb_linrelax_gurobi/linrelax_gurobi.hpp"
#include "generalizedassignment/lb_lagrelax_volume/lagrelax_volume.hpp"
#include "generalizedassignment/lb_lagrelax_bundle/lagrelax_bundle.hpp"
#include "generalizedassignment/lb_lagrelax_lbfgs/lagrelax_lbfgs.hpp"
#include "generalizedassignment/lb_columngeneration/columngeneration.hpp"
#include "generalizedassignment/opt_branchandcut_cbc/branchandcut_cbc.hpp"
#include "generalizedassignment/opt_branchandcut_cplex/branchandcut_cplex.hpp"
#include "generalizedassignment/opt_branchandcut_gurobi/branchandcut_gurobi.hpp"
#include "generalizedassignment/opt_branchandprice/branchandprice.hpp"
#include "generalizedassignment/opt_constraintprogramming_gecode/constraintprogramming_gecode.hpp"
#include "generalizedassignment/opt_constraintprogramming_cplex/constraintprogramming_cplex.hpp"
#include "generalizedassignment/ub_random/random.hpp"
#include "generalizedassignment/ub_greedy/greedy.hpp"
#include "generalizedassignment/ub_repair/repair.hpp"
#include "generalizedassignment/ub_ls_shiftswap/ls_shiftswap.hpp"
#include "generalizedassignment/ub_localsolver/localsolver.hpp"
#include "generalizedassignment/ub_colgenheuristics/colgenheuristics.hpp"

#include <map>

using namespace generalizedassignment;

std::function<Output (Instance&, std::mt19937_64&, Info)> generalizedassignment::get_algorithm(std::string str)
{
    benchtools::Algorithm algo(str);

    if (algo.name == "") {
        std::cerr << "\033[32m" << "ERROR, missing algorithm." << "\033[0m" << std::endl;
        return [](Instance& ins, std::mt19937_64&, Info info) { return Output(ins, info); };

    /*
     * Lower bounds
     */
#if COINOR_FOUND
    } else if (algo.name == "linrelax_clp") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_linrelax_clp(ins, info);
        };
#endif
#if GUROBI_FOUND
    } else if (algo.name == "linrelax_gurobi") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_linrelax_gurobi(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "lagrelax_knapsack_volume") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_lagrelax_knapsack_volume(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "lagrelax_knapsack_bundle") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_lagrelax_knapsack_bundle(ins, info);
        };
#endif
#if DLIB_FOUND
    } else if (algo.name == "lagrelax_knapsack_lbfgs") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_lagrelax_knapsack_lbfgs(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "lagrelax_assignment_volume") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_lagrelax_assignment_volume(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "lagrelax_assignment_bundle") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return lb_lagrelax_assignment_bundle(ins, info);
        };
#endif
#if DLIB_FOUND
    } else if (algo.name == "lagrelax_assignment_lbfgs") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            LagRelaxAssignmentLbfgsOptionalParameters p;
            p.info = info;
            return lb_lagrelax_assignment_lbfgs(ins, p);
        };
#endif
    } else if (algo.name == "columngeneration") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            ColGenOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return lb_columngeneration(ins, p);
        };

    /*
     * Exact algorithms
     */
#if COINOR_FOUND
    } else if (algo.name == "branchandcut_cbc") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            BranchAndCutCbcOptionalParameters p;
            p.info = info;
            return sopt_branchandcut_cbc(ins, p);
        };
#endif
#if CPLEX_FOUND
    } else if (algo.name == "branchandcut_cplex") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            BranchAndCutCplexOptionalParameters p;
            p.info = info;
            return sopt_branchandcut_cplex(ins, p);
        };
#endif
#if GUROBI_FOUND
    } else if (algo.name == "branchandcut_gurobi") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            BranchAndCutGurobiOptionalParameters p;
            p.info = info;
            return sopt_branchandcut_gurobi(ins, p);
        };
#endif
    } else if (algo.name == "branchandprice_dfs") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            BranchAndPriceOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sopt_branchandprice_dfs(ins, p);
        };
    } else if (algo.name == "branchandprice_astar") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            BranchAndPriceOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sopt_branchandprice_astar(ins, p);
        };
#if GECODE_FOUND
    } else if (algo.name == "constraintprogramming_gecode") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            ConstraintProgrammingGecodeOptionalParameters p;
            p.info = info;
            return sopt_constraintprogramming_gecode(ins, p);
        };
#endif
#if CPLEX_FOUND
    } else if (algo.name == "constraintprogramming_cplex") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            ConstraintProgrammingCplexOptionalParameters p;
            p.info = info;
            return sopt_constraintprogramming_cplex(ins, p);
        };
#endif

    /*
     * Upper bounds
     */
    } else if (algo.name == "random") {
        return [](Instance& ins, std::mt19937_64& gen, Info info) {
            return sol_random(ins, gen, info);
        };
    } else if (algo.name == "greedy") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            auto it = algo.args.find("f");
            std::string des_str = (it == algo.args.end())? "cij": it->second;
            std::unique_ptr<Desirability> f = desirability(des_str, ins);
            return sol_greedy(ins, *f, info);
        };
    } else if (algo.name == "greedyregret") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            auto it = algo.args.find("f");
            std::string des_str = (it == algo.args.end())? "cij": it->second;
            std::unique_ptr<Desirability> f = desirability(des_str, ins);
            return sol_greedyregret(ins, *f, info);
        };
    } else if (algo.name == "mthg") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            auto it = algo.args.find("f");
            std::string des_str = (it == algo.args.end())? "cij": it->second;
            std::unique_ptr<Desirability> f = desirability(des_str, ins);
            return sol_mthg(ins, *f, info);
        };
    } else if (algo.name == "mthgregret") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            auto it = algo.args.find("f");
            std::string des_str = (it == algo.args.end())? "cij": it->second;
            std::unique_ptr<Desirability> f = desirability(des_str, ins);
            return sol_mthgregret(ins, *f, info);
        };
#if COINOR_FOUND
    } else if (algo.name == "repaircombrelax") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return sol_repaircombrelax(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "repairgreedy") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            return sol_repairgreedy(ins, info);
        };
#endif
#if COINOR_FOUND
    } else if (algo.name == "repairlinrelax_clp") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            LinRelaxClpOutput linrelax_output = lb_linrelax_clp(ins);
            return sol_repairlinrelax_clp(ins, linrelax_output, info);
        };
#endif
    } else if (algo.name == "ls_shiftswap") {
        return [algo](Instance& ins, std::mt19937_64& gen, Info info) {
            LSShiftSwapOptionalParameters p;
            p.info = info;
            return sol_ls_shiftswap(ins, gen, p);
        };
    } else if (algo.name == "ts_shiftswap") {
        return [algo](Instance& ins, std::mt19937_64& gen, Info info) {
            TSShiftSwapOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sol_ts_shiftswap(ins, gen, p);
        };
    } else if (algo.name == "sa_shiftswap") {
        return [algo](Instance& ins, std::mt19937_64& gen, Info info) {
            SAShiftSwapOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sol_sa_shiftswap(ins, gen, p);
        };
#if LOCALSOLVER_FOUND
    } else if (algo.name == "localsolver") {
        return [](Instance& ins, std::mt19937_64&, Info info) {
            LocalSolverOptionalParameters p;
            p.info = info;
            return sol_localsolver(ins, p);
        };
#endif
    } else if (algo.name == "cgh_restrictedmaster") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            CghRestrictedMasterOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sol_cgh_restrictedmaster(ins, p);
        };
    } else if (algo.name == "cgh_purediving") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            CghPureDivingOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sol_cgh_purediving(ins, p);
        };
    } else if (algo.name == "cgh_divingwithlds") {
        return [algo](Instance& ins, std::mt19937_64&, Info info) {
            CghDivingWithLdsOptionalParameters p;
            p.info = info;
            p.set_params(algo.args);
            return sol_cgh_divingwithlds(ins, p);
        };


    } else {
        std::cerr << "\033[31m" << "ERROR, unknown algorithm: " << algo.name << "\033[0m" << std::endl;
        assert(false);
        return [](Instance& ins, std::mt19937_64&, Info info) { return Output(ins, info); };
    }
}

