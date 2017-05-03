#include "conditional_effect_incremental_lm_cut_heuristic.h"

#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace conditional_effects {

Landmark::Landmark(vector<RelaxedOperator *> &operators, int cost) :
    vector<RelaxedOperator *>(operators),
    cost(cost) {
}

// construction and destruction
IncrementalLandmarkCutHeuristic::IncrementalLandmarkCutHeuristic(const Options &opts)
    : LandmarkCutHeuristic(opts),
      current_state_handle(StateHandle::no_state),
      current_landmarks_cost(0),
      current_parent_state_handle(StateHandle::no_state),
      current_op(0) {
}

IncrementalLandmarkCutHeuristic::~IncrementalLandmarkCutHeuristic() {
}

void IncrementalLandmarkCutHeuristic::initialize() {
    LandmarkCutHeuristic::initialize();
    cout << "Incremental computation will only be used locally" << endl;
}

void IncrementalLandmarkCutHeuristic::reset_operator_costs(const State &state) {
    // Redo cost changes of current landmarks (if there are no current landmarks,
    // this will just reset all costs to base costs)
    LandmarkCutHeuristic::reset_operator_costs(state);
    if (state.get_handle() == current_state_handle) {
        // All landmarks that do not mention op remain landmarks
        for (size_t i = 0; i < current_parent_landmarks.size(); ++i) {
            Landmark &landmark = current_parent_landmarks[i];
            bool keep_landmark = true;
            for (Landmark::iterator it_op = landmark.begin(); it_op != landmark.end(); ++it_op) {
                if ((*it_op)->group->op == current_op) {
                    keep_landmark = false;
                    break;
                }
            }
            if (keep_landmark) {
                reduce_operator_costs(landmark, landmark.cost);
                current_landmarks_cost += landmark.cost;
            }
        }
    }
}

bool IncrementalLandmarkCutHeuristic::reach_state(const State &parent_state,
        const Operator &op, const State &state) {
    current_state_handle = state.get_handle();
    current_landmarks_cost = 0;
    current_op = &op;
    if (parent_state.get_handle() != current_parent_state_handle) {
        current_parent_state_handle = parent_state.get_handle();
        current_parent_landmarks.clear();
        LandmarkCutHeuristic::compute_heuristic(parent_state);
    }
    return false;
}

void IncrementalLandmarkCutHeuristic::discovered_landmark(const State &state,
        vector<RelaxedOperator *> &landmark, int cost) {
    if (state.get_handle() == current_state_handle) {
        current_landmarks_cost += cost;
    } else {
        assert(state.get_handle() == current_parent_state_handle);
        current_parent_landmarks.push_back(Landmark(landmark, cost));
    }
}

int IncrementalLandmarkCutHeuristic::compute_heuristic(const State &state) {
    if (current_state_handle == StateHandle::no_state) {
        // First call to compute_heuristic: initial state
        current_state_handle = state.get_handle();
    }
    assert(state.get_handle() == current_state_handle);
    if (LandmarkCutHeuristic::compute_heuristic(state) == DEAD_END) {
        return DEAD_END;
    }
    return current_landmarks_cost;
}

}

static Heuristic *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new conditional_effects::IncrementalLandmarkCutHeuristic(opts);
}


static Plugin<Heuristic> _plugin("conditional_effect_incremental_lmcut", _parse);
