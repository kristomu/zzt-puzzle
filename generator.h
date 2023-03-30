#pragma once

#include "board.h"
#include <random>

// Puzzle generators. For now this only includes the random fill
// generator (create a random board at some sparsity) as the
// "grow_board" generator depends on the minmax search, which
// would produce annoying dependencies among headers. Fix later.

void fill_puzzle(zzt_board & board, size_t tiles_to_fill,
	std::mt19937 & rng);

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size,
	std::mt19937 & rng);

zzt_board create_indexed_puzzle(double sparsity,
	coord player_pos, coord max_size,
	uint64_t index);