#include "gap/ub_vdns_simple/vdns_simple.hpp"
#include "gap/ub_random/random.hpp"
#include "gap/ub_repair/repair.hpp"
#include "gap/opt_branchandcut_cbc/branchandcut_cbc.hpp"

#include <set>
#include <random>
#include <algorithm>
#include <vector>

using namespace gap;

bool move_gap(const Instance& ins, Solution& sol,
        AgentIdx m, const std::vector<AgentIdx>& agents,
        ItemIdx n, const std::vector<ItemIdx>& items,
        Info& info)
{
    std::vector<Weight> c(m, 0);
    for (AgentIdx i=0; i<m; ++i)
        c[i] = ins.capacity(agents[i]);
    Instance ins_tmp(m, n);
    std::vector<AgentIdx> sol_vec;
    Cost v = 0;
    std::vector<ItemIdx> pos;
    for (ItemIdx j: items) {
        AgentIdx i = -1;
        for (AgentPos i_pos=0; i_pos<m; ++i_pos) {
            if (agents[i_pos] == sol.agent(j)) {
                i = i_pos;
                break;
            }
        }
        if (i == -1)
            continue;
        if (ins_tmp.item_number() == n) {
            c[i] -= ins.alternative(j, sol.agent(j)).w;
            continue;
        }
        v += ins.alternative(j, sol.agent(j)).c;
        ItemIdx j_tmp = ins_tmp.add_item();
        sol_vec.push_back(i);
        for (AgentPos i_pos=0; i_pos<m; ++i_pos)
            ins_tmp.set_alternative(j_tmp, i_pos,
                    ins.alternative(j, agents[i_pos]).w,
                    ins.alternative(j, agents[i_pos]).c);
        pos.push_back(j);
    }
    for (AgentPos i_pos=0; i_pos<m; ++i_pos)
        ins_tmp.set_capacity(i_pos, c[i_pos]);
    Solution sol_tmp(ins_tmp);
    for (ItemIdx j=0; j<(ItemIdx)pos.size(); ++j)
        sol_tmp.set(j, sol_vec[j]);
    sopt_branchandcut_cbc({
            .ins = ins_tmp,
            .sol = sol_tmp,
            .stop_at_first_improvment = false,
            .info = Info().set_timelimit(info.timelimit - info.elapsed_time()),
            });
    if (v <= sol_tmp.cost())
        return false;

    //for (ItemIdx j=0; j<(ItemIdx)pos.size(); ++j)
        //if (sol.agent(pos[j]) != agents[sol_tmp.agent(j)])
            //std::cout << "item " << items[j] << " prev " << sol.agent(pos[j]) << " new " << agents[sol_tmp.agent(j)] << std::endl;

    for (ItemIdx j=0; j<(ItemIdx)pos.size(); ++j)
        sol.set(pos[j], agents[sol_tmp.agent(j)]);
    return true;
}

bool move_mbp(const Instance& ins, Solution& sol,
        ItemIdx n, const std::vector<ItemIdx>& items,
        const std::vector<AgentIdx>& t)
{
    int numberRows = ins.agent_number();
    int numberColumns = n;
    int numberElements = 2 * numberColumns;

    // Matrix data - column ordered
    std::vector<CoinBigIndex> start(numberColumns + 1);
    for (ItemIdx j=0; j<=numberColumns; ++j)
        start[j] = 2 * j;
    std::vector<int> length(numberColumns, 2);

    std::vector<int> rows(numberElements);
    std::vector<double> elements(numberElements);
    for (ItemIdx j=0; j<numberColumns; ++j) {
        AgentIdx sj = sol.agent(items[j]);
        AgentIdx tj = t[j];
        if (sj < tj) {
            rows[2 * j]     = sj;
            rows[2 * j + 1] = tj;
            elements[2 * j]     =   ins.alternative(items[j], sj).w;
            elements[2 * j + 1] = - ins.alternative(items[j], tj).w;
        } else {
            rows[2 * j]     = tj;
            rows[2 * j + 1] = sj;
            elements[2 * j]     = - ins.alternative(items[j], tj).w;
            elements[2 * j + 1] =   ins.alternative(items[j], sj).w;
        }
    }
    CoinPackedMatrix matrix(true, numberRows, numberColumns, numberElements,
            elements.data(), rows.data(), start.data(), length.data());

    // Rim data
    std::vector<double> objective(numberColumns);
    Cost csum = 0;
    for (ItemIdx j=0; j<numberColumns; ++j) {
        AgentIdx sj = sol.agent(items[j]);
        AgentIdx tj = t[j];
        csum += ins.alternative(items[j], tj).c;
        objective[j] = ins.alternative(items[j], sj).c - ins.alternative(items[j], tj).c;
    }
    for (ItemIdx j=numberColumns; j<ins.item_number(); ++j)
        csum += ins.alternative(items[j], sol.agent(items[j])).c;

    std::vector<double> rowUpper(numberRows);
    for (AgentIdx i=0; i<numberRows; ++i)
        rowUpper[i] = ins.capacity(i);
    for (ItemIdx j=0; j<numberColumns; ++j)
        rowUpper[t[j]] -= ins.alternative(items[j], t[j]).w;
    for (ItemIdx j=numberColumns; j<ins.item_number(); ++j)
        rowUpper[sol.agent(items[j])] -= ins.alternative(items[j], sol.agent(items[j])).w;
    std::vector<double> colLower(numberColumns, 0);
    std::vector<double> colUpper(numberColumns, 1);
    OsiCbcSolverInterface solver1;

    // Reduce printout
    solver1.getModelPtr()->setLogLevel(0);
    solver1.messageHandler()->setLogLevel(0);

    // Load problem
    solver1.loadProblem(matrix, colLower.data(), colUpper.data(),
              objective.data(), 0, rowUpper.data());

    // Mark integer
    for (ItemIdx j=0; j<numberColumns; ++j)
        solver1.setInteger(j);

    // Solve
    solver1.initialSolve();

    // Pass data and solver to CbcModel
    CbcModel model(solver1);
    model.setMaximumSeconds(16);

    // Add initial solution
    Weight v = 0;
    for (ItemIdx j=0; j<n; ++j) {
        AgentIdx sj = sol.agent(items[j]);
        AgentIdx tj = t[j];
        v += ins.alternative(items[j], sj).c - ins.alternative(items[j], tj).c;
    }
    std::vector<double> sol_init(numberColumns, 1);
    model.setBestSolution(sol_init.data(), numberColumns, v);

    // Reduce printout
    model.setLogLevel(0);
    model.solver()->setHintParam(OsiDoReducePrint, true, OsiHintTry);

    // Do complete search
    model.branchAndBound();

    // Get solution
    const double *solution = model.solver()->getColSolution();
    for (ItemIdx j=0; j<numberColumns; ++j)
        if (solution[j] < 0.5)
            sol.set(items[j], t[j]);
    //std::cout << "csum " << csum << " v " << v << " csum+v " << csum + v << std::endl;
    //std::cout << "obj val " << model.solver()->getObjValue()
        //<< " new val " << csum + model.solver()->getObjValue() << std::endl;
    //std::cout << "end sol.value() " << sol.cost() << " n " << sol.item_number() << " f " << sol.feasible() << std::endl;

    return (csum + v > sol.cost());
}

