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
		int solution_length;

		eval_score(int score_in, int soln_length_in) {
			score = score_in;
			solution_length = soln_length_in;
		}

		// Here, x > y means "x is better than y"; and
		// x is better than y if x's score is greater or
		// it's got a shorter solution length.

		bool operator>(const eval_score & other) {
			if (score != other.score) {
				return score > other.score;
			}

			// Used to be the other way around, oops
			return solution_length < other.solution_length;
		}

		bool operator>=(const eval_score & other) {
			if (score != other.score) {
				return score > other.score;
			}

			return solution_length <= other.solution_length;
		}

		bool operator==(const eval_score & other) {
			return score == other.score && 
				solution_length == other.solution_length;
		}

		bool operator!=(const eval_score & other) {
			return !(*this == other);
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
			const coord & end_square, int max_solution_length,
			uint64_t & nodes_visited) = 0;
};