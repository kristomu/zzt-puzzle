#include "linux-reconstruction-of-zzt/csrc/world.h"
#include "linux-reconstruction-of-zzt/csrc/board.h"

#include "linux-reconstruction-of-zzt/csrc/elements/info.h"

#include <stdexcept>
#include <iostream>
#include <fstream>

#include "generator.h"
#include "solver/all.h"

int main() {

	// Generate a board.
	int board_to_generate = 2220788;
	dfs_solver dfs;

	coord max(4 + board_to_generate % 4, 4 + (board_to_generate/4) % 4);

	coord player_pos(0, 3);
	coord end_square(max.x-1, max.y-1);

	zzt_board output_board = grow_indexed_board(player_pos, end_square,
		max, 30, dfs, board_to_generate);

	std::cout << "Going to export " << board_to_generate << std::endl;
	output_board.print();

	// Set up a world to write to.

	std::shared_ptr<ElementInfo> element_info_ptr = std::make_shared<ElementInfo>();
	TWorld world(element_info_ptr);

	world.Info.Name = "XPUZZLE";
	world.BoardCount = 0; // inclusive

	// This should be in a constructor.
	// We also need to set a player somehow... should be in Board's
	// constructor, perhaps. TODO.
	world.Info.Ammo = 0;
	world.Info.Gems = 0;
	world.Info.Health = 100;
	world.Info.CurrentBoardIdx = 0;
	world.Info.Torches = 0;
	world.Info.TorchTicks = 0;
	world.Info.EnergizerTicks = 0;
	world.Info.Score = 0;
	world.Info.BoardTimeSec = 0;
	world.Info.BoardTimeHsec = 0;
	world.Info.IsSave = false;

	world.currentBoard.Name = "Example board";

	coord pos(0, 0), output_ul(10, 10);
	for (pos.y = 0; pos.y < max.y; ++pos.y) {
		for (pos.x = 0; pos.x < max.x; ++pos.x) {
			coord output_pos = pos + output_ul;
			int zzt_element;
			switch(output_board.get_tile_at(pos)) {
				case T_EMPTY: zzt_element = E_EMPTY; break;
				case T_PLAYER: zzt_element = E_FAKE; break; // not yet, requires stats
				case T_SOLID: zzt_element = E_SOLID; break;
				case T_SLIDEREW: zzt_element = E_SLIDER_EW; break;
				case T_SLIDERNS: zzt_element = E_SLIDER_NS; break;
				case T_BOULDER: zzt_element = E_BOULDER; break;
				default: throw std::logic_error("unknown tile");
			}
			world.currentBoard.Tiles[output_pos.x][output_pos.y].Element = zzt_element;
			world.currentBoard.Tiles[output_pos.x][output_pos.y].Color = 0xF;
		}
	}

	std::ofstream out("XPUZZLE.ZZT");
	//out << world;		// Doesn't work: TODO, fix

	world.save(out, true);
	out.close();
}
