/*
 * weakened_ample.cc
 *
 *  Created on: Feb 16, 2012
 *      Author: yusra
 */

#include "stubborn.h"
#include "../successor_generator.h"
#include "../operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../state.h"
#include "../nes_lm_cut.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

StubbornSet::StubbornSet() {
	for (int i = 0; i < g_operators.size(); i++) {
		ops_axioms.push_back(&g_operators[i]);
	}
	for (int i = 0; i < g_axioms.size(); i++) {
		ops_axioms.push_back(&g_axioms[i]);
	}
	Options opts;
	opts.set("cost_type", 1);
	lm_nes = new NesLandmarkCutHeuristic(opts);
	lm_nes->initialize_nes();
	// Dependency Map
	for (int i = 0; i < ops_axioms.size(); i++) {
		const Operator* op1 = ops_axioms[i];
		for (int j = 0; j < ops_axioms.size(); j++) {
			const Operator* op2 = ops_axioms[j];
			if ((op1 != op2) && (isDependent(*op1, *op2))) {
				if (dependency_map.count(op1) == 0) {
					std::set<const Operator*>* empty = new std::set<
							const Operator*>();
					dependency_map.insert(pair<const Operator*, std::set<
							const Operator*> > (op1, *empty));
				}
				dependency_map[op1].insert(op2);
			}
		}
	}

	//	 Enabledness Map
	//	for (int i = 0; i < ops_axioms.size(); i++) {
	//		const Operator* op1 = ops_axioms[i];
	//		for (int j = 0; j < ops_axioms.size(); j++) {
	//			const Operator* op2 = ops_axioms[j];
	//			if ((op1 != op2) && (enable(*op1, *op2))) {
	//				if (enabledness_map.count(op1) == 0) {
	//					std::set<const Operator*>* empty = new std::set<
	//							const Operator*>();
	//					enabledness_map.insert(pair<const Operator*, std::set<
	//							const Operator*> > (op1, *empty));
	//				}
	//				enabledness_map[op1].insert(op2);
	//			}
	//		}
	//	}


//	for (int i = 0; i < ops_axioms.size(); i++) {
//		const Operator* op1 = ops_axioms[i];
//		for (int j = 0; j < ops_axioms.size(); j++) {
//			const Operator* op2 = ops_axioms[j];
//			if ((op1 != op2) && (enable(*op1, *op2))) {
//				//				if (enabledness_op_map.count(op1) == 0) {
//				//					std::set<const Operator*>* empty = new std::set<
//				//							const Operator*>();
//				//					enabledness_op_map.insert(pair<const Operator*, std::set<
//				//							const Operator*> > (op1, *empty));
//				//				}
//				if (enabledness_map.count(op1) == 0) {
//					std::set<pair<const Operator*, int> >* empty =
//							new std::set<pair<const Operator*, int> >();
//					enabledness_map.insert(pair<const Operator*, std::set<pair<
//							const Operator*, int> > > (op1, *empty));
//				}
//				enabledness_map[op1].insert(make_pair(op2, enabling_var));
//			}
//		}
//	}
}

StubbornSet::~StubbornSet() {
}
bool StubbornSet::in_dependency_map(const Operator* op1, const Operator* op2) {
	return (dependency_map[op1].count(op2) > 0);
}

//bool StubbornSet::in_enabledness_op_map(const Operator* op1,
//		const Operator* op2) {
//	return (enabledness_op_map[op1].count(op2) > 0);
//}

bool StubbornSet::in_enabledness_map(const Operator* op1, const Operator* op2,
		int var) {
	return (enabledness_map[op1].count(make_pair(op2, var)) > 0);
}

vector<const Operator *> StubbornSet::computeAmpleSet(State &state) {
	lm_nes->por_compute_heuristic(state);
	std::vector<const Operator*> disj_l = lm_nes->nes;
	std::vector<const Operator*> ops;
	std::set<const Operator*> has_been_processed;
	std::set<const Operator*> stubborn;
	//stubborn.clear();
	g_successor_generator->generate_applicable_ops(state, ops);
	//cout << "The number of all operators =" << ops.size() << endl;
	if (ops.size() > 1) {
		//random_shuffle(ops.begin(), ops.end());
		for (vector<const Operator*>::const_iterator it = disj_l.begin(); it
				!= disj_l.end(); it++) {
			const Operator* o = *it;
			//if (o->is_applicable(state)) {
				stubborn.insert(o);
			//	break;
			//}
		}
//		if (stubborn.empty())
//			stubborn.insert(ops[0]);
		// Constructing the stubborn set
		bool stable = false;
		while (!stable) {
			stable = true;
			for (set<const Operator*>::const_iterator i = stubborn.begin(); i
					!= stubborn.end(); i++) {
				const Operator* cand = *i;
				if (has_been_processed.count(cand) == 0) {
					if (cand->is_applicable(state)) {
						for (int j = 0; j < ops_axioms.size(); j++) {
							const Operator* op = ops_axioms[j];
							if (stubborn.count(op) == 0) {
								if (in_dependency_map(cand, ops_axioms[j])) {
									stubborn.insert(op);
									stable = false;
								}
							}
						}
					} else {

							//-------------------------------------------------------
							lm_nes->nes.clear();
							lm_nes->compute_a_nes(state, cand);

							std::vector<const Operator*> NES = lm_nes->nes;
							for (vector<const Operator*>::const_iterator iter =
									NES.begin(); iter != NES.end(); iter++) {
								if (stubborn.count(*iter) == 0) {
									stubborn.insert(*iter);
									stable = false;
								}
							}
					//	}
					}
					has_been_processed.insert(cand);
				}
			}
		}

		std::vector<const Operator*> ample;
		for (set<const Operator*>::const_iterator it = stubborn.begin(); it
				!= stubborn.end(); it++) {
			const Operator* op1 = *it;
			if (op1->is_applicable(state)) {
				ample.push_back(op1);
			}
		}
		//cout << "size of ample = " << ample.size() << endl;
		return ample;
	}
	return ops;
}

