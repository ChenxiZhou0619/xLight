#pragma once
#include <eigen3/Eigen/Core>

template<typename T>
struct TMat4 {
    Eigen::Matrix4<T> mat;

    TMat4(): mat(Eigen::Matrix4<T>::Zero()) { }
    
    TMat4(const Eigen::Matrix4<T> &_mat) : mat(_mat) { }

    TMat4<T> operator*(const TMat4<T> &m) const {
        return TMat4 { mat * m.mat };
    }

    TMat4<T>& operator*=(const TMat4<T> &m) {
        mat *= m.mat;
        return *this;
    }

    T operator()(int i, int j) const {
        return mat(i, j);
    }

    T& operator()(int i, int j) {
        return mat(i, j);
    }

    Vector3f rotate(const Vector3f &_v) const {
        Eigen::Vector4f v{_v.x, _v.y, _v.z, .0f};
        Eigen::Vector4f res = mat * v;
        return Vector3f {
            res[0],
            res[1],
            res[2]
        };
    }

};

template<typename T>
std::ostream& operator<<(std::ostream &os, const TMat4<T> &m) {
    os << "Mat4: \n"
       << m.mat;
    return os;
}