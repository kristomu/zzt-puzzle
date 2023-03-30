#include "generator.h"

// Fill some empty tiles with non-empty tiles: for each
// empty tile, leave it alone with probability sparsity,
// or fill it with probability 1-sparsity.
void fill_puzzle(zzt_board & board, double sparsity) {

	coord pos;
	for (pos.y = 0; pos.y < board.get_size().y; ++pos.y) {
		for (pos.x = 0; pos.x < board.get_size().x; ++pos.x) {
			if(board.get_tile_at(pos) != T_EMPTY) {
				continue;
			}
			if (drand48() < sparsity) { continue; }

			switch(1 + random()%4) {
				default:
				case 0: board.set(pos, T_EMPTY); break;
				// TODO: enable this ???
				case 1: //board.set(pos, T_SOLID); break;
				case 2: board.set(pos, T_SLIDEREW); break;
				case 3: board.set(pos, T_SLIDERNS); break;
				case 4: board.set(pos, T_BOULDER); break;
			}
		}
	}
}

zzt_board create_random_puzzle(double sparsity,
	coord player_pos, coord max_size) {

	zzt_board out_board(player_pos, max_size);

	fill_puzzle(out_board, sparsity);
	return out_board;
}