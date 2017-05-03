#include "por_search.h"
#include "plugin.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"
#include "successor_generator.h"

// Preliminary implementation of simple instantiation of strong
// stubborn sets. Disjunctive action landmarks are computed trivially.
// Currently only works for STRIPS.

PorSearch::PorSearch(const Options &opts) 
  : EagerSearch(opts)
{
    // TODO: Optimze! Only one direction has to be checked because our
    // approximation of interference is symmetrical
    init_interference();
}


void PorSearch::init_interference() {
    
    for(int i = 0; i < g_operators.size(); i++) {
	const Operator* op1 = &(g_operators[i]);
	vector<const Operator*> temp;
	for (int j = 0; j < g_operators.size(); j++) {
	    const Operator* op2 = &(g_operators[j]);
 	    if (interfere(op1, op2) && op1 != op2) {
		temp.push_back(op2);
	    }
	}
	
	interference_relation[op1] = temp;
    }
}

// returns true iff insertion succeeded
static inline bool insert_without_dup(const Operator* op, vector<const Operator*>& temp) {
  
  for (int i = 0; i < temp.size(); i++) {
      if (op == temp[i]) {
	  
	  assert(op->get_name() == temp[i]->get_name());
	  
	  return false;
      }
  }
  
  temp.push_back(op);
  return true;
  
}

// returns true iff op1 writes a variable in op2's precondition
static inline bool writesPrecondition(const Operator* op1, const Operator* op2) {

  for (int i = 0; i < op2->get_prevail().size(); i++) {
    int read_var = op2->get_prevail()[i].var;
    int read_val = op2->get_prevail()[i].prev;
    for (int j = 0; j < op1->get_pre_post().size(); j++) {
	if (op1->get_pre_post()[j].var == read_var){
	    if (op1->get_pre_post()[j].post != read_val) {
		return true;
	    }
	}
    }
  }

  for (int i = 0; i < op2->get_pre_post().size(); i++) {
    int read_var = op2->get_pre_post()[i].var;
    int read_val = op2->get_pre_post()[i].pre;
    if (op2->get_pre_post()[i].pre != -1) {
      for (int j = 0; j < op1->get_pre_post().size(); j++) {
	  if (op1->get_pre_post()[j].var == read_var){
	      if (op1->get_pre_post()[j].post != read_val) {
		  return true;
	      }
	  }
      }
    }
  }
  
  return false;
  
}
  

bool PorSearch::interfere(const Operator* op1, const Operator* op2) const {
  
  if (writesPrecondition(op1,op2) || writesPrecondition(op2,op1)) {
    return true;
  }
  else {
    // check if op1 and op2 write the same variable with different values
    for (int i = 0; i < op1->get_pre_post().size(); i++) {
      for (int j = 0; j < op2->get_pre_post().size(); j++) {
	
	if (op1->get_pre_post()[i].var == op2->get_pre_post()[j].var &&
	    op1->get_pre_post()[i].post != op2->get_pre_post()[j].post) {
	  
	  return true;
	  
	}
      }
    }
  }
  
  return false;
  
  
}

// adds operators to temp that write a variable that is read by op
void PorSearch::addNES(const Operator* op, const State& state, vector<const Operator*>& temp) {
  
    int read_var = -1;
    int read_value = -1;
    for (int i = 0; i < op->get_prevail().size(); i++) {
      if (state[op->get_prevail()[i].var] != op->get_prevail()[i].prev) {
	  read_var = op->get_prevail()[i].var;
	  read_value = op->get_prevail()[i].prev;
	  break;
      }
    }
    if (read_var == -1) {
	for (int i = 0; i < op->get_pre_post().size(); i++) {
	    if (op->get_pre_post()[i].pre != -1 &&
		state[op->get_pre_post()[i].var] != op->get_pre_post()[i].pre) {
		read_var = op->get_pre_post()[i].var;
		read_value = op->get_pre_post()[i].pre;
		break;
	    }
	}
    }
    
    assert(read_var != -1);
    
    // for (int j = 0; j < g_operators.size(); j++) {
    //   const Operator* o = &(g_operators[j]);
    //   for (int k = 0; k < o->get_pre_post().size(); k++) {
    // 	if (o->get_pre_post()[k].var == read_var &&
    // 	    o->get_pre_post()[k].post == read_value) {
    // 	  insert_without_dup(o, temp);
    // 	}
    //   }
    // }

    pair<int,int> lookup = make_pair(read_var,read_value);
    
    map<pair<int,int>, vector<const Operator*> >::iterator it;
    it = nes_map.find(lookup);
    
    vector<const Operator*> dummy;
    
    if (it != nes_map.end() ) { //found
        dummy = nes_map[lookup];
        for (int j = 0; j < dummy.size(); j++)
            temp.push_back(dummy[j]);
    }
    else {
        for (int j = 0; j < g_operators.size(); j++) {
            const Operator* o = &(g_operators[j]);
            for (int k = 0; k < o->get_pre_post().size(); k++) {
                if (o->get_pre_post()[k].var == read_var &&
                    o->get_pre_post()[k].post == read_value) {
                    insert_without_dup(o, dummy);
                }
            }
        }
        
        nes_map[lookup] = dummy;
        for (int j = 0; j < dummy.size(); j++)
            temp.push_back(dummy[j]);
    }
    
    
    
}

