#include <iostream>
#include "core/math/discretepdf.h"
#include "core/geometry/geometry.h"

int main(int argc, char **argv) {
    Distribution2D dist (3, 4);

    int k = 1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            dist.appendAtX(i, k++);
        }
    }
    float t = dist.normalize();
    float pdf;
    Vector2i i = dist.sample(Point2f {0.1834f, 0.9855f}, &pdf);
    std::cout << pdf << std::endl;
    std::cout << dist.pdf(i) << std::endl;

}