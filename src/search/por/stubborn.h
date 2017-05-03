/*
 * weakened_ample.h
 *
 *  Created on: Feb 16, 2012
 *      Author: yusra
 */

#ifndef STUBBORN_H_
#define STUBBORN_H_

#include "../globals.h"
#include "../operator.h"
#include "../state.h"
#include "../nes_lm_cut.h"
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <utility>
#include <vector>



using namespace std;

class StubbornSet {
private:
	bool commutative(const Operator &op1, const Operator &op2) const;
	bool disable(const Operator &op1, const Operator &op2) const;
	bool enable(const Operator &op1, const Operator &op2);
	//bool consistent(const Operator &op1, const Operator &op2) const;
	bool isDependent(const Operator &op, const Operator &other_op) const;
	bool in_dependency_map(const Operator* op1, const Operator* op2);
//	bool in_enabledness_map(const Operator* op1, const Operator* op2) const;
	bool in_enabledness_map(const Operator* op1, const Operator* op2, int var);
    std::vector<const Operator*> ops_axioms;
    std::map<const Operator*, std::set<const Operator*> > dependency_map;
  //  std::map<const Operator*, std::set<const Operator*> > enabledness_map;
    std::map<const Operator*, std::set<const Operator*> > enabledness_op_map;
    bool in_enabledness_op_map(const Operator* op1, const Operator* op2) ;
    std::map<const Operator*, std::set<pair<const Operator*, int> > > enabledness_map;
    NesLandmarkCutHeuristic* lm_nes;
    int enabling_var;

public:
	StubbornSet();
	~StubbornSet();

	vector<const Operator *> computeAmpleSet(State &state);
};

#endif
