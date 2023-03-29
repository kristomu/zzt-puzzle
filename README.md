# zzt-puzzle
This is a ZZT puzzle generator. More information to come.

## Computational complexity
ZZT slider puzzles are actually NP-hard, even if restricted to boulders alone.
With keys and doors (not implemented here), they might even be PSPACE-complete,
though I haven't proven this.

## Strategy
The puzzle generator works by creating a random board and checking if it's
solvable using min-max search with transposition tables. If it's not, then
the puzzle is just discarded, otherwise it's shown to the user.

There's also a "grow board" strategy which starts with a mostly empty board
and fills it until it can't be solved anymore, based on the observation that
a puzzle can't go from being unsolvable to solvable by adding more obstacles
(sliders, boulders, or solids).

However, given that slider puzzles are NP-hard, there might be a phase
transition and so this approach might not be the best. I suspect the phase
transition is around where half the board is being used.
