#include "linux-reconstruction-of-zzt/csrc/world.h"
#include "linux-reconstruction-of-zzt/csrc/board.h"

#include "linux-reconstruction-of-zzt/csrc/elements/info.h"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

#include "generator.h"
#include "solver/all.h"

// Integer to string. The silly name is because there's
// already an itos in linux-reconstruction.

std::string itos_puz(int source) {
	std::ostringstream q;
	q << source;
	return (q.str());
}

void convert_puzzle_board(size_t puzzle_number,
	const zzt_board & puzzle, TBoard & out_board,
	coord out_upper_left, char color, bool border) {

	coord pos, offset_pos;

	if (border) {
		for (pos.y = 0; pos.y < puzzle.get_size().y+2; ++pos.y) {
			for (pos.x = 0; pos.x < puzzle.get_size().x+2; ++pos.x) {

				// We only want to paint the borders.
				if (pos.x != 0 && pos.x != puzzle.get_size().x+1 &&
					pos.y != 0 && pos.y != puzzle.get_size().y+1) {
					continue;
				}

				offset_pos = pos + out_upper_left;
				out_board.Tiles[offset_pos.x][offset_pos.y].Element = E_SOLID;
				out_board.Tiles[offset_pos.x][offset_pos.y].Color = LightCyan;
			}

			// Append text showing the puzzle number.
			out_board.add_string(out_upper_left.x, out_upper_left.y,
				itos_puz(puzzle_number), Cyan);
		}
		// Now recurse by placing the puzzle itself inside the
		// border we just created.
		convert_puzzle_board(puzzle_number, puzzle, out_board,
			out_upper_left + coord(1, 1), color, false);
		return;
	}

	for (pos.y = 0; pos.y < puzzle.get_size().y; ++pos.y) {
		for (pos.x = 0; pos.x < puzzle.get_size().x; ++pos.x) {
			offset_pos = pos + out_upper_left;
			int zzt_element;
			switch(puzzle.get_tile_at(pos)) {
				case T_EMPTY: zzt_element = E_EMPTY; break;
				case T_PLAYER: zzt_element = E_FAKE; break; // not yet, requires stats
				case T_SOLID: zzt_element = E_SOLID; break;
				case T_SLIDEREW: zzt_element = E_SLIDER_EW; break;
				case T_SLIDERNS: zzt_element = E_SLIDER_NS; break;
				case T_BOULDER: zzt_element = E_BOULDER; break;
				default: throw std::logic_error("unknown tile");
			}
			out_board.Tiles[offset_pos.x][offset_pos.y].Element = zzt_element;
			out_board.Tiles[offset_pos.x][offset_pos.y].Color = color;
		}
	}
}

int main() {

	// Set up a world to write to.

	std::shared_ptr<ElementInfo> element_info_ptr = std::make_shared<ElementInfo>();
	TWorld world(element_info_ptr);

	world.Info.Name = "XPUZZLE";
	world.BoardCount = 0; // inclusive
	world.currentBoard.Name = "Puzzle board";

	// Set up everything solver-related.

	dfs_solver dfs;

	std::vector<size_t> board_indices = {220788, 903087, 196122, 2005885, 287599,
		903087, 2432415, 96478, 2862747, 1971791, 566543};

	// Where the writer starts placing a puzzle.
	// Note that (1,1) is the very upper left - ZZT coordinates are
	// 1-based due to the edge that surrounds every board.
	coord puzzles_upper_left(2, 3);

	// TODO: Parallelize board generation.
	for (size_t board_to_generate: board_indices) {

		coord max(4 + board_to_generate % 4, 4 + (board_to_generate/4) % 4);

		// Also TODO? mark the end square.
		coord player_pos(0, 3);
		coord end_square(max.x-1, max.y-1);

		zzt_board output_board = grow_indexed_board(player_pos, end_square,
			max, 30, dfs, board_to_generate);

		std::cout << "Going to export " << board_to_generate << std::endl;
		output_board.print();

		convert_puzzle_board(board_to_generate, output_board, world.currentBoard,
			puzzles_upper_left, LightRed, true);

		// Move the upper left point so that it doesn't overlap the last puzzle.
		// The itos part is in case the number specifying the puzzle protrudes
		// to the right of the puzzle itself.
		puzzles_upper_left.x += std::max((size_t)max.x + 2,
			itos_puz(board_to_generate).size()) + 2;

		if (puzzles_upper_left.x + 8 >= 60) {
			puzzles_upper_left.x = 3;
			puzzles_upper_left.y += 12;
		}
	}

	std::ofstream out("XPUZZLE.ZZT");
	//out << world;		// Doesn't work: TODO, fix

	world.save(out, true);
	out.close();
}