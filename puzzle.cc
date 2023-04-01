#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iterator>
#include <cstddef>
#include <numeric>
#include <limits>
#include <vector>
#include <cmath>
#include <list>
#include <map>
#include <set>

#include "coord.h"
#include "board.h"
#include "generator.h"

#include "solver/all.h"

// Things that need to be done:
//	- REFACTORING. Board definitely deserves its own file.
//		And the board generator should be put somewhere and
//		finalized so that rearranging the rest of the code won't
//		invalidate the board IDs (currently based on random seeds).
//	- Parallelization (openmp or queues).
//	- Move ordering in solve(), try the naive approach first
//	- Iterative deepening DFS, try the previous iteration's PV first
//	- Some way to tell that a board is definitely unsolvable, or variant
//		solutions that can check if it is unsolvable (a slider puzzle can
//		never be solved if it's unsolvable when all the sliders are boulders, say)
//	- The TODO in solve regarding early pruning (alpha beta analog) where if the
//		solution is 4 moves out and we have 10 moves left, set the moves left to 4
//		because there's no point in going deeper.

void test_one() {
	coord max(random()%10 + 2, random()%10 + 2);
	coord player_pos(random() % max.x, random() % max.y);

	// TODO: make random
	zzt_board test_board = create_indexed_puzzle(0.8, player_pos, max, 1);

	//std::cout << "===============" << std::endl;
	//std::cout << "Starting off:" << std::endl;
	zzt_board before = test_board;
	//test_board.print();
	//std::cout << "\n";
	int moves_made = 0;
	for (int j = 0; j < random() % 4 + 1; ++j) {
		if (test_board.do_move((direction)(random() % 4))) {
			++moves_made;
			if (test_board != before && (test_board.get_hash() == before.get_hash())) {
				throw std::runtime_error("But the hash didn't change!");
			}
			//std::cout << "Made a move:\n";
			//test_board.print();
		}
	}

	for (int k = 0; k < moves_made; ++k) {
		test_board.undo_move();
		//std::cout << "After undoing:\n";
		//test_board.print();
	}

	if (test_board != before) {
		throw std::runtime_error("Undo seems to not be working! That's a bug.");
	}

}

void print_solution(const std::vector<direction> & solution) {
	for (direction dir: solution) {
		switch(dir) {
			case NORTH: std::cout << "N "; break;
			case SOUTH: std::cout << "S "; break;
			case EAST: std::cout << "E "; break;
			case WEST: std::cout << "W "; break;
			default:
				throw std::logic_error("print_solution: Unknown direction!");
		}
	}
	std::cout << std::endl;
}

const int MAX_DEPTH = 40;

// TODO: Get the following stats:
//		- number of solutions
//		- number of pushes done in the PV (as opposed to
//			just walking on empties)
//		- number of tiles pushed/discrepancy from initial state
//		- number of direction reversals done in the PV (~ RLE coding length) [DONE]
//		- departure from naive guess at each PV step (counterintuitive moves)
//		- length (well duh :P)
// Then some linear programming/curve fitting should give a difficulty
// estimate. I may include a bunch of variables and then use good old
// compressed sensing, really... Also include the "way too simple"
// puzzles where the player is in a corner or the exit is in one.

// https://sci-hub.st/http://dx.doi.org/10.1109/cig.2015.7317913
// If I use ratings to get feedback, I should remember that they're
// potentially arbitrarily affinely scaled.

// Also add this functionality:
//		- Store best move in the transposition table.
//		- IDDFS based on this
//		- Move reordering based on greedy heuristic

// I can't reproduce generated boards, and there seems to be a bug where some
// solutions are much too long (or alternatively, it fails to find a short
// solution). Both of these need to be fixed: I have to unify the random number
// generators so I can reproduce the boards -- and probably just create a random
// puzzle, not *grow* them, as the latter depends on the solve() function...
// TODO? Is there any chance I can do this by first of April?

/////////////////////////// Statistics ////////////////////

// Returns the number of turns required (e.g. north to east to south is
// three turns). Presumably more complex puzzles have more turns.
int get_path_turns(const std::vector<direction> & solution) {
	direction cur_dir = IDLE;
	int turns_so_far = 0;

	for (direction d: solution) {
		if (d != cur_dir) {
			++turns_so_far;
			cur_dir = d;
		}
	}

	return turns_so_far;
}

// Calculate how much a move changes a board.
int count_changes(zzt_board before, zzt_board after) {

	coord where;
	int changes = 0;

	for (where.y = 0; where.y < before.get_size().y; ++where.y) {
		for (where.x = 0; where.x < before.get_size().x; ++where.x) {
			if (before.get_tile_at(where) != after.get_tile_at(where)) {
				++changes;
			}
		}
	}

	return changes;
}

