#pragma once

#include <limits>

const double infinity = std::numeric_limits<double>::infinity();

class Interval {
public:
    double min, max;

    Interval();

    Interval(double min, double max);

    Interval(const Interval& a, const Interval& b);

    double size() const;

    bool contains(double x) const;

    bool surrounds(double x) const;

    double clamp(double x) const;

    Interval expand(double delta) const;

    static const Interval empty, universe;
};


Interval operator+(const Interval& ival, double displacement);

Interval operator+(double displacement, const Interval& ival);