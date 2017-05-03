#include "simple_stubborn_sets.h"

#include "../globals.h"
#include "../utilities.h"
#include "../operator.h"
#include "../successor_generator.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

namespace POR {
std::vector<std::vector<Fact> > op_preconds;
std::vector<std::vector<Fact> > op_effects;

// Implementation of simple instantiation of strong stubborn sets.
// Disjunctive action landmarks are computed trivially.
//
//
// TODO: get_op_index belongs to a central place.
// We currently have copies of it in different parts of the code.
//
//
// Internal representation of operator preconditions.
// Only used during computation of interference relation.
// op_preconds[i] contains precondition facts of the i-th
// operator.
//
//
// Internal representation of operator effects.
// Only used during computation of interference relation.
// op_effects[i] contains effect facts of the i-th
// operator.

static inline int get_op_index(const Operator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

inline bool operator<(const Operator &lhs, const Operator &rhs) {
    return get_op_index(&lhs) < get_op_index(&rhs);
}

//static inline bool preconditions_have_mutex (int op1, int op2) {
//    const std::vector<Prevail> op1_prevail = g_operators[op1].get_prevail();
//    const std::vector<Prevail> op2_prevail = g_operators[op2].get_prevail();
//    const std::vector<PrePost> op1_pre_post = g_operators[op1].get_pre_post();
//    const std::vector<PrePost> op2_pre_post = g_operators[op2].get_pre_post();
//    //Checking the prevails of op1 against the prevails and the pre-posts of op2.
//    for(int i = 0; i < op1_prevail.size(); i++) {
//    	 int var = op1_prevail[i].var;
//    	 int value = op1_prevail[i].prev;
//    	 //prevails of op2.
//    	 for(int j = 0; j < op2_prevail.size(); j++) {
//    		 int other_var = op2_prevail[j].var;
//    		 int other_value = op2_prevail[j].prev;
//    		 if (are_mutex(make_pair(var, value), make_pair(other_var, other_value))) {
//                return true;
//    		 }
//    	 }
//    	 //pre-posts of op2.
//    	 for(int j = 0; j < op2_pre_post.size(); j++) {
//    		 int other_var = op2_pre_post[j].var;
//    		 int other_value = op2_pre_post[j].pre;
//    		 if (are_mutex(make_pair(var, value), make_pair(other_var, other_value))) {
//    		     return true;
//    		 }
//    	 }
//    }
//    //Checking the pre-posts of op1 against the prevails and the pre-posts of op2.
//    for(int i = 0; i < op1_pre_post.size(); i++) {
//        int var = op1_pre_post[i].var;
//    	int value = op1_pre_post[i].pre;
//    	//prevails of op2.
//    	 for(int j = 0; j < op2_prevail.size(); j++) {
//    	     int other_var = op2_prevail[j].var;
//    	     int other_value = op2_prevail[j].prev;
//    	     if (are_mutex(make_pair(var, value), make_pair(other_var, other_value))) {
//    	         return true;
//    	     }
//    	 }
//
//    	 //pre-posts of op2.
//    	 for(int j = 0; j < op2_pre_post.size(); j++) {
//    	     int other_var = op2_pre_post[j].var;
//    	     int other_value = op2_pre_post[j].pre;
//    	     std::cout << "var = " << var << std::endl;
//    	     std::cout << "other_var = " << other_var << std::endl;
//    	     std::cout << "value = " << value << std::endl;
//    	     std::cout << "other_value = " << other_value << std::endl;
//    	     if (are_mutex(make_pair(var, value), make_pair(other_var, other_value))) {
//    	         return true;
//    	     }
//    	 }
//    }
//	return false;
//}

static inline bool can_disable(int op1_no, int op2_no) {
    size_t i = 0;
    size_t j = 0;
    while (i < op_preconds[op2_no].size() && j < op_effects[op1_no].size()) {
        int read_var = op_preconds[op2_no][i].var;
        int write_var = op_effects[op1_no][j].var;
        if (read_var < write_var) {
            i++;
        } else if (read_var == write_var) {
            std::set<int> read_var_vals = op_preconds[op2_no][i].vals;
            std::set<int> write_var_vals = op_effects[op1_no][j].vals;
            if (read_var_vals.size() == 1 && write_var_vals.size() == 1) {
                int read_value = *(read_var_vals.begin());
                int write_value = *(write_var_vals.begin());
                if (read_value != write_value) {
                    return true;
                }
            } else {
                /*
                 * if either var has more than one possible value, that implies
                 * that op_1 can disable op_2
                 */
                return true;
            }
            i++;
            j++;
        } else {
            // read_var > write_var
            j++;
        }
    }
    return false;
}

static inline bool can_conflict(int op1_no, int op2_no) {
    size_t i = 0;
    size_t j = 0;
    while (i < op_effects[op1_no].size() && j < op_effects[op2_no].size()) {
        int var1 = op_effects[op1_no][i].var;
        int var2 = op_effects[op2_no][j].var;
        if (var1 < var2) {
            i++;
        } else if (var1 == var2) {
            std::set<int> var1_vals = op_effects[op1_no][i].vals;
            std::set<int> var2_vals = op_effects[op2_no][j].vals;
            if (var1_vals.size() == 1 && var2_vals.size() == 1) {
                int value1 = *(var1_vals.begin());
                int value2 = *(var2_vals.begin());
                if (value1 != value2)
                    return true;
            } else {
                /*
                 * if either var has more than one possible value, that implies
                 * that two operators can conflict.
                 */
                return true;
            }
            i++;
            j++;
        } else {
            // var1 > var2
            j++;
        }
    }
    return false;
}

static inline bool interfere(int op1_no, int op2_no) {
    return can_disable(op1_no, op2_no) || can_conflict(op1_no, op2_no) || can_disable(op2_no, op1_no);
}

// Return the first unsatified goal pair, or (-1, -1) if there is none.
static inline pair<int, int> find_unsatisfied_goal(const State &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        int goal_value = g_goal[i].second;
        if (state[goal_var] != goal_value)
            return make_pair(goal_var, goal_value);
    }
    return make_pair(-1, -1);
}

static inline void find_unsatisfied_effect_conditions(const Operator &op, 
						      const State &state, 
						      vector<pair<int, int> >& unsat_eff_conditions) {

    const vector<PrePost> &pre_post = op.get_pre_post();
    
    for (size_t i = 0; i < pre_post.size(); ++i) {
	for (int j = 0; j < pre_post[i].cond.size(); j++) {
	    const Prevail cond_p = pre_post[i].cond[j];	
	    int var = cond_p.var;
	    int value = cond_p.prev;
	    if (state[var] != value) {
		unsat_eff_conditions.push_back(make_pair(var, value));
		break;
	    }
	}
    }
}  

// Return the first unsatified precondition, or (-1, -1) if there is none.
static inline pair<int, int> find_unsatisfied_precondition(
        const Operator &op, const State &state) {
    const vector<Prevail> &prevail = op.get_prevail();
    for (size_t i = 0; i < prevail.size(); ++i) {
        int var = prevail[i].var;
        int value = prevail[i].prev;
        if (state[var] != value) {
            return make_pair(var, value);
        }
    }
    
    const vector<PrePost> &pre_post = op.get_pre_post();
    for (size_t i = 0; i < pre_post.size(); ++i) {
        int var = pre_post[i].var;
        int value = pre_post[i].pre;
        if (value != -1 && state[var] != value) {
            return make_pair(var, value);
        }
    }
    return make_pair(-1, -1);
}


SimpleStubbornSets::SimpleStubbornSets(bool otf_intereference, bool otf_achievers,
        int mem_bound_option, bool conservative_otf) {
    compute_sorted_operators();
    on_the_fly_interference = otf_intereference;
    on_the_fly_achievers = otf_achievers;
    conservative_otf_interference_and_achievers = conservative_otf;
    cond_effects_used = false;
    vector_entries_upper_bound = 1000000*mem_bound_option;
    number_of_vector_entries = 0;
    vector_entry_bound_exceeded = false;

    if(!has_cond_effects()) {
        /*
         * The following is needed for the computation of active operators
         * which we ignore now for the case of conditional effects.
         */
        size_t num_variables = g_variable_domain.size();
        reachability_map.resize(num_variables);
        build_dtgs();
        build_reachability_map();
    } else {
        std::cout << "Conditional effects are supported ... " << std::endl;
        cond_effects_used = true;
    }

    if(!on_the_fly_interference) {
        compute_interference_relation();
    } else {
        size_t num_operators = g_operators.size();
        interference_relation.resize(num_operators);
        interference_is_computed.assign(g_operators.size(), false);
    }

    if(!on_the_fly_achievers) {
        compute_achievers();
    } else {
        size_t num_variables = g_variable_domain.size();
        achievers.resize(num_variables);
        achievers_are_computed.resize(num_variables);

        for (int var_no = 0; var_no < num_variables; ++var_no){
            achievers[var_no].resize(g_variable_domain[var_no]);
            achievers_are_computed[var_no].assign(g_variable_domain[var_no], false);
        }
    }
}

SimpleStubbornSets::~SimpleStubbornSets() {
}

void SimpleStubbornSets::dump_options() const {
    cout << "partial order reduction method: simple stubborn sets" << endl;
    std::string interference_computation_mode;
    std::string achievers_computation_mode;
    if(on_the_fly_interference) {
        interference_computation_mode = "on";
    } else {
        interference_computation_mode = "off";
    }

    if(on_the_fly_achievers) {
        achievers_computation_mode = "on";
    } else {
        achievers_computation_mode = "off";
    }
    cout << "interference relation on-the-fly mode: " << interference_computation_mode << std::endl;
    cout << "achievers relation on-the-fly mode: " << achievers_computation_mode << std::endl;
}

void SimpleStubbornSets::build_dtgs() {
    /*
  NOTE: Code lifted and adapted from M&S atomic abstraction code.

  We need a more general mechanism for creating data structures of
  this kind.
     */

    /*
  NOTE: for expansion core, the DTG for v *does* include
  self-loops from d to d if there is an operator that sets the
  value of v to d and has no precondition on v. This is different
  from the usual DTG definition.
     */

    cout << "[partial-order-reduction] Building DTGs..." << flush;
    assert(dtgs.empty());
    size_t num_variables = g_variable_domain.size();

    dtgs.resize(num_variables);

    // Step 1: Create the empty DTG nodes.
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        size_t var_size = g_variable_domain[var_no];
        dtgs[var_no].nodes.resize(var_size);
        dtgs[var_no].goal_values.resize(var_size, true);
    }

