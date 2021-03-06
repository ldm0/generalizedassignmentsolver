#include "generalizedassignmentsolver/algorithms/columngeneration.hpp"

#if CPLEX_FOUND
#include <ilcplex/ilocplex.h>
#endif

#if COINOR_FOUND
#include <coin/ClpModel.hpp>
#include <coin/OsiClpSolverInterface.hpp>
#endif

using namespace generalizedassignmentsolver;

class ColGenSolver
{

public:

    virtual ~ColGenSolver() { }
    virtual void add_column(std::vector<int> rows, Cost c) = 0;
    virtual void solve() = 0;
    virtual double objective() = 0;
    virtual std::vector<double>& dual_solution() = 0;
    virtual std::vector<double> solution() = 0;
};

#if COINOR_FOUND

class ColGenSolverClp: public ColGenSolver
{

public:

    ColGenSolverClp(AgentIdx agent_constraint_number, ItemIdx item_constraint_number):
        m_(agent_constraint_number),
        n_(item_constraint_number),
        ones_(m_ + n_, 1),
        dual_sol_vd_(m_ + n_)
    {
        model_.messageHandler()->setLogLevel(0);
        for (AgentIdx i = 0; i < agent_constraint_number; ++i)
            model_.addRow(0, NULL, NULL, 0, 1);
        for (ItemIdx j = 0; j < item_constraint_number; ++j)
            model_.addRow(0, NULL, NULL, 1, item_constraint_number);
    }

    virtual ~ColGenSolverClp() { }

    void add_column(std::vector<int> rows, Cost c)
    {
        model_.addColumn(rows.size(), rows.data(), ones_.data(), 0.0, 1, c);
    }

    void solve() { model_.primal(); }

    double objective() { return model_.objectiveValue(); }

    std::vector<double>& dual_solution()
    {
        double* dual_sol = model_.dualRowSolution();
        for (int row = 0; row < m_ + n_; ++ row)
            dual_sol_vd_[row] = dual_sol[row];
        return dual_sol_vd_;
    }

    std::vector<double> solution()
    {
        const double* solution = model_.getColSolution();
        std::vector<double> sol(model_.numberColumns());
        for (int col = 0; col < model_.numberColumns(); ++col)
            sol[col] = solution[col];
        return sol;
    }

private:

    AgentIdx m_;
    ItemIdx n_;
    ClpSimplex model_;
    std::vector<double> ones_;
    std::vector<double> dual_sol_vd_;

};

#endif

#if CPLEX_FOUND

ILOSTLBEGIN

class ColGenSolverCplex: public ColGenSolver
{

public:

    ColGenSolverCplex(AgentIdx agent_constraint_number, ItemIdx item_constraint_number):
        m_(agent_constraint_number),
        n_(item_constraint_number),
        model_(env_),
        obj_(env_),
        range_(env_),
        cplex_(model_),
        dual_sol_(env_, m_ + n_),
        dual_sol_vd_(m_ + n_)
    {
        obj_.setSense(IloObjective::Minimize);
        model_.add(obj_);
        for (AgentIdx i = 0; i < m_; ++i)
            range_.add(IloRange(env_, -IloInfinity, 1));
        for (ItemIdx j = 0; j < n_; ++j)
            range_.add(IloRange(env_, 1, IloInfinity));
        model_.add(range_);
        cplex_.setOut(env_.getNullStream()); // Remove standard output
    }

    virtual ~ColGenSolverCplex()
    {
        env_.end();
    }

    void add_column(std::vector<int> rows, Cost c)
    {
        IloNumColumn col = obj_(c);
        for (int i: rows)
            col += range_[i](1);
        vars_.push_back(IloNumVar(col, 0, 1, ILOFLOAT));
        model_.add(vars_.back());
    }

    void solve() { cplex_.solve(); }

    double objective() { return cplex_.getObjValue(); }

    std::vector<double>& dual_solution()
    {
        cplex_.getDuals(dual_sol_, range_);
        for (int row = 0; row < m_ + n_; ++ row)
            dual_sol_vd_[row] = dual_sol_[row];
        return dual_sol_vd_;
    }

