#pragma once

#include "solver.h"

class minmax_solver : public solver {
	private:
		std::unordered_map<uint64_t, int> transpositions;
		std::vector<std::vector<direction> > principal_variation;

		std::array<std::pair<int, direction>, 4> move_ordering = {
			std::pair<int, direction>(0, SOUTH),
			std::pair<int, direction>(0, EAST),
			std::pair<int, direction>(0, WEST),
			std::pair<int, direction>(0, NORTH)};

		// Evaluation function (higher is better)
		int evaluate(const zzt_board & board,
			const coord & end_square) const;

		eval_score inner_solve(zzt_board & board,
			const coord & end_square, int recursion_level,
			uint64_t & nodes_visited);

	public:
		std::vector<direction> get_solution() const;

		eval_score solve(zzt_board & board,
			const coord & end_square, int recursion_level,
			uint64_t & nodes_visited);
};