    // Step 2: Mark goal values in each DTG. Variables that are not in
    //         the goal have previously been set to "all are goals".
    for (int i = 0; i < g_goal.size(); ++i) {
        int var_no = g_goal[i].first;
        int goal_value = g_goal[i].second;
        vector<bool> &goal_values = dtgs[var_no].goal_values;
        size_t var_size = g_variable_domain[var_no];
        goal_values.clear();
        goal_values.resize(var_size, false);
        goal_values[goal_value] = true;
    }

    // Step 3: Add DTG arcs.
    for (int op_no = 0; op_no < g_operators.size(); ++op_no) {
        const Operator &op = g_operators[op_no];
        const vector<PrePost> &pre_post = op.get_pre_post();
        for (int i = 0; i < pre_post.size(); ++i) {
            int var = pre_post[i].var;
            int pre_value = pre_post[i].pre;
            int post_value = pre_post[i].post;

            ExpansionCoreDTG &dtg = dtgs[var];
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = g_variable_domain[var];
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }
            for (int value = pre_value_min; value < pre_value_max; ++value) {
                dtg.nodes[value].outgoing.push_back(
                        ExpansionCoreDTG::Arc(post_value, op_no));
                dtg.nodes[post_value].incoming.push_back(
                        ExpansionCoreDTG::Arc(value, op_no));
            }
        }
    }
}

