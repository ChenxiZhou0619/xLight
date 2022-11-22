#pragma once
#include <assimp/scene.h>
#include <tinyformat/tinyformat.h>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>

#include "core/geometry/geometry.h"

template <typename T>
struct TMat4 {
  Eigen::Matrix4<T> mat;

  static TMat4<T> Identity() { return TMat4<T>{Eigen::Matrix4f::Identity()}; }

  static TMat4<T> Translate(const Vector3f &_v) {
    TMat4<T> res{Eigen::Matrix4f::Identity()};
    res(0, 3) = _v.x;
    res(1, 3) = _v.y;
    res(2, 3) = _v.z;
    return res;
  }

  static TMat4<T> Scale(const Vector3f &_v) {
    TMat4<T> res{Eigen::Matrix4f::Identity()};
    res(0, 0) = _v.x;
    res(1, 1) = _v.y;
    res(2, 2) = _v.z;
    return res;
  }

  static TMat4<T> Perspective(float vertFov, float aspect, float near,
                              float far) {
    Eigen::Matrix4f pers;
    pers << 1.f, .0f, .0f, .0f, .0f, 1.f, .0f, .0f, .0f, .0f,
        far / (far - near), -far * near / (far - near), .0f, .0f, 1.f, .0f;

    float recipY = 1.f / std::tan(vertFov * .5f * M_PI / 180.f);
    Eigen::Matrix4f yScale;
    yScale << recipY, .0f, .0f, .0f, .0f, recipY, .0f, .0f, .0f, .0f, 1.f, .0f,
        .0f, .0f, .0f, 1.f;

    Eigen::Matrix4f aspectAdjust;
    aspectAdjust << 1.f / aspect, .0f, .0f, .0f, .0f, 1.f, .0f, .0f, .0f, .0f,
        1.f, .0f, .0f, .0f, .0f, 1.f;
    return TMat4<T>{aspectAdjust * yScale * pers};
  }

  TMat4() : mat(Eigen::Matrix4<T>::Zero()) {}

  TMat4(const Eigen::Matrix4<T> &_mat) : mat(_mat) {}

  TMat4(const aiMatrix4x4 &_mat) {
    mat << _mat.a1, _mat.a2, _mat.a3, _mat.a4, _mat.b1, _mat.b2, _mat.b3,
        _mat.b4, _mat.c1, _mat.c2, _mat.c3, _mat.c4, _mat.d1, _mat.d2, _mat.d3,
        _mat.d4;
  }

  TMat4<T> operator*(const TMat4<T> &m) const { return TMat4{mat * m.mat}; }

  Point3f operator*(const Point3f &p) const {
    Eigen::Vector4f v{p.x, p.y, p.z, 1.f};
    Eigen::Vector4f res = mat * v;
    return Point3f{res[0] / res[3], res[1] / res[3], res[2] / res[3]};
  }

  TMat4<T> &operator*=(const TMat4<T> &m) {
    mat *= m.mat;
    return *this;
  }

  T operator()(int i, int j) const { return mat(i, j); }

  T &operator()(int i, int j) { return mat(i, j); }

  TMat4<T> inverse() const { return TMat4<T>{mat.inverse()}; }

  Vector3f rotate(const Vector3f &_v) const {
    Eigen::Vector4f v{_v.x, _v.y, _v.z, .0f};
    Eigen::Vector4f res = mat * v;
    return Vector3f{res[0], res[1], res[2]};
  }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const TMat4<T> &m) {
  os << "Mat4: \n"
     << tfm::format("%.3f %.3f %.3f %.3f\n", m.mat(0, 0), m.mat(0, 1),
                    m.mat(0, 2), m.mat(0, 3))
     << tfm::format("%.3f %.3f %.3f %.3f\n", m.mat(1, 0), m.mat(1, 1),
                    m.mat(1, 2), m.mat(1, 3))
     << tfm::format("%.3f %.3f %.3f %.3f\n", m.mat(2, 0), m.mat(2, 1),
                    m.mat(2, 2), m.mat(2, 3))
     << tfm::format("%.3f %.3f %.3f %.3f\n", m.mat(3, 0), m.mat(3, 1),
                    m.mat(3, 2), m.mat(3, 3));
  return os;
}