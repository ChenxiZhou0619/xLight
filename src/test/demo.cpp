#include <iostream>
#include "core/math/mat4.h"
#include "core/math/math.h"

int main() {
    Point3f p {-.17778f, .1f, .1f};
    
    Mat4f perspective = Mat4f::Perspective(
        90, 
        1.7778f, 
        .1f, 
        std::numeric_limits<float>::max()
    );

    perspective = 
    Mat4f::Scale(Vector3f {.5f, -.5f, 1.f})
    * Mat4f::Translate(Vector3f {1.f, -1.f, .0f})
    * perspective;

    Mat4f sampleToFilm = perspective.inverse();
    
    
    std::cout << sampleToFilm * Point3f {0.f, 0.f, 0.f} << std::endl;
}