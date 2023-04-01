#include "generator.h"

#include <algorithm>
#include <omp.h>

typedef std::pair<coord, tile> coord_and_tile;

// Generate a vector of (coordinate, tile) pairs in random order
// for filling a board. This is used to fill a certain number of
// empty tiles in fill_puzzle (below) and also to gradually fill
// in empty tiles for grow_board.

std::vector<coord_and_tile> get_empty_coord_assignments(
	const zzt_board & board, std::mt19937 & rng) {

	std::uniform_int_distribution<int> tile_dist(0, NUM_OBSTACLES-1);

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

			coord_and_tile pos_tile;
			pos_tile.first = pos;
			pos_tile.second = obstacles[tile_dist(rng)];
			empty_coord_assignments.push_back(pos_tile);
		}
	}

	// Shuffle the vector to create a random order.
	std::shuffle(empty_coord_assignments.begin(),
		empty_coord_assignments.end(), rng);

	return empty_coord_assignments;
}

// Fill empty tiles until tiles_to_fill tiles have been
// filled or everything is full.
void fill_puzzle(zzt_board & board, size_t tiles_to_fill,
	std::mt19937 & rng) {

	std::vector<coord_and_tile> empty_coord_assignments =
		get_empty_coord_assignments(board, rng);

	// Then fill the board.
	for (size_t i = 0; i < std::min(empty_coord_assignments.size(),
		tiles_to_fill); ++i) {

		board.set(empty_coord_assignments[i].first,
			empty_coord_assignments[i].second);
	}
}

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size,
	std::mt19937 & rng) {

	zzt_board out_board(player_pos, max_size);

	int tiles_to_fill = round(
		(max_size.x * max_size.y) * 1-sparsity);

	fill_puzzle(out_board, tiles_to_fill, rng);
	return out_board;
}

// Create a puzzle by index.

zzt_board create_indexed_puzzle(double sparsity,
	coord player_pos, coord max_size,
	uint64_t index) {

	// Use the RNG as a deterministic source of
	// numbers with no pattern.

	std::mt19937 prng(index);

	return create_random_puzzle(sparsity,
		player_pos, max_size, prng);
}

// Grow a board to just before the point where it can't be
// solved.
zzt_board grow_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	std::mt19937 & rng) {

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

	std::vector<coord_and_tile> empty_coord_assignments =
		get_empty_coord_assignments(board, rng);

	int current_depth = 1;
	int filled_squares = 0;

	uint64_t nodes_visited = 0;

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
			}
		} while (result.score <= 0 && result.score != LOSS && current_depth < recursion_level);

		if (result.score < 0) {
			board.set(new_coord_tile.first, T_EMPTY);
			if (!omp_in_parallel()) {
				std::cout << "\ngrow_board: unsolvable at " << filled_squares
					<< ", returning.\n";
			}
			return board;
		}
	}

	return board;
}

zzt_board grow_indexed_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	uint64_t index) {

	std::mt19937 prng(index);

	return grow_board(player_pos, end_square, size,
		recursion_level, guiding_solver, prng);
}