void SimpleStubbornSets::recurse_forwards(int var, int start_value,
        int current_value, std::vector<bool> &reachable) {
    ExpansionCoreDTG &dtg = dtgs[var];
    if (!reachable[current_value]) {
        reachable[current_value] = true;
        const vector<ExpansionCoreDTG::Arc> &outgoing = dtg.nodes[current_value].outgoing;
        for (int i = 0; i < outgoing.size(); ++i)
            recurse_forwards(var, start_value, outgoing[i].target_value, reachable);
    }
}

void SimpleStubbornSets::build_reachability_map() {
    size_t num_variables = g_variable_domain.size();
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        ExpansionCoreDTG &dtg = dtgs[var_no];
        size_t num_values = dtg.nodes.size();
        reachability_map[var_no].resize(num_values);
        for (int val = 0; val < num_values; ++val) {
            reachability_map[var_no][val].assign(num_values, false);
        }
        for (int start_value = 0; start_value < g_variable_domain[var_no]; start_value++) {
            vector<bool> &reachable = reachability_map[var_no][start_value];
            recurse_forwards(var_no, start_value, start_value, reachable);
        }
    }
}

void SimpleStubbornSets::compute_active_operators(const State &state,
        std::vector<bool>& active_ops) {
    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        Operator &op = g_operators[op_no];
        bool all_preconditions_are_active = true;
        const vector<Prevail> &prevail = op.get_prevail();
        for (size_t i = 0; i < prevail.size(); ++i) {
            int var = prevail[i].var;
            int value = prevail[i].prev;
            int current_value = state[var];
            std::vector<bool> &reachable_values = reachability_map[var][current_value];
            if (!reachable_values[value]) {
                all_preconditions_are_active = false;
                break;
            }
        }
        if (all_preconditions_are_active) {
            const vector<PrePost> &pre_post = op.get_pre_post();
            for (size_t i = 0; i < pre_post.size(); ++i) {
                int var = pre_post[i].var;
                int value = pre_post[i].pre;
                if (value != -1) {
                    int current_value = state[var];
                    std::vector<bool> &reachable_values = reachability_map[var][current_value];
                    if (!reachable_values[value]) {
                        all_preconditions_are_active = false;
                        break;
                    }
                }
            }
        }
        if (all_preconditions_are_active) {
            active_ops[op_no] = true;
        }
    }
}

