#include "coord.h"

coord get_delta(direction dir) {
	switch(dir) {
		case NORTH: return coord(0, -1);
		case SOUTH: return coord(0, 1);
		case EAST: return coord(1, 0);
		case WEST: return coord(-1, 0);
		default: throw std::logic_error("get_delta: Invalid direction!");
	}
}