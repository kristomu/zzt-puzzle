#pragma once

#include "board.h"

// Puzzle generators. For now this only includes the random fill
// generator (create a random board at some sparsity) as the
// "grow_board" generator depends on the minmax search, which
// would produce annoying dependencies among headers. Fix later.

void fill_puzzle(zzt_board & board, double sparsity);

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size);