// adds operators to stubborn that interfere with op
void PorSearch::addInterfering(const Operator* op, vector<const Operator*>& temp) {
  
    for (int i = 0; i < interference_relation[op].size(); i++)
    	insert_without_dup(interference_relation[op][i], temp);
    
}


int PorSearch::step() {
  
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        return FAILED;
    }
    SearchNode node = n.first;
    
    State s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;
    
    vector<const Operator *> applicable_ops;
    set<const Operator *> preferred_ops;
    
    g_successor_generator->generate_applicable_ops(s, applicable_ops);
    // This evaluates the expanded state (again) to get preferred ops
    for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
        Heuristic *h = preferred_operator_heuristics[i];
        h->evaluate(s);
        if (!h->is_dead_end()) {
            // In an alternation search with unreliable heuristics, it is
            // possible that this heuristic considers the state a dead end.
            vector<const Operator *> preferred;
            h->get_preferred_operators(preferred);
            preferred_ops.insert(preferred.begin(), preferred.end());
        }
    }
    search_progress.inc_evaluations(preferred_operator_heuristics.size());
    
    // lookup goal variable
    int goal_var = -1;
    int goal_val = -1;
    for (int i = 0; i < g_goal.size(); i++) {
	if (s[g_goal[i].first] != g_goal[i].second) {
	    goal_var = g_goal[i].first;
	    goal_val = g_goal[i].second;
	    break;
	}
    }
    assert(goal_var != -1);
    
    // build strong stubborn set by first collecting operators that
    // write goal_var (simplest way of choosing a disjunctive action
    // landmark)
    pair<int,int> lookup = make_pair(goal_var,goal_val);
    
    map<pair<int,int>, vector<const Operator*> >::iterator it;
    it = nes_map.find(lookup);
    
    vector<const Operator*> stubborn;
        
    if (it != nes_map.end() ) { //found
        stubborn = nes_map[lookup];
    }
    else {
	for (int i = 0; i < g_operators.size(); i++) {
	    const Operator* op = &(g_operators[i]);
	    for (int j = 0; j < op->get_pre_post().size(); j++) {
		if (op->get_pre_post()[j].var == goal_var &&
		    op->get_pre_post()[j].post == goal_val) {
		    insert_without_dup(op, stubborn);
		}
	    }
	}
	
	nes_map[lookup] = stubborn;
    }

    // iteratively insert operators to stubborn according to the
    // definition of strong stubborn sets until a fixpoint is reached
    bool go_ahead = true;
    vector<const Operator*> temp;
    while (go_ahead) {
	go_ahead = false;
	temp.clear();
	
	
	for (int i = 0; i < stubborn.size(); i++) {
	    const Operator* op = stubborn[i];
	    
	    if (op->is_applicable(s)) {
		addInterfering(op, temp);
	    }
	    else {
		addNES(op, s, temp);
	    }
	}

	for (int i = 0; i < temp.size(); i++) {
	    go_ahead |= insert_without_dup(temp[i], stubborn);
	}
	
    }
    
    
    applicable_ops.clear();
    for (int i = 0; i < stubborn.size(); i++) {
      if (stubborn[i]->is_applicable(s))
    	applicable_ops.push_back(stubborn[i]);
    }
    

    for (int i = 0; i < applicable_ops.size(); i++) {
        const Operator *op = applicable_ops[i];
	
        if ((node.get_real_g() + op->get_cost()) >= bound)
            continue;

        State succ_state(s, *op);
        search_progress.inc_generated();
        bool is_preferred = (preferred_ops.find(op) != preferred_ops.end());

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        // update new path
        if (use_multi_path_dependence || succ_node.is_new()) {
            bool h_is_dirty = false;
            for (size_t i = 0; i < heuristics.size(); i++)
                h_is_dirty = h_is_dirty || heuristics[i]->reach_state(
                    s, *op, succ_node.get_state());
            if (h_is_dirty && use_multi_path_dependence)
                succ_node.set_h_dirty();
        }

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.
            for (size_t i = 0; i < heuristics.size(); i++)
                heuristics[i]->evaluate(succ_state);
            succ_node.clear_h_dirty();
            search_progress.inc_evaluated_states();
            search_progress.inc_evaluations(heuristics.size());

            // Note that we cannot use succ_node.get_g() here as the
            // node is not yet open. Furthermore, we cannot open it
            // before having checked that we're not in a dead end. The
            // division of responsibilities is a bit tricky here -- we
            // may want to refactor this later.
            open_list->evaluate(node.get_g() + get_adjusted_cost(*op), is_preferred);
            bool dead_end = open_list->is_dead_end();
            if (dead_end) {
                succ_node.mark_as_dead_end();
                search_progress.inc_dead_ends();
                continue;
            }

            //TODO:CR - add an ID to each state, and then we can use a vector to save per-state information
            int succ_h = heuristics[0]->get_heuristic();
            if (do_pathmax) {
                if ((node.get_h() - get_adjusted_cost(*op)) > succ_h) {
                    //cout << "Pathmax correction: " << succ_h << " -> " << node.get_h() - get_adjusted_cost(*op) << endl;
                    succ_h = node.get_h() - get_adjusted_cost(*op);
                    heuristics[0]->set_evaluator_value(succ_h);
                    open_list->evaluate(node.get_g() + get_adjusted_cost(*op), is_preferred);
                    search_progress.inc_pathmax_corrections();
                }
            }
            succ_node.open(succ_h, node, op);

            open_list->insert(succ_node.get_state_buffer());
            if (search_progress.check_h_progress(succ_node.get_g())) {
                reward_progress();
            }
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(*op)) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                //TODO:CR - test if we should add a reevaluate flag and if it helps
                // if we reopen closed nodes, do that
                if (succ_node.is_closed()) {
                    /* TODO: Verify that the heuristic is inconsistent.
                     * Otherwise, this is a bug. This is a serious
                     * assertion because it can show that a heuristic that
                     * was thought to be consistent isn't. Therefore, it
                     * should be present also in release builds, so don't
                     * use a plain assert. */
                    //TODO:CR - add a consistent flag to heuristics, and add an assert here based on it
                    search_progress.inc_reopened();
                }
                succ_node.reopen(node, op);
                heuristics[0]->set_evaluator_value(succ_node.get_h());
                // TODO: this appears fishy to me. Why is here only heuristic[0]
                // involved? Is this still feasible in the current version?
                open_list->evaluate(succ_node.get_g(), is_preferred);

                open_list->insert(succ_node.get_state_buffer());
            } else {
                // if we do not reopen closed nodes, we just update the parent pointers
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back
                succ_node.update_parent(node, op);
            }
        }
    }

    return IN_PROGRESS;
  
}

