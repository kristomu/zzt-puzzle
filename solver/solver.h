#pragma once

#include "../board.h"
#include <vector>
#include <unordered_map>

const int WIN = 1e9 - 1, LOSS = -1e9;

// Evaluation score. We first compare the actual score. If there's a tie,
// then the shortest path (highest recursion level at win state) wins.

// TODO? Return PV in the eval_score struct??? It would make it
// impossible to get the PV before solving.

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

// This is a class that suggests what moves to try first for
// different player positions. Note that it produces a mapping
// from player positions to an ordering of the moves, so it
// can't take changing game state into account.
class move_ordering {
	public:
		std::vector<std::vector<
			std::vector<direction> > > move_hint;
		std::vector<direction> get_move_order(coord pos) const;
};

class solver {
	public:
		virtual std::vector<direction> get_solution() const = 0;
		virtual eval_score solve(zzt_board & board,
			const coord & end_square, int recursion_level,
			uint64_t & nodes_visited) = 0;
};