    std::vector<double> solution()
    {
        std::vector<double> sol(vars_.size());
        for (int col = 0; col < (int)vars_.size(); ++col)
            sol[col] = cplex_.getValue(vars_[col]);
        return sol;
    }

private:

    AgentIdx m_;
    ItemIdx n_;
    IloEnv env_;
    IloModel model_;
    IloObjective obj_;
    IloRangeArray range_;
    IloCplex cplex_;
    std::vector<IloNumVar> vars_;
    IloNumArray dual_sol_;
    std::vector<double> dual_sol_vd_;

};

#endif

ColGenOutput& ColGenOutput::algorithm_end(Info& info)
{
    PUT(info, "Algorithm", "Iterations", it);
    PUT(info, "Algorithm", "AddedColumns", added_column_number);
    Output::algorithm_end(info);
    VER(info, "Iterations: " << it << std::endl);
    VER(info, "Added columns: " << added_column_number << std::endl);
    return *this;
}

void add_column(const Instance& instance,
        ColGenOptionalParameters& parameters,
        ColGenSolver& solver,
        std::vector<std::vector<std::vector<ItemIdx>>>* columns,
        AgentIdx i,
        ColIdx col_idx,
        std::vector<std::pair<AgentIdx, ColIdx>>& col_indices,
        const std::vector<int>& agent_rows,
        const std::vector<int>& item_rows)
{
    std::vector<int> rows;
    rows.push_back(agent_rows[i]);
    Cost c = 0;
    Weight w = 0;
    Weight t = instance.capacity(i);

    std::vector<ItemIdx> column = (*columns)[i][col_idx];
    if (column.size() > 0) {
        for (ItemIdx j2 = 0; j2 < column[0]; ++j2) {
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j2][i] == 1)
                return;
        }
    }
    for (auto it = column.begin(); it != column.end(); ++it) {
        ItemIdx j = *it;
        // Check fixed variables
        if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 0)
            return;
        ItemIdx j_suiv = (std::next(it) == column.end())? instance.item_number(): *std::next(it);
        for (ItemIdx j2 = j + 1; j2 < j_suiv; ++j2) {
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j2][i] == 1)
                return;
        }
        if (item_rows[j] == -1) {
            t -= instance.weight(j, i);
            continue;
        }

        c += instance.cost(j, i);
        w += instance.weight(j, i);
        rows.push_back(item_rows[j]);
    }
    col_indices.push_back({i, col_idx});

    //if (i == 8 || i == 9) {
        //std::cout << "add_column i " << i << " c " << c << " items ";
        //for (ItemIdx j: (*columns)[i][col_idx])
            //std::cout << j << " ";
        //std::cout << std::endl;
    //}
    solver.add_column(rows, c);
}

ColGenOutput generalizedassignmentsolver::columngeneration(
        const Instance& instance, ColGenOptionalParameters parameters)
{
    VER(parameters.info, "*** columngeneration --lp-solver " << parameters.lp_solver << " ***" << std::endl);
    ColGenOutput output(instance, parameters.info);

    ItemIdx n = instance.item_number();
    AgentIdx m = instance.agent_number();

    // Handle fixed variables and fixed agents.
    std::vector<int> agent_row(m, -2);
    int row_idx = 0;
    for (AgentIdx i = 0; i < m; ++i) {
        if (parameters.fixed_agents != NULL && (*parameters.fixed_agents)[i] == 1)
            continue;
        agent_row[i] = row_idx;
        row_idx++;
    }
    AgentIdx agent_constraint_number = row_idx;

    Cost c0 = 0;
    std::vector<int> item_row(n, -2);
    for (ItemIdx j = 0; j < n; ++j) {
        for (AgentIdx i = 0; i < m; ++i) {
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 1) {
                c0 += instance.cost(j, i);
                item_row[j] = -1;
                break;
            }
        }
        if (item_row[j] == -2) {
            item_row[j] = row_idx;
            row_idx++;
        }
    }
    ItemIdx item_constraint_number = row_idx - agent_constraint_number;

    // Initialize solver
    std::unique_ptr<ColGenSolver> solver = NULL;
