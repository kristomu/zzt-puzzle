#include "generator.h"

#include <random>
#include <algorithm>
#include <omp.h>

typedef std::pair<coord, tile> coord_and_tile;

// Generate a vector of (coordinate, tile) pairs in random order
// for filling a board. This is used to fill a certain number of
// empty tiles in fill_puzzle (below) and also to gradually fill
// in empty tiles for grow_board.

// If varied_tile_selection is true, then the RNG is used to determine
// the fraction of non-empty tiles that should be of each type (slider,
// boulder, solid); if it's false, then there's an equal chance of each.

// NOTE: varied_tile_selection must be false for the old behavior!
// Remember this if we're going to be backwards compatible later.

std::vector<coord_and_tile> get_empty_coord_assignments(
	const zzt_board & board, rng & rng_to_use,
	bool varied_tile_selection) {

	// Create the uniform and categorical distributions. Due to
	// backwards compatibility concerns, both are defined no matter
	// what varied_tile_selection says, but only one is used
	// for each.

	// Separating out the functions might be a better idea, consider
	// this later... TODO?
	std::uniform_int_distribution<int> tile_dist(0, NUM_OBSTACLES-1);
	std::discrete_distribution<int> weighted_tile_dist({});

	if (varied_tile_selection) {
		std::vector<double> weights(NUM_OBSTACLES);

		// Fill the weights vector at random.
		std::uniform_real_distribution<double> weight_dist(0, 1);
		std::generate(weights.begin(), weights.end(),
			[&weight_dist, &rng_to_use]() {
			return weight_dist(rng_to_use);
		});

		weighted_tile_dist = std::discrete_distribution<int>(
			weights.begin(), weights.end());
	}

	// Create a list of coordinates of every empty tile on the board,
	// along with a candidate tile to place there. Although this
	// consumes more entropy, I'm doing it like this because it can
	// be reused for the grow_board function.

	std::vector<coord_and_tile> empty_coord_assignments;
	coord pos;

	for (pos.y = 0; pos.y < board.get_size().y; ++pos.y) {
		for (pos.x = 0; pos.x < board.get_size().x; ++pos.x) {
			if(board.get_tile_at(pos) != T_EMPTY) {
				continue;
			}

			int obstacle_idx;
			if (varied_tile_selection) {
				obstacle_idx = weighted_tile_dist(rng_to_use);
			} else {
				obstacle_idx = tile_dist(rng_to_use);
			}

			coord_and_tile pos_tile;
			pos_tile.first = pos;
			pos_tile.second = obstacles[obstacle_idx];
			empty_coord_assignments.push_back(pos_tile);
		}
	}

	// Shuffle the vector to create a random order.
	std::shuffle(empty_coord_assignments.begin(),
		empty_coord_assignments.end(), rng_to_use);

	return empty_coord_assignments;
}

// Fill empty tiles until tiles_to_fill tiles have been
// filled or everything is full.
void fill_puzzle(zzt_board & board, size_t tiles_to_fill,
	rng & rng_to_use) {

	std::vector<coord_and_tile> empty_coord_assignments =
		get_empty_coord_assignments(board, rng_to_use, true);

	// Then fill the board.
	for (size_t i = 0; i < std::min(empty_coord_assignments.size(),
		tiles_to_fill); ++i) {

		board.set(empty_coord_assignments[i].first,
			empty_coord_assignments[i].second);
	}
}

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size,
	rng & rng_to_use) {

	zzt_board out_board(player_pos, max_size);

	int tiles_to_fill = round(
		(max_size.x * max_size.y) * 1-sparsity);

	fill_puzzle(out_board, tiles_to_fill, rng_to_use);
	return out_board;
}

// Create a puzzle by index.

zzt_board create_indexed_puzzle(double sparsity,
	coord player_pos, coord max_size,
	uint64_t index) {

	// Use the RNG as a deterministic source of
	// numbers with no pattern.

	rng prng(index);

	return create_random_puzzle(sparsity,
		player_pos, max_size, prng);
}

