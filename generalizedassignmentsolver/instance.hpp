#pragma once

#include "benchtools/info.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <memory>
#include <map>

#define TOL 0.0000001

namespace generalizedassignmentsolver
{

using benchtools::Info;

typedef int64_t Cost;
typedef double  PCost;
typedef int64_t Weight;
typedef int64_t ItemIdx;
typedef int64_t ItemPos;
typedef int64_t AgentIdx;
typedef int64_t AgentPos;
typedef int64_t AltIdx;
typedef int64_t AltPos;
typedef int64_t StateIdx;
typedef int64_t Counter;
typedef int64_t Seed;
typedef int64_t ColIdx;

class Solution;

struct Alternative
{
    AltIdx k;
    ItemIdx j;
    AgentIdx i;
    Weight w;
    Cost c;

    double efficiency() const { return c * w; }
};

struct Item
{
    ItemIdx j;
    std::vector<AltIdx> alt;
    Weight w; // total weight
    Cost c; // total cost

    Cost c_min = -1; // minimum cost
    Cost c_max = -1; // maximum cost
    AgentIdx i_cmin = -1; // min cost agent
    AgentIdx i_cmax = -1; // max cost agent

    Weight w_min = -1; // minimum weight
    Weight w_max = -1; // maximum weight
    AgentIdx i_wmin = -1; // min weight agent
    AgentIdx i_wmax = -1; // max weight agent
};

class Instance
{

public:

    /**
     * Constructors and destructor
     */

    /** Create instance from file. */
    Instance(std::string filename, std::string format = "generalizedassignment_beasley");

    /** Manual constructor. */
    Instance(AgentIdx m);
    void set_name(std::string name) { name_ = name; }
    void set_capacity(AgentIdx i, Weight t) { t_[i] = t; }
    void add_item();
    void set_alternative(ItemIdx j, AgentIdx i, Weight w, Cost p);
    void clear();

    /** Constructor for test instances. */
    void add_item(const std::vector<std::pair<Weight, Cost>>& a);
    void set_capacities(const std::vector<Weight>& t);
    void set_optimal_solution(Solution& sol);

    /** Copy constructor. */
    Instance(const Instance& ins);
    /** Copy assignment operator. */
    Instance& operator=(const Instance& ins);
    /** Destructor. */
    ~Instance();

    /**
     * Getters
     */

    std::string name() const { return name_; }
    const Item& item(ItemPos j) const { return items_[j]; }
    AltIdx alternative_index(ItemIdx j, AgentIdx i) const { return items_[j].alt[i]; } 
    const Alternative& alternative(AltPos k) const { return alternatives_[k]; }
    const Alternative& alternative(ItemIdx j, AgentIdx i) const { return alternatives_[items_[j].alt[i]]; } 

    inline Cost cost_max()   const { return c_max_; }
    inline Cost weight_max() const { return w_max_; }
    inline Cost profit(const Alternative& a)  const { return c_max_ - a.c; }
    inline Cost profit(AltIdx k)              const { return profit(alternative(k)); }
    inline Cost profit(ItemIdx j, AgentIdx i) const { return profit(alternative_index(i, j)); }

    ItemIdx item_number()       const { return items_.size(); }
    AgentIdx agent_number()     const { return t_.size(); }
    AltIdx alternative_number() const { return alternatives_.size(); }
    Weight capacity(AgentIdx i) const { return t_[i]; }

    const Solution* optimal_solution() const { return sol_opt_.get(); }
    Cost optimum() const;
    Cost bound() const { return c_tot_ + 1; }

    void plot(std::string filename);
    void write(std::string filename);

private:

    void read_beasley(std::ifstream& file);
    void read_standard(std::ifstream& file);

    std::string name_;
    std::vector<Item> items_;
    std::vector<Alternative> alternatives_;
    std::vector<Weight> t_;
    Cost c_max_ = -1;
    Cost c_tot_ = 0;
    Cost w_max_ = -1;

    std::unique_ptr<Solution> sol_opt_;

};

std::ostream& operator<<(std::ostream &os, const Alternative& alt);
std::ostream& operator<<(std::ostream &os, const Instance& instance);

}