#if CPLEX_FOUND
    if (parameters.lp_solver == "cplex")
        solver = std::unique_ptr<ColGenSolver>(new ColGenSolverCplex(agent_constraint_number, item_constraint_number));
#endif
#if COINOR_FOUND
    if (parameters.lp_solver == "clp")
        solver = std::unique_ptr<ColGenSolver>(new ColGenSolverClp(agent_constraint_number, item_constraint_number));
#endif
    if (solver == NULL)
        return output.algorithm_end(parameters.info);

    // KP utils
    knapsacksolver::Instance instance_kp;
    // indices[j] == -2: item j is not in KP but belong to the column.
    //            == -1: item j is not in KP and does not belong to the column.
    //            >=  0: item j is the item of index indices[j] in KP.
    std::vector<knapsacksolver::ItemIdx> indices(n);
    std::vector<knapsacksolver::Weight> capacities_kp(m);
    for (AgentIdx i = 0; i < m; ++i) {
        capacities_kp[i] = instance.capacity(i);
        for (ItemIdx j = 0; j < n; ++j) {
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 1)
                capacities_kp[i] -= instance.weight(j, i);
        }
        if (capacities_kp[i] < 0)
            std::cout << "ERROR i " << i << " c " << capacities_kp[i] << std::endl;
    }

    // Add initial columns
    std::vector<int> rows(item_constraint_number);
    std::iota(rows.begin(), rows.end(), agent_constraint_number);
    solver->add_column(rows, 10 * instance.bound());
    output.column_indices.push_back({-1, -1});

    std::vector<std::vector<std::vector<ItemIdx>>>* columns;
    if (parameters.columns == NULL) {
        output.columns.resize(m);
        columns = &output.columns;
    } else {
        columns = parameters.columns;
        for (AgentIdx i = 0; i < m; ++i) {
            if (parameters.fixed_agents != NULL && (*parameters.fixed_agents)[i] == 1)
                continue;
            for (ColIdx col_idx = 0; col_idx < (ColIdx)(*columns)[i].size(); ++col_idx)
                add_column(instance, parameters, *solver, columns, i, col_idx, output.column_indices, agent_row, item_row);
        }
    }

    bool found = true;
    Weight mult = 100000;
    while (found) {
        output.it++;

        // Check time
        if (!parameters.info.check_time())
            break;

        // Solve LP
        solver->solve();
        VER(parameters.info,
                "It " << std::setw(8) << output.it
                << " | T " << std::setw(10) << parameters.info.elapsed_time()
                << " | C " << std::setw(10) << c0 + solver->objective()
                << " | COL " << std::setw(10) << output.added_column_number
                << std::endl);
        std::vector<double>& dual_sol = solver->dual_solution();

        // Find and add new columns
        found = false;
        for (AgentIdx i = 0; i < m; ++i) {
            if (agent_row[i] < 0)
                continue;

            // uᵢ: dual value associated with agent constraints (uᵢ ≤ 0).
            // vⱼ: dual value associated with item constraints.
            // xᵢⱼᵏ = 1 if yᵢᵏ contains item j
            //      = 0 otherwise
            // Reduced cost rcᵢᵏ = ∑ⱼ xᵢⱼᵏ cᵢⱼ - (uᵢ + ∑ⱼ xᵢⱼᵏ vⱼ)
            //                   = ∑ⱼ xᵢⱼᵏ (cᵢⱼ - vⱼ) - uᵢ
            // The algorithm that solves KP takes positive integers as input.
            // * positive => the profit in KP are vⱼ - cᵢⱼ and its optimum is
            //   > 0.
            // * integers => we round down the profits to get an upper bound of
            //   the minimum reduced cost.
            // We need an upper bound on the minimum reduced cost in order to
            // know when to stoparameters.
            // At the end, we will need a lower bound on the minimum reduced
            // cost in order to compute the bound.
            // Upper bound on the reduced cost rcubᵢᵏ = - opt(KPfloor) + ⌈-uᵢ⌉
            // Lower bound on the reduced cost rclbᵢᵏ = - opt(KPceil)  + ⌊-uᵢ⌋

            instance_kp.clear();
            instance_kp.set_capacity(capacities_kp[i]);
            Cost rc_ub = std::ceil((mult * (- dual_sol[agent_row[i]])));
            ItemIdx j_kp = 0;
            for (ItemIdx j = 0; j < n; ++j) {
                if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 1) {
                    indices[j] = -2;
                    continue;
                }
                if ((parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 0)) {
                    indices[j] = -1;
                    continue;
                }
                knapsacksolver::Profit profit = std::floor(mult * dual_sol[item_row[j]])
                    - std::ceil(mult * instance.cost(j, i));
                if (profit <= 0 || instance.weight(j, i) > capacities_kp[i]) {
                    indices[j] = -1;
                    continue;
                }
                instance_kp.add_item(instance.weight(j, i), profit);
                indices[j] = j_kp;
                j_kp++;
            }
            auto output_kp = knapsacksolver::minknap(instance_kp);
            rc_ub -= output_kp.solution.profit();
            //std::cout << "i " << i << " rc_ub " << rc_ub << " opt(kp) " << output_kp.solution.profit() << " vi " << (Cost)std::ceil((mult * (- dual_sol[agent_row[i]]))) << std::endl;
            if (rc_ub >= 0)
                continue;

            found = true;
            (*columns)[i].push_back({});
            for (ItemIdx j = 0; j < n; ++j)
                if (indices[j] == -2
                        || (indices[j] >= 0 && output_kp.solution.contains_idx(indices[j])))
                    (*columns)[i].back().push_back(j);
            add_column(instance, parameters, *solver, columns, i, (*columns)[i].size() - 1, output.column_indices, agent_row, item_row);
            output.added_column_number++;
        }
    }

    // Compute the bound
    // Solve LP
    solver->solve();
    std::vector<double>& dual_sol = solver->dual_solution();

    Cost lb = std::floor(mult * solver->objective());
    for (AgentIdx i = 0; i < m; ++i) {
        if (agent_row[i] < 0)
            continue;
        instance_kp.clear();
        instance_kp.set_capacity(capacities_kp[i]);
        Cost rc_lb = std::floor((mult * (- dual_sol[agent_row[i]])));
        for (ItemIdx j = 0; j < n; ++j) {
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 1)
                continue;
            if (parameters.fixed_alt != NULL && (*parameters.fixed_alt)[j][i] == 0)
                continue;
            knapsacksolver::Profit profit = std::ceil(mult * dual_sol[item_row[j]])
                - std::floor(mult * instance.cost(j, i));
            if (profit <= 0 || instance.weight(j, i) > capacities_kp[i])
                continue;
            instance_kp.add_item(instance.weight(j, i), profit);
        }
        auto output_kp = knapsacksolver::minknap(instance_kp);
        rc_lb -= output_kp.solution.profit();
        lb += rc_lb;
        //std::cout << rc_lb << std::endl;
    }

    lb = c0 + std::ceil((double)lb / mult);
    output.update_lower_bound(lb, std::stringstream(""), parameters.info);

    // Compute x
    output.solution = solver->solution();
    for (ItemIdx j = 0; j < n; ++j)
        output.x.push_back(std::vector<double>(instance.agent_number(), 0));
    for (ColIdx col_idx = 1; col_idx < (int)output.column_indices.size(); ++col_idx) {
        AgentIdx i       = output.column_indices[col_idx].first;
        ColIdx col_idx_2 = output.column_indices[col_idx].second;
        for (ItemIdx j: (*columns)[i][col_idx_2])
            output.x[j][i] += output.solution[col_idx];
    }

    return output.algorithm_end(parameters.info);
}

