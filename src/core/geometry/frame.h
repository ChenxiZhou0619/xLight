#pragma once

#include "normal.h"
#include "vector.h"
#include "point.h"
#include "common.h"

using Vector3f = TVector3<float>;

class Frame {
    Vector3f _x, _y, _z;

public:

    Frame() = default;

    Frame(const Normal3f &normal) {
        Vector3f tmp {0, 1, 0};
        if (dot(normal, tmp) < EPSILON) {
            tmp = Vector3f {0, 0, 1};
        }
        _x = normalize(cross(normal, tmp));
        _y = Vector3f{normal};
        _z = cross(_x, _y);
    }

    static float cosTheta(const Vector3f &localV) {
        return localV[1];
    }

    static float tanTheta(const Vector3f &localV) {
        float temp = 1.f - localV.y * localV.y;
        if(temp <= .0f ) return .0f;
        return std::sqrt(temp) / localV.y;
    }

    Vector3f toLocal(const Vector3f &world) const {
        return Vector3f {
            dot(world, _x),
            dot(world, _y),
            dot(world, _z)
        };
    } 

    Vector3f toWorld(const Vector3f &local) const {
        return local.x * _x + local.y * _y + local.z * _z;
    }

};