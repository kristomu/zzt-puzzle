#include "linux-reconstruction-of-zzt/csrc/world.h"
#include "linux-reconstruction-of-zzt/csrc/board.h"

#include "linux-reconstruction-of-zzt/csrc/elements/info.h"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <omp.h>

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
	world.currentBoard.create(false); // remove yellow border
	world.currentBoard.Name = "Puzzle board 0";

	// Set up everything solver-related.

	dfs_solver dfs;

	std::vector<size_t> board_indices = {0, 9, 99, 999, 9846, 9999, 10000, 96478, 98630, 98694,
		99971, 99998, 99999, 100000, 100016, 185055, 196122, 287599, 316107, 404718, 425103,
		467615, 502319, 514655, 517134, 532535, 556749, 582351, 640143, 683787, 687151, 753135,
		773693, 816699, 835739, 872379, 892607, 903087, 955229, 962974, 987535, 988007, 988918,
		989210, 989778, 999385, 999887, 999903, 999919, 999935, 999951, 999967, 999979, 999981,
		999982, 999983, 999987, 999991, 999995, 999996, 999997, 999998, 999999, 1000000,
		1000001, 1000002, 1000003, 1000004, 1000005, 1000006, 1000007, 1000008, 1000009,
		1000010, 1000011, 1000012, 1000013, 1000014, 1000016, 1000017, 1000018, 1000019,
		1000020, 1000024, 1000028, 1000032, 1000035, 1000048, 1000064, 1000065, 1000080,
		1000096, 1000098, 1000099, 1000128, 1000144, 1000160, 1000176, 1000208, 1000272,
		1000320, 1000336, 1000416, 1000496, 1058095, 1069151, 1098255, 1128910, 1200923,
		1221807, 1238475, 1305215, 1397631, 1473471, 1499679, 1508111, 1632399, 1689758,
		1741775, 1832971, 1836407, 1985371, 2005885, 2054911, 2330779, 2503039, 2533967,
		2620447, 2733375, 2844639, 2862747, 2865519, 3117851, 3148587, 3327007, 3596175,
		3717823, 3779711, 3832575, 3918575, 3942667, 3950399, 4136063};

	// Where the writer starts placing a puzzle.
	// Note that (1,1) is the very upper left - ZZT coordinates are
	// 1-based due to the edge that surrounds every board.
	coord puzzles_upper_left(2, 3);

	// TODO: Parallelize board generation.
	std::map<int, zzt_board> constructed_boards;

	#pragma omp parallel for private(dfs) schedule(monotonic:dynamic)
	for (size_t board_to_generate: board_indices) {

		coord max(4 + board_to_generate % 4, 4 + (board_to_generate/4) % 4);

		// Also TODO? mark the end square.
		coord player_pos(0, 3);
		coord end_square(max.x-1, max.y-1);

		zzt_board output_board = grow_indexed_board(player_pos, end_square,
			max, 30, dfs, board_to_generate);

		#pragma omp critical
		{
			std::cout << "Including " << board_to_generate << std::endl;
			output_board.print();
			constructed_boards[board_to_generate] = output_board;
		}
	}

	for (size_t board_to_generate: board_indices) {

		coord max(4 + board_to_generate % 4, 4 + (board_to_generate/4) % 4);

		// Also TODO? mark the end square.
		coord player_pos(0, 3);
		coord end_square(max.x-1, max.y-1);

		zzt_board output_board = constructed_boards[board_to_generate];

		convert_puzzle_board(board_to_generate, output_board, world.currentBoard,
			puzzles_upper_left, LightRed, true);

		// Move the upper left point so that it doesn't overlap the last puzzle.
		// The itos part is in case the number specifying the puzzle protrudes
		// to the right of the puzzle itself.
		puzzles_upper_left.x += std::max((size_t)max.x + 2,
			itos_puz(board_to_generate).size()) + 2;

		if (puzzles_upper_left.x + 8 >= 60) {
			if (puzzles_upper_left.y + 8 >= 21) {
				// TODO: There really should be a way to access different
				// boards in world.cc; the more painless the better. Until
				// then, I have to do it this ugly way.
				world.BoardData[world.BoardCount] = world.currentBoard.dump_and_truncate();
				world.BoardCount++;
				world.Info.CurrentBoardIdx++; // needed because save() closes the current board.
				world.currentBoard.create(false);
				world.currentBoard.Name = "Puzzle board " + itos_puz(world.BoardCount);
				puzzles_upper_left = coord(2, 3);
			} else {
				puzzles_upper_left.x = 2;
				puzzles_upper_left.y += 12;
			}
		}
	}

	std::ofstream out("XPUZZLE.ZZT");
	//out << world;		// Doesn't work: TODO, fix

	world.save(out, true);
	out.close();
}