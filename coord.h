#pragma once

#include <stdexcept>

// A simple 2D coordinate class used to handle tile positions.

class coord {
	public:
		int x;
		int y;

		coord() { x = 0; y = 0; }
		coord(int x_in, int y_in) { x = x_in; y = y_in; }

		// boilerplate

		coord operator+(const coord & rhs) const {
			return coord(x + rhs.x, y + rhs.y);
		}

		coord operator-(const coord & rhs) const {
			return coord(x - rhs.x, y - rhs.y);
		}

		coord operator+=(const coord & rhs) {
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		coord operator-=(const coord & rhs) {
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		bool operator==(const coord & other) {
			return x == other.x && y == other.y;
		}

		bool operator!=(const coord & other) {
			return !(*this == other);
		}
};

// And some helper routines for translating from directions to
// relative coordinates.

enum direction {NORTH, SOUTH, EAST, WEST, IDLE};

coord get_delta(direction dir);