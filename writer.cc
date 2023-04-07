#include "linux-reconstruction-of-zzt/csrc/world.h"
#include "linux-reconstruction-of-zzt/csrc/board.h"

#include <iostream>
#include <fstream>

int main() {
	TWorld world;

	world.Info.Name = "XPUZZLE";
	world.BoardCount = 1;

	// This should be in a constructor.
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

	std::ofstream out("XPUZZLE.ZZT");
	//out << world;		// Doesn't work: TODO, fix

	world.save(out, true);
	out.close();
}