Solution gap::sol_vdns_simple(const Instance& ins, std::mt19937_64& gen, Info info)
{
    ItemIdx n = ins.item_number();
    AgentIdx m = ins.agent_number();

    init_display(info);
    Solution sol_best(ins);

    LinRelaxClpOutput linrelax_output = lb_linrelax_clp(ins);
    Cost lb = linrelax_output.lb;
    Solution sol_curr = sol_repairlinrelax(ins, linrelax_output);
    //Solution sol_curr = sol_random(ins, gen);

    std::stringstream ss;
    sol_best.update(sol_curr, lb, ss, info);

    std::vector<ItemIdx> items(n, 0);
    std::iota(items.begin(), items.end(), 0);
    std::uniform_int_distribution<AgentIdx> dis_i(0, m-2);

    /*
    for (;;) {
        ItemIdx n_mbp = std::min(n, (ItemIdx)256);
        std::shuffle(items.begin(), items.end(), gen);
        std::vector<AgentIdx> t(n_mbp);
        for (ItemIdx j=0; j<n_mbp; ++j) {
            AgentIdx sj = sol_curr.agent(items[j]);
            AgentIdx i = dis_i(gen);
            if (i > sj)
                ++i;
            t[j] = i;
        }
        if (move_mbp(ins, sol_curr, n_mbp, items, t)) {
            std::stringstream ss;
            ss << "mbp n " << n_mbp;
            sol_best.update(sol_curr, lb, ss, info);
        }
    }
     */

    for (AgentIdx m_gap=2; m_gap<=m && info.check_time(); ++m_gap) {
        std::vector<std::vector<AgentIdx>> agents;
        std::vector<AgentIdx> vec(m_gap);
        std::iota(vec.begin(), vec.end(), 0);
        while (vec[0] <= m - m_gap) {
            while (vec.back() < m) {
                agents.push_back(vec);
                vec.back()++;
            }
            AgentIdx i = m_gap - 1;
            while (i >= 0 && vec[i] >= m - m_gap + i)
                i--;
            if (i == -1)
                break;
            vec[i]++;
            for (AgentIdx i2=i+1; i2<m_gap; ++i2)
                vec[i2] = vec[i2 - 1] + 1;
        }

        std::shuffle(agents.begin(), agents.end(), gen);
        ItemIdx k_last = agents.size() - 1;
        for (ItemIdx k=0; ; k=(k+1)%agents.size()) {
            if (move_gap(ins, sol_curr, m_gap, agents[k], n, items, info)) {
                std::stringstream ss;
                ss << "gap m " << m_gap << ":";
                for (AgentIdx i: agents[k])
                    ss << " " << i;
                sol_best.update(sol_curr, lb, ss, info);
                k_last = (k != 0)? k - 1: agents.size() - 1;
                std::shuffle(items.begin(), items.end(), gen);

            }
            if (k == k_last)
                break;
            if (!info.check_time())
                return algorithm_end(sol_best, info);
        }
    }
    return algorithm_end(sol_best, info);
}

