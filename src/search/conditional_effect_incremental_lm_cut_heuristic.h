#ifndef CONDITIONAL_EFFECT_INCREMENTAL_LM_CUT_HEURISTIC_H
#define CONDITIONAL_EFFECT_INCREMENTAL_LM_CUT_HEURISTIC_H

#include "conditional_effect_lm_cut_heuristic.h"
#include "state_id.h"

namespace conditional_effects {

// wraps vector<RelaxedOperator *> to add costs
// adding or removing operators after creation does *not* update costs (not implemented)
class Landmark : public std::vector<RelaxedOperator *> {
public:
    int cost;
    Landmark(std::vector<RelaxedOperator *> &operators, int cost);
};

class IncrementalLandmarkCutHeuristic : public LandmarkCutHeuristic {
private:
    StateHandle current_state_handle;
    int current_landmarks_cost;
    StateHandle current_parent_state_handle;
    std::vector<Landmark> current_parent_landmarks;
    const Operator *current_op;
protected:
    virtual void initialize();
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);
    virtual void discovered_landmark(const State &state, std::vector<RelaxedOperator *> &landmark, int cost);
    virtual void reset_operator_costs(const State &state);
    virtual int compute_heuristic(const State &state);
public:
    IncrementalLandmarkCutHeuristic(const Options &opts);
    virtual ~IncrementalLandmarkCutHeuristic();
};

}

#endif
