#pragma once

#include "solver.h"

// Meta-class that turns any solver into an iterative
// deepening one.

template<typename T> class iddfs_solver : public solver {
	private:
		T baseline_solver;

	public:
		std::vector<direction> get_solution() const {
			return baseline_solver.get_solution();
		}

		eval_score solve(zzt_board & board,
			const coord & end_square, int recursion_level,
			uint64_t & nodes_visited) {

			eval_score partial_solve(LOSS, 0);

			for (int i = 1; i < recursion_level; ++i) {
				partial_solve = baseline_solver.
					solve(board, end_square, i, nodes_visited);

				// If the result is definite, return it.
				// TODO: Use some kind of get_win_state instead
				// of constants so that we can consider different
				// degrees of victory or failure.
				if (partial_solve.score == WIN || 
					partial_solve.score == LOSS) {
					return partial_solve;
				}
			}

			return partial_solve;
		}
};