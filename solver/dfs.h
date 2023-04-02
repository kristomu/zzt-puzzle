#pragma once

#include "solver.h"
#include <unordered_set>

class dfs_solver : public solver {
	private:
		// The first part of the transposition table is the value
		// from that position, and the second is the recursion level.
		// We need to keep track of the recursion level because we
		// can't accept table matches that are closer to the root
		// than we are.
		std::unordered_map<uint64_t, eval_score> transpositions;
		std::unordered_set<uint64_t> being_processed;
		std::vector<std::vector<direction> > principal_variation;

		// Evaluation function (higher is better)
		int evaluate(const zzt_board & board,
			const coord & end_square) const;

		eval_score inner_solve(zzt_board & board,
			const coord & end_square, int max_solution_length,
			uint64_t & nodes_visited, eval_score & best_score_so_far);

		bool transposition_enabled = true;
		int last_solution_length = 0;

	public:
		std::vector<direction> get_solution() const;

		eval_score solve(zzt_board & board,
			const coord & end_square, int max_solution_length,
			uint64_t & nodes_visited);

		// For debugging purposes.
		void set_transposition_table_use(bool use) {
			transposition_enabled = use;
		}
};