// Count changes along as the player moves on a path.
// The board is really const because we'll undo every move we do,
// but C++ doesn't know that.
std::vector<int> count_changes(zzt_board board,
	const std::vector<direction> & path) {

	std::vector<int> changes;

	for (direction d: path) {
		zzt_board before = board;
		if (!board.do_move(d)) {
			throw std::runtime_error("count_changes: Can't make that move!");
		}
		changes.push_back(count_changes(before, board));
	}

	for (size_t i = 0; i < path.size(); ++i) {
		board.undo_move();
	}

	return changes;
}

// Count the number of changes between the starting and the
// end position.
int count_start_end_changes(zzt_board board,
	const std::vector<direction> & path) {

	zzt_board before = board;

	for (direction d: path) {
		if (!board.do_move(d)) {
			throw std::runtime_error("count_changes: Can't make that move!");
		}
	}

	return count_changes(before, board);
}

// Get the fraction of tiles that are not empty.
double get_density(const zzt_board & board) {
	coord where;
	int nonempty = 0;

	for (where.y = 0; where.y < board.get_size().y; ++where.y) {
		for (where.x = 0; where.x < board.get_size().x; ++where.x) {
			if (board.get_tile_at(where) != T_EMPTY) {
				++nonempty;
			}
		}
	}

	return nonempty/(double)(board.get_size().x * board.get_size().y);
}

// A move is "counterintuitive" or "unusual" if it
// isn't one of the moves that go directly towards the target.

int count_unusual_moves(const zzt_board & board,
	const coord & end_square,
	const std::vector<direction> & solution_path) {

	int unusual_moves = 0;
	coord pos = board.player_pos;

	for (direction dir_taken: solution_path) {
		// For each move, check if the distance after making this
		// move is the same as after making the naive move. NOTE:
		// This deliberately doesn't check if the naive move
		// direction is blocked.
		coord after_move = pos + get_delta(dir_taken);
		int after_dist = end_square.manhattan_dist(after_move);

		bool move_is_naive = true;

		for (direction other_dir: {EAST, WEST, NORTH, SOUTH}) {
			if (other_dir == dir_taken) { continue; }

			int candidate_dist = end_square.manhattan_dist(
				pos + get_delta(other_dir));
			move_is_naive &= (candidate_dist >= after_dist);
		}

		if (!move_is_naive) {
			++unusual_moves;
		}

		// move to the next position and repeat.
		pos = after_move;
	}

	return unusual_moves;
}

double get_unusual_dir_proportion(const zzt_board & board,
	const coord & end_square,
	const std::vector<direction> & solution_path) {

	return count_unusual_moves(board, end_square,
		solution_path)/(double)solution_path.size();
}

// Euclidean distance. Yeah, I know the Manhattan distance is
// for coordinates and this is for vectors, it's ugly. Perhaps
// I'll fix it later? TODO?
double euclidean_dist(const std::vector<double> & x,
	const std::vector<double> & y) {

	double squared_error = 0;

	for (size_t i = 0; i < std::min(x.size(), y.size()); ++i) {
		if (!finite(x[i]) || !finite(y[i])) {
			throw std::invalid_argument("Trying to take distance of non-finite values");
		}
		squared_error += (x[i]-y[i]) * (x[i]-y[i]);
	}

	return sqrt(squared_error);
}

// A given number of times, create a vector that has random
// values between minimum and maximum on each dimension, then
// pick the vector closest to this random vector that has not
// already been picked. Finally, print them all out. This is
// sort of a poor man's grid but not really, but should help the
// linear regression determine effects better than if I were to
// just randomly sample all results seen (which are heavily skewed).
void print_useful_stats(size_t desired_num_points,
	const std::map<int, std::vector<double> > & stats_by_id) {

	// If we asked for more points than there are, fix that.
	desired_num_points = std::min(desired_num_points,
		stats_by_id.size());

	size_t dim = stats_by_id.begin()->second.size();

	std::vector<double>
		minima(dim, std::numeric_limits<double>::infinity()),
		maxima(dim, -std::numeric_limits<double>::infinity());

	std::set<int> seen_IDs;
	size_t i;

	for (auto pos = stats_by_id.begin(); pos != stats_by_id.end();
		++pos) {
		for (i = 0; i < dim; ++i) {
			minima[i] = std::min(minima[i], pos->second[i]);
			maxima[i] = std::max(maxima[i], pos->second[i]);
		}
	}

	for (size_t point_idx = 0; point_idx < desired_num_points; ++point_idx) {
		double record = std::numeric_limits<double>::infinity();
		auto recordholder = stats_by_id.end();

		std::vector<double> random_stat;
		for (i = 0; i < dim; ++i) {
			random_stat.push_back(minima[i] + drand48() * (maxima[i] - minima[i]));
		}

		for (auto pos = stats_by_id.begin(); pos != stats_by_id.end();
			++pos) {
			if (seen_IDs.find(pos->first) != seen_IDs.end()) {
				continue;
			}

			if (euclidean_dist(random_stat, pos->second) < record) {
				record = euclidean_dist(random_stat, pos->second);
				recordholder = pos;
			}
		}

		if (recordholder == stats_by_id.end()) {
			throw std::logic_error("Found no records!");
		}

		seen_IDs.insert(recordholder->first);
		std::cout << "[" << recordholder->first << "]: ";

		// Copy the variables.
		std::copy(recordholder->second.begin(), recordholder->second.end(),
			std::ostream_iterator<double>(std::cout, " "));
		std::cout << "\n";
	}
}