/*
 * TODO: this method contains redundant code --> refactoring.
 */
void SimpleStubbornSets::compute_sorted_operators() {
    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        Operator *op = &g_operators[op_no];

        const vector<PrePost> &pre_post = op->get_pre_post();
        const vector<Prevail> &prevail = op->get_prevail();

        //conditional effects
        std::vector<int> visited_preconds;
        std::vector<int> visited_effects;

        vector<Fact> pre;
        vector<Fact> eff;

        for (size_t i = 0; i < pre_post.size(); i++) {
            const PrePost *pp = &pre_post[i];
            //precondition
            if (pp->pre != -1) {
                bool visited_var = false;
                for (int k = 0; k < visited_preconds.size(); k++) {
                    if (pp->var == visited_preconds[k]) {
                        //pre[k] contains pp->var
                        pre[k].vals.insert(pp->pre);
                        visited_var = true;
                        break;
                    }
                }
                if(!visited_var) {
                    visited_preconds.push_back(pp->var);
                    std::set<int> vals;
                    vals.insert(pp->pre);
                    Fact p(pp->var, vals);
                    pre.push_back(p);
                }
            }
            //effect conditions
            for (int j = 0; j < pp->cond.size(); j++) {
                const Prevail cond_p = pp->cond[j];
                //bool visited_var = false;
                bool visited_var = false;
                for (int k = 0; k < visited_preconds.size(); k++) {
                    if (cond_p.var == visited_preconds[k]) {
                        //pre[k] contains pp->var
                        pre[k].vals.insert(cond_p.prev);
                        visited_var = true;
                        break;
                    }
                }
                if(!visited_var) {
                    visited_preconds.push_back(cond_p.var);
                    std::set<int> vals;
                    vals.insert(cond_p.prev);
                    Fact cond(cond_p.var, vals);
                    pre.push_back(cond);
                }
            }

            //post condition (effect)
            bool visited_var = false;
            for (int k = 0; k < visited_effects.size(); k++) {
                if (pp->var == visited_effects[k]) {
                    //eff[k] contains pp->var
                    eff[k].vals.insert(pp->post);
                    visited_var = true;
                    break;
                }
            }
            if(!visited_var) {
                visited_effects.push_back(pp->var);
                std::set<int> vals;
                vals.insert(pp->post);
                Fact post(pp->var, vals);
                eff.push_back(post);
            }
        }

        //prevail preconditions
        for (size_t i = 0; i < prevail.size(); i++) {
            const Prevail *pp = &prevail[i];
            bool visited_var = false;
            for (int k = 0; k < visited_preconds.size(); k++) {
                if (pp->var == visited_preconds[k]) {
                    //pre[k] contains pp->var
                    pre[k].vals.insert(pp->prev);
                    visited_var = true;
                    break;
                }
            }
            if(!visited_var) {
                visited_preconds.push_back(pp->var);
                std::set<int> vals;
                vals.insert(pp->prev);
                Fact p(pp->var, vals);
                pre.push_back(p);
            }
        }

        //sort the variables
        if (pre.size() != 0) {
            sort(pre.begin(), pre.end());
            for (size_t i = 0; i < pre.size() - 1; ++i) {
                assert(pre[i].var < pre[i + 1].var);
            }
        }

        //sort the variables
        sort(eff.begin(), eff.end());
        for (size_t i = 0; i < eff.size() - 1; ++i) {
            assert(eff[i].var < eff[i + 1].var);
        }

        op_preconds.push_back(pre);
        op_effects.push_back(eff);
    }
}

