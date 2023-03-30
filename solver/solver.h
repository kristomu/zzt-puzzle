#pragma once

#include "../board.h"
#include <vector>
#include <unordered_map>

const int WIN = 1e9 - 1, LOSS = -1e9;

// Evaluation score. We first compare the actual score. If there's a tie,
// then the shortest path (highest recursion level at win state) wins.
class eval_score {
	public:
		int score;
		int recursion_level;

		eval_score(int score_in, int recur_level_in) {
			score = score_in;
			recursion_level = recur_level_in;
		}

		bool operator>(const eval_score & other) {
			if (score != other.score) {
				return score > other.score;
			}

			return recursion_level > other.recursion_level;
		}
};

class solver {
	private:
		std::unordered_map<uint64_t, int> transpositions;
		std::vector<std::vector<direction> > principal_variation;

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