int main(int argc, char ** argv) {
	// We gather statistics about the boards as potential inputs to
	// a linear model, to get a good idea of what makes a board hard.
	// Because preparing a ton of puzzles is tedious, we should pick ones
	// that have different values of the inputs. This map keeps track
	// of them.
	std::map<int, std::vector<double> > stats_by_id;

	bool parallel = false;

	if (argc > 1 && std::string(argv[1]) == "--parallel" ) {
		std::cout << "Enabling parallel mode." << std::endl;
		parallel = true;
	} else {
		std::cout << "Starting serial mode. "
			"Use --parallel to parallelize." << std::endl;
	}

	dfs_solver dfs;
	iddfs_solver<dfs_solver> iddfs;

	// Apparently using omp parallel like this can cause dfs and iddfs
	// to have an undefined state once they've been replicated to the
	// threads. I do this because I don't want to be creating new solvers
	// in memory all the time (including their expensive transposition tables),
	// but something more elegant would probably be preferrable.
	#pragma omp parallel for if(parallel) private(dfs, iddfs)
	for (int i = 0; i < (int)1e7; ++i) {
		// Vary the size of the board but in a predictable way
		// so that we don't have to deal with
		coord max(4 + i % 4, 4 + (i/4) % 4);

		coord player_pos(0, 3);
		coord end_square(max.x-1, max.y-1);

		zzt_board test_board =
			grow_indexed_board(player_pos, end_square,
				max, MAX_DEPTH, dfs, i);

		uint64_t nodes_visited = 0;
		eval_score result = iddfs.solve(test_board, end_square,
			MAX_DEPTH, nodes_visited);

		#pragma omp critical
		if (result.score > 0 ) {
			std::vector<direction> solution = iddfs.get_solution();
			// Get some statistics.
			std::vector<int> changes_with_sol = count_changes(test_board,
				solution);
			double max_change = *std::max_element(changes_with_sol.begin(),
				changes_with_sol.end());
			double mean_change = std::accumulate(changes_with_sol.begin(),
				changes_with_sol.end(), 0) / (double)changes_with_sol.size();
			double solution_turns = get_path_turns(solution);
			double start_finish_changes = count_start_end_changes(test_board, solution);
			double unusual_moves = count_unusual_moves(test_board, end_square, solution);
			double unusual_proportion = get_unusual_dir_proportion(test_board, end_square,
				solution);
			double real_board_sparsity = 1 - get_density(test_board);

			// These are used for my attempts to create a model for
			// how difficult a puzzle is to solve; the more the better
			// (as long as I can endure playing all the puzzles to provide
			// the required data).
			std::vector<double> stats = {
				(double)solution.size(),
				(double)max.x,
				(double)max.y,
				(double)(max.x * max.y),
				solution_turns,
				mean_change,
				max_change,
				start_finish_changes,
				unusual_moves,
				unusual_proportion,
				real_board_sparsity,
				(double)nodes_visited,
				log(nodes_visited)};

			stats_by_id[i] = stats;

			std::cout << "Index is " << i << std::endl;
			test_board.print();
			std::cout << "Index " << i << ": size: " << max.x << ", " << max.y
				<< " = " << max.x * max.y << std::endl;
			std::cout << "Index " << i << ": Solution score: " << result.score << std::endl;
			std::cout << "Index " << i << ": turns in solution " << solution_turns
				<< std::endl;
			std::cout << "Index " << i << ": Changes: mean: " << mean_change
				<< " max: " << max_change << std::endl;
			std::cout << "Index " << i << ": Start-finish change count: " <<
				start_finish_changes << std::endl;
			std::cout << "Index " << i << ": Unusual moves " <<
				unusual_moves << std::endl;
			std::cout << "Index " << i << ": Unusual move proportion " <<
				unusual_proportion << std::endl;
			std::cout << "Index " << i << ": Real sparsity is "
				<< real_board_sparsity << ", solution in " << solution.size()
				<< "/" << result.recursion_level << ": ";
			print_solution(solution);

			std::cout << "Index " << i << ": nodes visited: " << nodes_visited << std::endl;

			std::cout << "Index " << i << ": summary: ";
			std::copy(stats.begin(), stats.end(),
				std::ostream_iterator<double>(std::cout, " "));
			std::cout << std::endl;
		}
	}
}