// Grow a board to just before the point where it can't be
// solved.
// min_skips and max_skips denote how many "skips" -- ignoring
// a tile if it produces an unsolvable board -- we handle before
// giving up. A higher number of skips will give a higher chance
// of a complex board, but generation will be slower.
zzt_board grow_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	rng & rng_to_use, int min_skips, int max_skips) {

	// One of the biggest wastes of time in this calculation
	// is to determine if a board is solvable, because we need
	// to (worst case) extend up to the maximum recursion level,
	// and there's no way to tell something is unsolvable short
	// of trying everythin.

	// Therefore, we use a reduced board where everything but
	// the immediate environment around the exit is empty. If
	// the reduced board (which has a considerably smaller
	// state space) is unsolvable, then so is the full board.

	zzt_board board(player_pos, size);
	zzt_board reduced_board(player_pos, size);

	int sumlength = size.x + size.y;

	int skips_remaining = max_skips;
	if (min_skips != max_skips) {
		skips_remaining = rng_to_use.lrand(min_skips, max_skips+1);
	}

	std::vector<coord_and_tile> empty_coord_assignments =
		get_empty_coord_assignments(board, rng_to_use, true);

	int current_depth = 1;
	int filled_squares = 0;

	uint64_t nodes_visited = 0;

	// For statistical purposes: this gives the longest stretch of
	// apparently unsolvable puzzles before a deeper depth uncovers
	// that the puzzle is indeed solvable. This could be used later
	// for a progressive deepening idea where if nothing happens after
	// a certain number of depth increases, we assume the puzzle is
	// unsolvable.
	int max_depth_until_solvable = 0, depth_until_solvable = 0;

	for (auto new_coord_tile: empty_coord_assignments) {

		// Don't overwrite the player position.
		if (new_coord_tile.first == player_pos) {
			continue;
		}

		board.set(new_coord_tile.first, new_coord_tile.second);

		// We always admit solids because they can only reduce
		// the state space.
		if (new_coord_tile.first.manhattan_dist(
			end_square) < 6 || new_coord_tile.second == T_SOLID) {
			reduced_board.set(new_coord_tile.first,
				new_coord_tile.second);
		}

		++filled_squares;

		// Don't print stats if we're running in parallel mode.
		if (!omp_in_parallel()) {
			std::cout << "grow_board: " << filled_squares
				<< " filled squares.   (" << nodes_visited << ")   \r"
				<< std::flush;
		}

		nodes_visited = 0;
		eval_score result(LOSS, 0);

		// Do an interleaved iterative deepening DFS: each time we
		// fail, we increase the depth until we either reach the
		// maximum or succeed. If we reach the maximum, then the
		// board became unsolvable due to the last tile we filled,
		// so remove it and return; if we succeed, it's still solvable,
		// so add another tile.
		do {
			if (!omp_in_parallel()) {
				std::cout << "grow_board/IDDFS: " << current_depth << "  \r" << std::flush;
			}
			// First test the reduced board.
			result = guiding_solver.solve(reduced_board, end_square,
				current_depth, nodes_visited);
			if (result.score > 0) {
				result = guiding_solver.solve(board, end_square,
					current_depth, nodes_visited);
			}
			if (result.score <= 0) {
				++current_depth;
				++depth_until_solvable;
			}
		// Try a maxlength heuristic... seems to work in practice,
		// that if we add something to a board, it'll never take more
		// moves than the max length along an edge to solve... IDK why.
		// Apparently it *is* possible. HACK to see how much performance
		// we get from a looser criterion.
		} while (result.score <= 0 && result.score != LOSS &&
			current_depth < recursion_level && depth_until_solvable < sumlength + 1);

		if (result.score < 0) {
			board.set(new_coord_tile.first, T_EMPTY);
			// ??? Figure out what this is for later.
			if (!omp_in_parallel()) {
				std::cout << "\ngrow_board: unsolvable at " << filled_squares
					<< ", returning.\n";
			}
			#pragma omp critical
				std::cout << "\ngrow_board: max depth until solvable: "
					<< max_depth_until_solvable << "\n";
			if (skips_remaining == 0) {
				return board;
			} else {
				--skips_remaining;
				depth_until_solvable = 0; // required to stop an instant fail
			}
		} else {
			max_depth_until_solvable = std::max(max_depth_until_solvable,
				depth_until_solvable);
			depth_until_solvable = 0;
		}
	}

	return board;
}

zzt_board grow_indexed_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	uint64_t index) {

	rng prng(index);

	return grow_board(player_pos, end_square, size,
		recursion_level, guiding_solver, prng, 0, 3);
}
