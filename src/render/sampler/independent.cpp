#include "independent.h"

float Independent::next1D() {
    return dist(gen);
}

Point2f Independent::next2D() {
    return Point2f(next1D(), next1D());
}