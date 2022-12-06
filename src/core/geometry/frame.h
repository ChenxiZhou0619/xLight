#pragma once

#include "common.h"
#include "normal.h"
#include "point.h"
#include "vector.h"

using Vector3f = TVector3<float>;

inline void Coordinate(const Vector3f &n, Vector3f *v1, Vector3f *v2) {
  if (std::abs(n.x) > std::abs(n.y)) {
    *v1 = Vector3f(-n.z, 0, n.x) / std::sqrt(n.z * n.z + n.x * n.x);
  } else {
    *v1 = Vector3f(0, n.z, -n.y) / std::sqrt(n.z * n.z + n.y + n.y);
  }
  *v2 = cross(n, *v1);
}

class Frame {
  Vector3f _x, _y, _z;

public:
  Frame() = default;

  explicit Frame(const Normal3f &normal) {
    Vector3f tmp{0, 1, 0};
    if (std::abs(dot(normal, tmp)) > 1 - EPSILON) {
      tmp = Vector3f{0, 0, 1};
    }
    _x = normalize(cross(normal, tmp));
    _y = Vector3f{normal};
    _z = cross(_x, _y);
  }

  Frame(Normal3f N, Vector3f T, Vector3f B) {
    _x = T;
    _y = Vector3f{N};
    _z = B;
  }

  static float cosTheta(const Vector3f &localV) { return localV[1]; }

  static float tanTheta(const Vector3f &localV) {
    float temp = 1.f - localV.y * localV.y;
    if (temp <= .0f)
      return .0f;
    return std::sqrt(temp) / localV.y;
  }

  Vector3f toLocal(const Vector3f &world) const {
    return Vector3f{dot(world, _x), dot(world, _y), dot(world, _z)};
  }

  Vector3f toWorld(const Vector3f &local) const {
    return local.x * _x + local.y * _y + local.z * _z;
  }

  friend std::ostream &operator<<(std::ostream &os, const Frame &rhs);
};
inline std::ostream &operator<<(std::ostream &os, const Frame &rhs) {
  std::cout << "Frame is:\n"
            << "\t" << rhs._x << "\n"
            << "\t" << rhs._y << "\n"
            << "\t" << rhs._z;
  return os;
}
