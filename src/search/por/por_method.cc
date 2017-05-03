#include "por_method.h"

// TODO: We can get rid of the following includes once we have the
//       plugin mechanism in place for this.
#include "expansion_core.h"
#include "simple_stubborn_sets.h"
#include "sss_expansion_core.h"
#include "../option_parser.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace POR {
PORMethod::PORMethod() {}

PORMethod::~PORMethod() {}

NullPORMethod::NullPORMethod() {}

NullPORMethod::~NullPORMethod() {}

void NullPORMethod::dump_options() const {
    cout << "partial order reduction method: none" << endl;
}

PORMethodWithStatistics::PORMethodWithStatistics()
    : unpruned_successors_generated(0),
      pruned_successors_generated(0) {
}

PORMethodWithStatistics::~PORMethodWithStatistics() {
}

void PORMethodWithStatistics::prune_operators(
    const State &state, std::vector<const Operator *> &ops) {
    unpruned_successors_generated += ops.size();
    do_pruning(state, ops);
    pruned_successors_generated += ops.size();
}

void PORMethodWithStatistics::dump_statistics() const {
    cout << "total successors before partial-order reduction: "
         << unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << pruned_successors_generated << endl;
}

bool PORMethodWithStatistics::sufficient_por_pruning(double ratio) const{
    if(unpruned_successors_generated == 0) {
        return true;
    }

    return ((pruned_successors_generated / unpruned_successors_generated) < (1-ratio));
}

void add_parser_option(OptionParser &parser, const string &option_name) {
    // TODO: Use the plug-in mechanism here.
    vector<string> por_methods;
    vector<string> por_methods_doc;

    por_methods.push_back("NONE");
    por_methods_doc.push_back("no partial order reduction");
    por_methods.push_back("EXPANSION_CORE");
    por_methods_doc.push_back("expansion core");
    por_methods.push_back("SIMPLE_STUBBORN_SETS");
    por_methods_doc.push_back("simple stubborn sets");
    por_methods.push_back("SSS_EXPANSION_CORE");
    por_methods_doc.push_back("strong stubborn sets that dominate expansion core");
    parser.add_enum_option(option_name, por_methods,
               "partial-order reduction method to be used",
               "NONE",
               por_methods_doc);

    parser.add_option<bool>("on-the-fly-interference",
                            "on-the-fly-interference-computation",
                            "false");
    parser.add_option<bool>("on-the-fly-achievers",
                            "on-the-fly-achievers-computation",
                            "false");
    parser.add_option<int>("num-por-probes",
                           "number-of-expansions",
                           OptionParser::to_str(std::numeric_limits<int>::max()));
    parser.add_option<int>("por-mem-bound",
                           "memory-size",
                           OptionParser::to_str(std::numeric_limits<int>::max()));
    parser.add_option<double>("required-pruning-ratio",
                              "pruning-ratio",
                              "0");
    parser.add_option<bool>("conservative-otf",
                            "conservative-otf",
                            "false");
}

PORMethod *create(int option, bool interference, bool achievers, int mem_bound, bool conservative_otf) {
    if (option == NO_POR_METHOD) {
        return new NullPORMethod;
    } else if (option == EXPANSION_CORE) {
        return new ExpansionCore;
    } else if (option == SIMPLE_STUBBORN_SETS) {
        return new SimpleStubbornSets(interference, achievers, mem_bound, conservative_otf);
    } else if (option == SSS_EXPANSION_CORE) {
        return new SSS_ExpansionCore;
    } else {
        cerr << "internal error: unknown POR method " << option << endl;
        abort();
    }
}
}
