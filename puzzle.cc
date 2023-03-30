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

	zzt_board test_board = create_random_puzzle(0.8, player_pos, max);

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

// Get the L1 distance. We'll use this as an evaluation function
// for the lack of anything better.
double manhattan_dist(const coord & a, const coord & b) {
	return fabs(a.x-b.x) + fabs(a.y-b.y);
}

// Higher is better.
int evaluate(const zzt_board & board, const coord & end_square) {
	return -manhattan_dist(board.player_pos, end_square);
}

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

// It would be a good idea to find a way to decisively say "nope, the board
// is unsolvable" ahead of time. Problem is, I have no idea how. XXX

eval_score solve(zzt_board & board, const coord & end_square,
	std::vector<std::vector<direction> > & principal_variation,
	std::unordered_map<uint64_t, int> & transpositions, int recursion_level,
	uint64_t & nodes_visited) {

	++nodes_visited;

	//std::cout << "recursion level " << recursion_level << "[" << board.get_hash() << "]" << std::endl;

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
	if (transpositions.size() < 2e7) {
		transpositions[board.get_hash()] = recursion_level;
	}

	eval_score record_score(LOSS, recursion_level);

	for (direction dir: {SOUTH, EAST, NORTH, WEST}) {
		if (!board.do_move(dir)) { continue; }

		eval_score solution_score = solve(board, end_square,
			principal_variation, transpositions,
			recursion_level-1, nodes_visited);

		// TODO??? If we get a winning solution, then for all subsequent
		// solutions, limit the depth to that of the winning solution
		// because we're not interested in any longer solutions.

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

		board.undo_move();
	}

	return record_score;
}

std::vector<direction> get_solution(const
	std::vector<std::vector<direction> > & principal_variation) {

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

// Iterative deepening.
// I should provide the PV later...
/*
int idfs_solve(zzt_board & board, const coord & end_square,
	int max_recursion_level) {

	int score;

	for (int recur = 0; recur < max_recursion_level; ++recur) {
		std::unordered_map<uint64_t, int> transpositions;

		score = find_solution(board, end_square,
			transpositions, recur);

		if (score == WIN || score == LOSS) {
			return score;
		}
	}

	return score;
}*/

// Puzzle generation idea around the property that a slider puzzle
// never goes from unsolvable to solvable by adding more non-empty
// components to it.

const int MAX_DEPTH = 30;

int turns_to_solve(zzt_board board, coord end_square, int max_steps) {
	std::unordered_map<uint64_t, int> transpositions;

	std::vector<std::vector<direction> > pv(max_steps+1,
		std::vector<direction>(max_steps+1, IDLE));

	uint64_t nodes_visited;

	eval_score result = solve(board, end_square,
		pv, transpositions, max_steps,
		nodes_visited);

	// There may be an off-by-one here, fix later
	if (result.score > 0) {
		return max_steps-result.recursion_level;
	} else {
		return -1;
	}
}

int poor_mans_idfs_turns_to_solve(zzt_board board, coord end_square) {
	int max_steps = MAX_DEPTH;

	int shallow_search = turns_to_solve(board, end_square, 17);
	if (shallow_search > 0) {
		return shallow_search;
	}

	return turns_to_solve(board, end_square, max_steps);
}

zzt_board grow_board(double sparsity, coord player_pos, coord end_square, coord size) {
	bool found_board = false;

	zzt_board out_board(player_pos, size);
	zzt_board recordholder = out_board;
	int record = -1;

	for (int i = 0;!found_board; ++i) {
		if (i % 16 == 15) { sparsity = 1 - (1 - sparsity)*0.9;}
		std::cout << i << ", " << sparsity << "   \r" << std::flush;

		out_board = zzt_board(player_pos, size);

		for (int i = 0; i < 1000; ++i) {
			zzt_board mutation = out_board;
			fill_puzzle(mutation, sparsity);

			int candidate_quality = poor_mans_idfs_turns_to_solve(mutation, end_square);
			if (candidate_quality > record) {
				found_board = true;
				record = candidate_quality;
				recordholder = mutation;
				out_board = mutation;
			}
		}
	}

	return recordholder;
}

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
//		- IDFS based on this
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

	for (direction d: path) {
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
		int after_dist = manhattan_dist(after_move, end_square);

		bool move_is_naive = true;

		for (direction other_dir: {EAST, WEST, NORTH, SOUTH}) {
			if (other_dir == dir_taken) { continue; }

			int candidate_dist = manhattan_dist(
				pos + get_delta(other_dir), end_square);
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
	size_t i, j;

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

int main() {
	// We gather statistics about the boards as potential inputs to
	// a linear model, to get a good idea of what makes a board hard.
	// Because preparing a ton of puzzles is tedious, we should pick ones
	// that have different values of the inputs. This map keeps track
	// of them.
	std::map<int, std::vector<double> > stats_by_id;
	std::unordered_map<uint64_t, int> transpositions;

	for (int i = 0; i < 1e7; ++i) {

		srand48(i);
		srand(i);
		srandom(i);

		int span = 2 + random() % 6;

		coord max(4 + random() % 4, 4 + random() % 4);
		coord player_pos(0, 3);
		//coord end_square(max.x-1, 3);
		coord end_square(max.x-1, max.y-1);

		// didn't use to have the 0.67x
		double sparsity = /*0.67 **/ round(drand48()*100)/100.0;

		transpositions.clear();

		zzt_board test_board =
			create_random_puzzle(sparsity, player_pos, max);
		if (i % 2 == 0) {
			test_board = grow_board(sparsity, player_pos, end_square, max);
		}

		/*return(-1);*/

		// TODO: Report the actual number of non-spaces.
		int max_steps = MAX_DEPTH;

		std::vector<std::vector<direction> > pv(max_steps+1,
			std::vector<direction>(max_steps+1, IDLE));

		uint64_t nodes_visited = 0;

		eval_score result = solve(test_board, end_square,
			pv, transpositions, max_steps, nodes_visited);

		//int score = idfs_find_solution(test_board, end_square, 30);

		if (result.score > 0 ) { // && result.recursion_level < max_steps - 5 /* 14 */) {
			transpositions.clear();

			std::vector<direction> solution = get_solution(pv);
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

			std::vector<double> stats = {
														// TODO, add intercept and update
														// these values.
				(double)solution.size(),				// first guess: -109.538
				(double)max.x,							// first guess: -64.6429
				(double)max.y,							// first guess: -119.6481
				(double)(max.y, max.x * max.y),			// first guess: 44.00
				solution_turns,							// first guess: 82.425
				mean_change,							// first guess: 1440.206
				max_change,								// first guess: -79.979
				start_finish_changes,					// first guess: -76.2424
				unusual_moves,							// first guess: 37.38125
				unusual_proportion,						// first guess: 2505.12601424
				real_board_sparsity,					// first guess: 968.24895351
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

			print_useful_stats(20, stats_by_id);
		}
	}
}