static SearchEngine *_parse_por_astar(OptionParser &parser) {
    
    parser.add_option<ScalarEvaluator *>("eval");
    parser.add_option<bool>("pathmax", false,
                            "use pathmax correction");
    parser.add_option<bool>("mpd", false,
                            "use multi-path dependence (LM-A*)");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = 0;
    if (!parser.dry_run()) {
        GEvaluator *g = new GEvaluator();
        vector<ScalarEvaluator *> sum_evals;
        sum_evals.push_back(g);
        ScalarEvaluator *eval = opts.get<ScalarEvaluator *>("eval");
        sum_evals.push_back(eval);
        ScalarEvaluator *f_eval = new SumEvaluator(sum_evals);

        // use eval for tiebreaking
        std::vector<ScalarEvaluator *> evals;
        evals.push_back(f_eval);
        evals.push_back(eval);
        OpenList<state_var_t *> *open = \
            new TieBreakingOpenList<state_var_t *>(evals, false, false);

        opts.set("open", open);
        opts.set("f_eval", f_eval);
        opts.set("reopen_closed", true);
        opts.set("partial_order_reduction", 0); // HACK!
        engine = new PorSearch(opts);
    }

    return engine;
}


static Plugin<SearchEngine> _plugin_por_astar("por_astar", _parse_por_astar);
