#pragma once

#include "solver/solver.h"
#include "board.h"

#include "random/random.h"

// Puzzle generators. For now this only includes the random fill
// generator (create a random board at some sparsity) as the
// "grow_board" generator depends on the minmax search, which
// would produce annoying dependencies among headers. Fix later.

void fill_puzzle(zzt_board & board, size_t tiles_to_fill,
	rng & rng_to_use);

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size,
	rng & rng_to_use);

zzt_board create_indexed_puzzle(double sparsity,
	coord player_pos, coord max_size,
	uint64_t index);

zzt_board grow_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	rng & rng_to_use, int min_skips, int max_skips);

zzt_board grow_indexed_board(coord player_pos, coord end_square,
	coord size, int recursion_level, solver & guiding_solver,
	uint64_t index);