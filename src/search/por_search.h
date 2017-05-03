// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-
////////////////////////////////////////////////////////////////////
//
// $Id$
//
////////////////////////////////////////////////////////////////////

#ifndef POR_SEARCH_H
#define POR_SEARCH_H

#include "eager_search.h"


class PorSearch : public EagerSearch {

protected:
    
    std::map<const Operator*, std::vector<const Operator*> > interference_relation;
    std::map<std::pair<int,int>, std::vector<const Operator*> > nes_map;
    
    int step();
    void addNES(const Operator* op, const State& state, std::vector<const Operator*>& temp);
    void addInterfering(const Operator* op, std::vector<const Operator*>& temp);
    bool interfere(const Operator* op1, const Operator* op2) const;
public:
    PorSearch(const Options &opts);

private:
    void init_interference();


};

#endif /* POR_SEARCH_H */