void SimpleStubbornSets::compute_interference_relation() {
    size_t num_operators = g_operators.size();
    interference_relation.resize(num_operators);
    for (size_t op1_no = 0; op1_no < num_operators; ++op1_no) {
        vector<int> &interfere_op1 = interference_relation[op1_no];
        for (size_t op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no && interfere(op1_no, op2_no)) {
                interfere_op1.push_back(op2_no);
            }
        }
    }
}

void SimpleStubbornSets::compute_achievers() {
    size_t num_variables = g_variable_domain.size();
    achievers.resize(num_variables);
    for (int var_no = 0; var_no < num_variables; ++var_no)
        achievers[var_no].resize(g_variable_domain[var_no]);
    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        const Operator &op = g_operators[op_no];
        const vector<PrePost> &pre_post = op.get_pre_post();
        for (size_t i = 0; i < pre_post.size(); ++i) {
            int var = pre_post[i].var;
            int value = pre_post[i].post;
            achievers[var][value].push_back(op_no);
        }
    }
}

inline void SimpleStubbornSets::mark_as_stubborn(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
    }
}

// Add all operators that achieve the fact (var, value) to stubborn set.
void SimpleStubbornSets::add_nes_for_fact(pair<int, int> fact) {
    int var = fact.first;
    int value = fact.second;
    bool all_ops_marked_as_stubborn = false;
    const vector<int> &op_nos = achievers[var][value];
    if(on_the_fly_achievers && !achievers_are_computed[var][value]){
        if(!vector_entry_bound_exceeded){
            achievers_are_computed[var][value] = true;
            for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
                const Operator &op = g_operators[op_no];
                const vector<PrePost> &pre_post = op.get_pre_post();
                for (size_t i = 0; i < pre_post.size(); ++i) {
                    if ((var == pre_post[i].var) && ( value == pre_post[i].post)){
                        achievers[var][value].push_back(op_no);
                        number_of_vector_entries++;
                    }
                }
            }
            vector_entry_bound_exceeded = (number_of_vector_entries > vector_entries_upper_bound);
        } else {
            all_ops_marked_as_stubborn = true;
            //TODO: find which of the following is more efficient in practice.
            /* 1st possibility: if the mem-bound is exceeded and the achieving operators
            haven't been computed yet, then compute the achieving ops, add them to the
            stubborn set but don't store them */
            if(!conservative_otf_interference_and_achievers) {
                for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
                    const Operator &op = g_operators[op_no];
                    const vector<PrePost> &pre_post = op.get_pre_post();
                    for (size_t i = 0; i < pre_post.size(); ++i) {
                        if ((var == pre_post[i].var) && ( value == pre_post[i].post)) {
                            if(cond_effects_used || active_ops[op_no]) {
                                mark_as_stubborn(op_no);
                            }
                        }
                    }
                }
            } else { // Add all operators to stubborn.
                /* 2nd possibility (already checked): if the mem-bound is exceeded and the
                 * achieving operators haven't been computed yet, then assume that all
                 * operators achieve this fact.
                 */
                //Note: this can lead to more expansions.
                for (size_t i = 0; i < g_operators.size(); ++i) {
                    int op_no = get_op_index(&g_operators[i]);
                    if(cond_effects_used || active_ops[op_no]) {
                        mark_as_stubborn(op_no);
                    }
                }
            }
        }
    }
    if(!all_ops_marked_as_stubborn){
        for (size_t i = 0; i < op_nos.size(); ++i) {
            if(cond_effects_used || active_ops[op_nos[i]]) {
                //no conditional effects --> consider active operators
                mark_as_stubborn(op_nos[i]);
            }
        }
    }
}

