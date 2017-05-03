#include "metis_lm_cut_heuristic.h"

#include "option_parser.h"
#include "plugin.h"

#include "incremental_lm_cut_heuristic.h"
#include "conditional_effect_incremental_lm_cut_heuristic.h"


static Heuristic *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return 0;
    } else {
        bool has_conditional_effects = false;
        for (int i = 0; i < g_operators.size(); i++) {
            const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
            for (int j = 0; j < pre_post.size(); j++) {
                const vector<Prevail> &cond = pre_post[j].cond;
                if (!cond.empty()) {
                    has_conditional_effects = true;
                    break;
                }
            }
            if (has_conditional_effects)
                break;
        }
        if (has_conditional_effects) {
            cout << "Task has conditional effects, using LMcut heuristic with "
                    "basic support for conditional effects." << endl;
            return new conditional_effects::IncrementalLandmarkCutHeuristic(opts);
        } else {
            cout << "Task has no conditional effects, using original LMcut heuristic." << endl;
            return new IncrementalLandmarkCutHeuristic(opts);
        }
    }
}


static Plugin<Heuristic> _plugin("metis_ilmcut", _parse);
