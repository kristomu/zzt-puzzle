#include "dfs.h"
#include "../board.h"
#include <algorithm>
#include <cmath>

// Get the L1 distance. We'll use this as an evaluation function
// for the lack of anything better.

// Higher is better.
int dfs_solver::evaluate(const zzt_board & board, const coord & end_square) const {
	return -end_square.manhattan_dist(board.player_pos);
}

// It would be a good idea to find a way to decisively say "nope, the board
// is unsolvable" ahead of time. Problem is, I have no idea how. XXX

eval_score dfs_solver::inner_solve(zzt_board & board, const coord & end_square,
	int recursion_level, uint64_t & nodes_visited) {

	++nodes_visited;

	if (board.player_pos == end_square) {
		return eval_score(WIN, recursion_level); // An outright win.
	}

	if (recursion_level == 0) {
		return eval_score(evaluate(board, end_square),
			recursion_level);
	}

	// Check if this board state has already been visited at this
	// recursion level or higher. If so, there's no need to go down
	// this branch. If it has been visited, but on a lower recursion
	// level, then it's possible that the extra moves may lead closer
	// to a solution, and we should check anyway.

	// Note that because the "game tree" is a cyclic graph, there's some
	// ambiguity about what value a solution should have if it consists
	// of just going back and forth all the time. On the other hand, it
	// should be possible to "pass", but transposition tables don't allow
	// us to. This may cause somewhat different ultimate scores with or
	// without transposition tables, but shouldn't matter much in practice.
	if (transpositions.find(board.get_hash()) != transpositions.end()) {
		// TODO: If the transposition table says it's a win at a lesser
		// recursion depth, that should also lead to a direct cutoff,
		// because then it's definitely a win given more moves (there's no
		// zugzwang). The difficult part is making sure the PV is as short
		// as possible, which may require inserting "best move" data into the
		// table itself... which we need for IDFS anyway.
		if (transpositions.find(board.get_hash())->second >= recursion_level) {
			return eval_score(LOSS, recursion_level);
		}
	}

	// Quick and dirty hack to keep size limited so we don't crash with
	// an out of memory error. This should really be done by using better
	// logic, e.g. take the hash mod something and if it's already there
	// and has a different untruncated value, only replace if our current
	// depth is less than its depth.

	// TODO: Separate off into a simple fixed size hash table with
	// both always-replace and highest-first strategy.
	// This essentially halts all progress once the table is full;
	// the transposition table is that important...
	if (transpositions.size() < 6e7) {
		transpositions[board.get_hash()] = recursion_level;
	}

	eval_score record_score(LOSS, recursion_level);

	// Determine the move ordering: be greedy and try to go
	// directly to the target first, i.e. minimizing Manhattan
	// distance.
	for (auto & pair: move_ordering) {
		pair.first = end_square.manhattan_dist(board.player_pos +
			get_delta(pair.second));
	}

	std::sort(move_ordering.begin(), move_ordering.end());

	for (auto & pair: move_ordering) {
		direction dir = pair.second;
		if (!board.do_move(dir)) { continue; }

		eval_score solution_score = inner_solve(board, end_square,
			recursion_level-1, nodes_visited);

		if (solution_score > record_score) {
			record_score = solution_score;

			// Copy the solution itself (PV) from the deeper
			// recursion onto our current best guess; plus the
			// direction that got us to the record.
			// At any level, pv[level][0] will then contain the
			// steps to the best solution so far, from the starting
			// point currently being explored at that level.
			principal_variation[recursion_level][0] = dir;
			std::copy(
				principal_variation[recursion_level-1].begin(),
				principal_variation[recursion_level-1].begin() +
					recursion_level-1,
				principal_variation[recursion_level].begin() + 1);
			// Since we may be copying a shorter solution over a longer
			// solution, terminate the longer solution with an IDLE...
			// like this.
			principal_variation[recursion_level][
				recursion_level - record_score.recursion_level] = IDLE;
		}

		// If we get a winning solution, then for all subsequent
		// solutions, limit the depth to that of the winning solution
		// because we're not interested in any longer solutions.
		// This is kind of quick and dirty; will fix later (the solution
		// may be shorter even though it has a high recursion level.)
		// TODO. But already like this it improves things somewhat.

		if (solution_score.score == WIN) {
			recursion_level = record_score.recursion_level + 1;
		}

		board.undo_move();
	}

	return record_score;
}

std::vector<direction> dfs_solver::get_solution() const {

	// Just read off the lower PV row to get the principal variation,
	// until we get to IDLE, which marks the end of the solution.

	std::vector<direction> solution;
	for (direction dir: *principal_variation.rbegin()) {
		if(dir == IDLE) {
			return solution;
		}
		solution.push_back(dir);
	}

	return solution;
}

eval_score dfs_solver::solve(zzt_board & board, const coord & end_square,
	int recursion_level, uint64_t & nodes_visited) {

	transpositions.clear();
	principal_variation = std::vector<std::vector<direction> >(
		recursion_level+1, std::vector<direction>(recursion_level+1, IDLE));

	return inner_solve(board, end_square, recursion_level,
		nodes_visited);
}