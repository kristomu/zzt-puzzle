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

class minmax_solver : public solver {
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