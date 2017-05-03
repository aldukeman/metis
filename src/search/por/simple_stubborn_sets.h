#ifndef POR_SIMPLE_STUBBORN_SETS_H
#define POR_SIMPLE_STUBBORN_SETS_H

#include "por_method.h"
#include "../globals.h"
#include "../operator.h"
#include "../successor_generator.h"
#include "expansion_core.h"
#include <vector>
#include <set>
#include <cassert>
#include <algorithm>
using namespace std;

class Operator;
class State;
namespace POR {

struct Fact {
    int var;
    std::set<int> vals;
    Fact(int v, set<int> d) : var(v), vals(d) {}
};


inline bool operator<(const Fact &lhs, const Fact &rhs) {
    return lhs.var < rhs.var;
}
////////////////////////////////////////////

class SimpleStubbornSets : public PORMethodWithStatistics {
    // achievers[var][value] contains all operator indices of
    // operators that achieve the fact (var, value).
	bool on_the_fly_interference;
	bool on_the_fly_achievers;
	bool conservative_otf_interference_and_achievers;
	int vector_entries_upper_bound;
	int number_of_vector_entries;
	bool vector_entry_bound_exceeded;
    std::vector<std::vector<std::vector<int> > > achievers;

    //achievers_are_computed indicates if the the achievers
    //have been computed for the operator
    std::vector<std::vector<bool> > achievers_are_computed;

    // interference_relation[op1_no] contains all operator indices
    // of operators that interfere with op1.
    std::vector<std::vector<int> > interference_relation;

    //interference_is_computed indicates if the the interference relation
    //has been computed for the operator
    std::vector<bool> interference_is_computed;

    // stubborn[op_no] is true iff the operator with operator index
    // op_no is contained in the stubborn set
    std::vector<bool> stubborn;

    // stubborn_queue contains the operator indices of operators that
    // have been marked and stubborn but have not yet been processed
    // (i.e. more operators might need to be added to stubborn because
    // of the operators in the queue).
    std::vector<int> stubborn_queue;
    bool cond_effects_used;
    std::vector<ExpansionCoreDTG> dtgs;
    std::vector<bool> active_ops;
    std::vector<std::vector<std::vector<bool> > > reachability_map;
    
    // helper structure
    std::vector<std::pair<int, int> > unsat_eff_conditions;

    void build_dtgs();
    void recurse_forwards(int var, int start_value,
    			int current_value, std::vector<bool> &reachable);
    void build_reachability_map();
    void compute_active_operators(const State &state,
    			std::vector<bool>& active_ops);
    void mark_as_stubborn(int op_no);
    void add_nes_for_fact(std::pair<int, int> fact);
    void add_interfering(int op_no);

    void compute_interference_relation();
    void compute_achievers();
    void compute_sorted_operators();
    void dump_interference_relation();
    void dump_achievers();
protected:
    virtual void do_pruning(const State &state,
                            std::vector<const Operator *> &ops);
public:
    SimpleStubbornSets(bool otf_interference, bool otf_achievers, int mem_bound, bool conservative_otf);
    ~SimpleStubbornSets();

    virtual void dump_options() const;
};
}



#endif
