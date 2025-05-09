#include "interval.hpp"

#include <limits>

const Interval Interval::empty = Interval(+infinity, -infinity);
const Interval Interval::universe = Interval(-infinity, +infinity);

Interval::Interval() : min(+infinity), max(-infinity) {} // Default Interval is empty

Interval::Interval(double min, double max) : min(min), max(max) {}

Interval::Interval(const Interval& a, const Interval& b) {
	// Create the Interval tightly enclosing the two input Intervals.
	min = a.min <= b.min ? a.min : b.min;
	max = a.max >= b.max ? a.max : b.max;
}

double Interval::size() const {
	return max - min;
}

bool Interval::contains(double x) const {
	return min <= x && x <= max;
}

bool Interval::surrounds(double x) const {
	return min < x && x < max;
}

double Interval::clamp(double x) const {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

Interval Interval::expand(double delta) const {
	auto padding = delta / 2;
	return Interval(min - padding, max + padding);
}

Interval operator+(const Interval& ival, double displacement) {
	return Interval(ival.min + displacement, ival.max + displacement);
}

Interval operator+(double displacement, const Interval& ival) {
	return ival + displacement;
}
