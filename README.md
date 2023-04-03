# zzt-puzzle
This is a ZZT puzzle generator. More information to come.

## Computational complexity
ZZT slider puzzles are actually NP-hard, even if restricted to boulders alone.[^Hoffman] With solids ("blocks tied to the board"), they are PSPACE-complete. [^Bremner]

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

## Things I've tried that didn't have much of an effect:

- IDDFS transposition table/move ordering guidance: minor if any effect (less
than 0.5% runtime); not worth the increase in complexity. Strangely, moving the
move suggested by the shallower search to the *end* of the move order improved
runtime. In any case, I didn't expect much of a change as most of our time is
spent in refutation (proving a board to be unsolvable).
- Retaining the transposition tables between runs in IDDFS: strangely enough
this leads to a significant slowdown, not a speedup as one would expect.

[^Hoffman]: HOFFMANN, Michael. Motion planning amidst movable square blocks: Push-* is NP-hard. In: Canadian Conference on Computational Geometry. 2000. p. 205-210.
[^Bremner]: BREMNER, David; O’ROURKE, Joseph; SHERMER, Thomas. Motion planning amidst movable square blocks is PSPACE complete. Draft, June, 1994, 28.
