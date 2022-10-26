#pragma once
#include <limits>

constexpr float EPSILON = std::numeric_limits<float>::epsilon() * 10;
constexpr float FLOATMAX = std::numeric_limits<float>::max();
constexpr float FLOATMIN = std::numeric_limits<float>::lowest();
constexpr float FINF = std::numeric_limits<float>::infinity();
