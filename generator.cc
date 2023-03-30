#include "generator.h"
#include <algorithm>

// Fill empty tiles until tiles_to_fill tiles have been
// filled or everything is full.
void fill_puzzle(zzt_board & board, size_t tiles_to_fill,
	std::mt19937 & rng) {

	std::uniform_int_distribution<int> tile_dist(0, NUM_OBSTACLES);

	// Create a list of coordinates of every empty tile on the board,
	// along with a candidate tile to place there. Although this
	// consumes more entropy, I'm doing it like this because it can
	// be reused for the grow_board function.

	typedef std::pair<coord, tile> coord_and_tile;

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