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

eval_score dfs_solver::inner_solve(zzt_board & board,
	const coord & end_square, int max_solution_length,
	uint64_t & nodes_visited, eval_score & best_score_so_far) {

	// Must be local or move reordering at one depth will
	// mess with that at another.
	std::array<std::pair<int, direction>, 4> move_ordering = {
			std::pair<int, direction>(0, SOUTH),
			std::pair<int, direction>(0, EAST),
			std::pair<int, direction>(0, WEST),
			std::pair<int, direction>(0, NORTH)};

	++nodes_visited;

	if (board.player_pos == end_square) {
		return eval_score(WIN, 0); // An outright win.
	}

	// We hit the horizon, so while the "solution" won't be
	// an actual solution, it ends here.
	if (max_solution_length <= 0) {
		return eval_score(evaluate(board, end_square), 0);
	}

	// If we have already seen a win, and we're not at a winning
	// square, abort with a loss because there's no way we can
	// improve on the current best outcome.
	if (best_score_so_far == eval_score(WIN, 0)) {
		return eval_score(LOSS, 0);
	}

	// Transposition table check: If we have a definite result at
	// the current state, then there's no need to go down it again.
	if (transposition_enabled &&
		(transpositions.find(board.get_hash()) != transpositions.end())) {

		auto pos = transpositions.find(board.get_hash());
		// If we have a win at this length or shorter, return it
		// immediately; we can't do better.
		if (pos->second.score == WIN && pos->second.solution_length <= max_solution_length) {
			return pos->second;
		}
		// If we have something that's not a win at this length or longer,
		// return it immediately; we can't do better either.
		if (pos->second.score < WIN && pos->second.solution_length >= max_solution_length) {
			return pos->second;
		}
	}

	// Looping around without changing the board state is never beneficial,
	// as we can always remove the loop. So if we visit a square that has
	// already been seen, count it as an automatic loss.
	// For some reason, placing this before the TT check makes things *much* slower;
	// I have no idea why.
	if (being_processed.find(board.get_hash()) != being_processed.end()) {
		return eval_score(LOSS, max_solution_length);
	}

	if (transposition_enabled) {
		being_processed.insert(board.get_hash());
	}

	eval_score record_score(LOSS, 0);

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

		// We now need to decrease the solution length for the
		// best score so far (our cutoff). This because if we descend
		// down a node, then clearly the solution, if we have one, just
		// got one node shorter.

		--best_score_so_far.solution_length;

		eval_score solution_score = inner_solve(board, end_square,
			max_solution_length-1, nodes_visited, best_score_so_far);

		++best_score_so_far.solution_length;

		if (solution_score.solution_length < 0) {
			throw std::logic_error("solution_length < 0");
		}

		if (solution_score > record_score &&
			solution_score >= best_score_so_far) {

			record_score = solution_score;
			best_score_so_far = solution_score;

			int actual_length = record_score.solution_length + 1;

			// Copy the solution itself (PV) from the deeper
			// recursion onto our current best guess; plus the
			// direction that got us to the record.
			// At any level, pv[level][0] will then contain the
			// steps to the best solution so far, from the starting
			// point currently being explored at that level.
			principal_variation[actual_length][0] = dir;
			std::copy(
				principal_variation[record_score.solution_length].begin(),
				principal_variation[record_score.solution_length].begin() +
					record_score.solution_length,
				principal_variation[actual_length].begin() + 1);
			// Since we may be copying a shorter solution over a longer
			// solution, terminate the longer solution with an IDLE...
			// like this.
			principal_variation[actual_length][actual_length+1] = IDLE;

			// If the record is a win, then we don't care about longer
			// wins, so adjust the max solution length accordingly.
			if (record_score.score == WIN) {
				max_solution_length = std::min(max_solution_length,
					record_score.solution_length);
				// The Manhattan distance places a limit on how far
				// we can go. If we can't possibly get to the end square
				// in the moves allotted, and the record is a win, then
				// just break right here. (If the record is not a win,
				// we could possibly get a better heuristic value.)
				// It might be possible to prune even more broadly by
				// making use of the evaluation function being a
				// Manhattan distance too. Not done yet. XXX
				if (record_score.solution_length <
					end_square.manhattan_dist(board.player_pos)) {
					max_solution_length = 0;
				}
			}
		}

		board.undo_move();
	}

	// Increment solution length because we added a move.
	// Then add to the TT and return!
	record_score.solution_length += 1;

	if (transposition_enabled) {
		transpositions[board.get_hash()] = record_score;
		being_processed.erase(board.get_hash());
	}

	return record_score;
}

std::vector<direction> dfs_solver::get_solution() const {

	// Just read off the lower PV row to get the principal variation,
	// until we get to IDLE, which marks the end of the solution.

	std::vector<direction> solution;
	for (direction dir: principal_variation[last_solution_length]) {
		if(dir == IDLE) {
			return solution;
		}
		solution.push_back(dir);
	}

	return solution;
}

eval_score dfs_solver::solve(zzt_board & board, const coord & end_square,
	int max_solution_length, uint64_t & nodes_visited) {

	transpositions.clear();
	principal_variation = std::vector<std::vector<direction> >(
		max_solution_length+2, std::vector<direction>(max_solution_length+2, IDLE));

	eval_score bound(LOSS-1, max_solution_length+1);

	eval_score best = inner_solve(board, end_square, max_solution_length,
		nodes_visited, bound);

	last_solution_length = best.solution_length;

	return best;
}