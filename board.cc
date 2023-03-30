#include "board.h"

void zzt_board::generate_zobrist() {
	// The vector contains one value for each possible tile at each
	// possible location. TODO later? Use a custom random function
	// with a fixed seed to make this completely deterministic and
	// debuggable.
	zobrist_values = std::vector<std::vector<std::vector<uint64_t> > >(
		size.y, std::vector<std::vector<uint64_t> >(size.x,
		std::vector<uint64_t>(NUM_TILE_TYPES, 0)));

	for (auto & row: zobrist_values) {
		for (auto & column: row) {
			for (uint64_t & cell: column) {
				cell = (random() << 32LL) + random();
			}
		}
	}
}

bool zzt_board::pushable(const coord & pos, const coord & delta) const {
	tile at_pos = get_tile_at(pos);

	switch(at_pos) {
		case T_EMPTY: return false;
		case T_PLAYER: return true;
		case T_SOLID: return false;
		case T_SLIDEREW: return delta.y == 0; // can't be moved vertically
		case T_SLIDERNS: return delta.x == 0; // can't be moved horizontally
		case T_BOULDER: return true;
		default: throw std::runtime_error("Pushable: unknown tile type!");
	}
}

bool zzt_board::push(const coord & current_pos, const coord & delta,
	coord & out_terminus) {
	// Check if the current tile is pushable.
	if (!pushable(current_pos, delta)) {
		 return false;
	}

	// Check if the destination tile is an empty: if so, we just have
	// to move the current tile over there.
	coord dest_tile = current_pos + delta;
	if (get_tile_at(dest_tile) == T_EMPTY) {
		swap(dest_tile, current_pos);
		out_terminus = dest_tile;
		return true;
	}

	// Check if the destination tile is pushable. If so, recurse.
	// If we get a true result, then there will be an empty at the
	// destination tile and we can move the source tile over there.
	if (!pushable(dest_tile, delta)) {
		return false;
	}

	if (push(dest_tile, delta, out_terminus)) {
		swap(dest_tile, current_pos);
		return true;
	} else {
		return false;
	}
}

bool zzt_board::do_move(direction dir) {
	// Try to push in the direction given. If it works, update the
	// player position and set the terminus and direction info.
	// Otherwise just return false.

	coord out_terminus;
	if (push(player_pos, dir, out_terminus)) {
		player_pos += get_delta(dir);

		push_info this_push;
		this_push.last_push_destination = out_terminus;
		this_push.player_direction = get_delta(dir);
		push_log.push_back(this_push);

		return true;
	} else {
		return false;
	}
}

void zzt_board::undo_move() {
	if (push_log.empty()) {
		throw std::runtime_error("Tried to undo before any moves were made");
	}

	// First move the player in the opposite direction, leaving
	// a gap on the player side of the chain of pushables that
	// we're undoing.

	// Then push from the terminus in the opposite direction of
	// the player's earlier movement, which will reset every tile
	// between the player and the terminus.

	// This will throw an exception if anything strange happens.
	coord last_move_dir_delta = push_log.rbegin()->player_direction;
	coord opposite_delta = coord(0, 0) - last_move_dir_delta;
	coord player_new_pos = player_pos + opposite_delta;

	if (get_tile_at(player_new_pos) != T_EMPTY) {
		throw std::logic_error("Trying to undo move but player can't retrace his steps!");
	}
	if (get_tile_at(player_pos) != T_PLAYER) {
		throw std::logic_error("Player pos is not correct!");
	}

	set(player_pos, T_EMPTY);
	set(player_new_pos, T_PLAYER);

	coord throwaway, chain_end = push_log.rbegin()->last_push_destination;

	// Only push the rest of the chain if there's anything to it.
	// If it's just the player, then skip.
	if (chain_end != player_pos) {
		if (!push(chain_end, opposite_delta, throwaway)) {
			throw std::logic_error("Can't push tiles back into original place!");
		}
	}

	player_pos = player_new_pos;

	push_log.pop_back();
}

void zzt_board::print() const {
	coord pos;
	for (pos.y = 0; pos.y < size.y; ++pos.y) {
		for (pos.x = 0; pos.x < size.x; ++pos.x) {
			switch(get_tile_at(pos)) {
				case T_EMPTY: std::cout << "."; break;
				case T_SOLID: std::cout << "#"; break;
				case T_PLAYER: std::cout << "@"; break;
				case T_SLIDEREW: std::cout << ">"; break;
				case T_SLIDERNS: std::cout << "^"; break;
				case T_BOULDER: std::cout << "x"; break;
				default: std::cout << "?"; break;
			}
		}
		std::cout << std::endl;
	}
}