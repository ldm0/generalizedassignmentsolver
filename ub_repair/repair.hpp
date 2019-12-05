#pragma once

#include "generalizedassignment/lib/solution.hpp"

#if COINOR_FOUND
#include "generalizedassignment/lb_linrelax_clp/linrelax_clp.hpp"
#endif

namespace generalizedassignment
{

Output sol_repairgreedy(const Instance& ins, Info info = Info());
Output sol_repaircombrelax(const Instance& ins, Info info = Info());
#if COINOR_FOUND
Output sol_repairlinrelax_clp(const Instance& ins, const LinRelaxClpOutput& linrelax_output, Info info = Info());
#endif

}

