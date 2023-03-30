#pragma once

#include "coord.h"

#include <iostream>
#include <vector>

enum tile				{T_EMPTY, T_PLAYER, T_SOLID,
						 T_SLIDEREW, T_SLIDERNS, T_BOULDER};

const tile tiles[] =	{T_EMPTY, T_PLAYER, T_SOLID,
						 T_SLIDEREW, T_SLIDERNS, T_BOULDER};
const tile obstacles[] =				  { T_SOLID,
						 T_SLIDEREW, T_SLIDERNS, T_BOULDER};

const int NUM_TILE_TYPES = 6;
const int NUM_OBSTACLES = 4;

// This one takes a bit of explaining. When the player moves,
// he may push a number of tiles in the direction he's moving.
// These tiles are then all shifted up one to the first empty
// space (if any; otherwise the move is impossible). Because
// copying the structure each time we descend in minmax/alpha-beta
// would be too slow, we need to be able to undo the move, which
// is exactly the same thing as pushing the whole stack, player
// included, one step in the opposite direction.
// Thus we need to know where to start pushing, and what direction
// the player moved when causing the push; this is what that's for.

struct push_info {
	coord last_push_destination;
	coord player_direction;
};

class zzt_board {
	private:
		coord size;

		// Only access this with set() and get() and in the
		// constructor!
		std::vector<std::vector<tile> > board_p;

		// Values for Zobrist hashing.
		std::vector<std::vector<std::vector<uint64_t> > > zobrist_values;
		uint64_t hash;
		void generate_zobrist();

		bool pushable(const coord & pos, const coord & delta) const;

		// out_terminus is the last position we push something onto.
		// See the comment below about last_push_destination.
		bool push(const coord & current_pos, const coord & delta,
			coord & out_terminus);
		bool push(coord & current_pos, direction dir,
			coord & out_terminus) {

			return push(current_pos, get_delta(dir), out_terminus);
		}


	public:
		coord player_pos;

		coord get_size() const { return size; }
		uint64_t get_hash() const { return hash; }

		// Pretend that the playing field is surrounded by
		// infinitely many solids.
		tile get_tile_at(const coord & where) const {
			if (where.x < 0 || where.y < 0) {
				return T_SOLID;
			}
			if (where.x >= size.x || where.y >= size.y) {
				return T_SOLID;
			}

			return board_p[where.y][where.x];
		}

		void set(const coord & where, tile what) {
			if (where.x < 0 || where.y < 0 ||
				where.x >= size.x || where.y >= size.y) {
				throw std::runtime_error("Set: Tried to set outside board!");
			}
			// Unhash the current tile at this position, set the new
			// tile, and hash it.
			hash ^= zobrist_values[where.y][where.x][
				board_p[where.y][where.x]];
			board_p[where.y][where.x] = what;

			hash ^= zobrist_values[where.y][where.x][(int)what];
		}

		void swap(const coord & a, const coord & b) {
			tile backup = get_tile_at(a);
			set(a, get_tile_at(b));
			set(b, backup);
		}

		zzt_board(coord player_pos_in, coord board_size) {
			size = board_size;
			generate_zobrist();

			board_p = std::vector<std::vector<tile > >(board_size.y,
				std::vector<tile>(board_size.x, T_EMPTY));
			hash = 0;
			// Hash in the Zobrist values of all the empties.
			for (int y = 0; y < size.y; ++y) {
				for (int x = 0; x < size.x; ++x) {
					hash ^= zobrist_values[y][x][(int)T_EMPTY];
				}
			}

			player_pos = player_pos_in;
			set(player_pos, T_PLAYER);
		}


		std::vector<push_info> push_log;
		std::vector<push_info>::const_iterator push_log_pos;

		// Move the player in the direction given
		bool do_move(direction dir);

		// Move the player in the opposite direction
		void undo_move();

		void print() const;

		bool operator==(const zzt_board & other) {
			if (other.get_size() != get_size()) { return false; }
			if (hash != other.hash) { return false; }

			coord pos;
			for (pos.y = 0; pos.y < get_size().y; ++pos.y) {
				for (pos.x = 0; pos.x < get_size().x; ++pos.x) {
					if(get_tile_at(pos) != other.get_tile_at(pos)) {
						return false;
					}
				}
			}

			return true;
		}

		bool operator!=(const zzt_board & other) {
			return !(*this == other);
		}
};