// Add all operators that interfere with op.
void SimpleStubbornSets::add_interfering(int op_no) {
    size_t num_operators = g_operators.size();
    bool all_ops_marked_as_stubborn = false;
    vector<int> &interfere_op = interference_relation[op_no];
    if(on_the_fly_interference && !interference_is_computed[op_no]){
        if(!vector_entry_bound_exceeded){
            interference_is_computed[op_no] = true;
            for (size_t other = 0; other < num_operators; ++other) {
                if (op_no != other && interfere(op_no, other)) {
                    interfere_op.push_back(other);
                    number_of_vector_entries++;
                }
            }
            vector_entry_bound_exceeded = (number_of_vector_entries > vector_entries_upper_bound);
        } else {
            //TODO: find which of the following is more efficient in practice.
            all_ops_marked_as_stubborn = true;
            /* 1st possibility: if the mem-bound is exceeded and the achieving operators
             *haven't been computed yet, then compute the interfering ops, add them to the
             *stubborn set but don't store them
             */
            if(!conservative_otf_interference_and_achievers) {
                for (size_t other = 0; other < num_operators; ++other) {
                    if (op_no != other && interfere(op_no, other)) {
                        if(cond_effects_used || active_ops[other]) {
                            mark_as_stubborn(other);
                        }
                    }
                }
            } else { // Add all operators to stubborn.
                /* 2nd possibility (already checked): if the mem-bound is exceeded and the
                 * achieving operators haven't been computed yet, then assume that all
                 * operators interfere with this operator.
                 */
                //Note: this can lead to more expansions.
                for (size_t i = 0; i < g_operators.size(); ++i) {
                    int op_no = get_op_index(&g_operators[i]);
                    if(cond_effects_used || active_ops[op_no]) {
                        mark_as_stubborn(op_no);
                    }
                }
            }
        }
    }
    if(!all_ops_marked_as_stubborn){
        for (size_t i = 0; i < interfere_op.size(); ++i) {
            if(cond_effects_used || active_ops[interfere_op[i]]) {
                mark_as_stubborn(interfere_op[i]);
            }
        }
    }
}

void SimpleStubbornSets::do_pruning(
        const State &state, vector<const Operator *> &applicable_ops) {
    // Clear stubborn set from previous call.
    stubborn.clear();
    stubborn.assign(g_operators.size(), false);
    if(!cond_effects_used) {
        active_ops.clear();
        active_ops.assign(g_operators.size(), false);
        compute_active_operators(state, active_ops);
    }
    // Add a necessary enabling set for an unsatisfied goal.
    pair<int, int> goal_pair = find_unsatisfied_goal(state);
    assert(goal_pair.first != -1);
    add_nes_for_fact(goal_pair);

    // Iteratively insert operators to stubborn according to the
    // definition of strong stubborn sets until a fixpoint is reached.
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        const Operator &op = g_operators[op_no];
        pair<int, int> fact = find_unsatisfied_precondition(op, state);
        if (fact.first == -1) {
	    add_interfering(op_no);

	    if (cond_effects_used) {
		unsat_eff_conditions.clear();
		find_unsatisfied_effect_conditions(op, state, unsat_eff_conditions);
		for (int k = 0; k < unsat_eff_conditions.size(); k++)
		    add_nes_for_fact(unsat_eff_conditions[k]);
	    }

        } else {
            //	if (fact.first != -1){
            // unsatisfied precondition found
            // => add an enabling set for it
            add_nes_for_fact(fact);
        }
    }

    // Now check which applicable operators are in the stubborn set.
    vector<const Operator *> unpruned_ops;
    unpruned_ops.reserve(applicable_ops.size());
    for (size_t i = 0; i < applicable_ops.size(); ++i) {
        const Operator *op = applicable_ops[i];
        int op_no = get_op_index(op);
        if (stubborn[op_no]) {
            unpruned_ops.push_back(op);
        }
    }

    if (unpruned_ops.size() != applicable_ops.size()) {
        applicable_ops.swap(unpruned_ops);
        sort(applicable_ops.begin(), applicable_ops.end());
    }
}
}