//bool StubbornSet::consistent(const Operator &op1, const Operator &op2) const {
//	// Check op1.pre_post against op2.pre_post and op2.prevail
//	for (int i = 0; i < op1.get_pre_post().size(); i++) {
//		for (int j = 0; j < op2.get_pre_post().size(); j++) {
//			if ((op1.get_pre_post()[i].var == op2.get_pre_post()[j].var)
//					&& (op1.get_pre_post()[i].pre != op2.get_pre_post()[j].pre)
//					&& ((op2.get_pre_post()[j].pre) != -1)) {
//				return false;
//			}
//		}
//		for (int m = 0; m < op2.get_prevail().size(); m++) {
//			if ((op1.get_pre_post()[i].var == op2.get_prevail()[m].var)
//					&& (op1.get_pre_post()[i].pre != op2.get_prevail()[m].prev)
//					&& ((op1.get_pre_post()[i].pre) != -1)) {
//				return false;
//			}
//		}
//	}
//	// Check op1.prevail against op2.pre_post and op2.prevail
//	for (int i = 0; i < op1.get_prevail().size(); i++) {
//		for (int j = 0; j < op2.get_pre_post().size(); j++) {
//			if ((op1.get_prevail()[i].var == op2.get_pre_post()[j].var)
//					&& (op1.get_prevail()[i].prev != op2.get_pre_post()[j].pre)
//					&& ((op2.get_pre_post()[j].pre) != -1)) {
//				return false;
//			}
//		}
//		for (int m = 0; m < op2.get_prevail().size(); m++) {
//			if ((op1.get_prevail()[i].var == op2.get_prevail()[m].var)
//					&& (op1.get_prevail()[i].prev != op2.get_prevail()[m].prev)
//					&& ((op2.get_pre_post()[i].pre) != -1)) {
//				return false;
//			}
//		}
//	}
//	return true;
//}
bool StubbornSet::disable(const Operator &op1, const Operator &op2) const {
	//	if (consistent(op1, op2)) {
	for (int i = 0; i < op1.get_pre_post().size(); i++) {
		for (int j = 0; j < op2.get_pre_post().size(); j++) {
			if ((op1.get_pre_post()[i].var == op2.get_pre_post()[j].var)
					&& (op1.get_pre_post()[i].post != op2.get_pre_post()[j].pre)) {
				return true;
			}
		}
		for (int m = 0; m < op2.get_prevail().size(); m++) {
			if ((op1.get_pre_post()[i].var == op2.get_prevail()[m].var)
					&& (op1.get_pre_post()[i].post != op2.get_prevail()[m].prev)) {
				return true;
			}
		}
	}
	//	}
	return false;
}

bool StubbornSet::commutative(const Operator &op1, const Operator &op2) const {
	//	if (consistent(op1, op2)) {
	for (int i = 0; i < op1.get_pre_post().size(); i++) {
		for (int j = 0; j < op2.get_pre_post().size(); j++) {
			if ((op1.get_pre_post()[i].var == op2.get_pre_post()[j].var)
					&& (op1.get_pre_post()[i].post
							!= op2.get_pre_post()[j].post)) {
				return false;
			}
		}
	}
	//	}
	return true;
}

bool StubbornSet::enable(const Operator &op1, const Operator &op2) {
	if (!disable(op1, op2)) {
		for (int i = 0; i < op1.get_pre_post().size(); i++) {
			for (int j = 0; j < op2.get_pre_post().size(); j++) {
				if ((op1.get_pre_post()[i].var == op2.get_pre_post()[j].var)
						&& (op1.get_pre_post()[i].post
								== op2.get_pre_post()[j].pre)) {
					enabling_var = op1.get_pre_post()[i].var;
					return true;
				}
			}
			for (int m = 0; m < op2.get_prevail().size(); m++) {
				if ((op1.get_pre_post()[i].var == op2.get_prevail()[m].var)
						&& (op1.get_pre_post()[i].post
								== op2.get_prevail()[m].prev)) {
					enabling_var = op1.get_pre_post()[i].var;
					return true;
				}
			}
		}
	}
	return false;
}

bool StubbornSet::isDependent(const Operator &op, const Operator &other_op) const {
//			if ((disable(op, other_op)) || (disable(other_op, op)) || (!commutative(
//					other_op, op))) {

	if(disable(op, other_op))
		return true;

	if(disable(other_op, op))
		return true;

	if(!commutative(other_op, op))
		return true;

	return false;
}

