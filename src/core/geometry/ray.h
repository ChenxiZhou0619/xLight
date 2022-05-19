/**
 * @file ray.h
 * @author zcx
 * @brief 
 * @version 0.1
 * @date 2022-05-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "vector.h"
#include "point.h"
#include "common.h"

template<typename T>
struct TRay3 {
public:

    TRay3() = default;

    TRay3(const TPoint3<T> &_ori, const TVector3<T> &_dir, T _time = .0f, T _tmin = EPSILON, T _tmax = FLOATMAX)
        : ori(_ori), dir(normalize(_dir)), time(_time), tmin(_tmin), tmax(_tmax) { };

    TPoint3<T> at(T dist) const {
        return ori + dist * dir;
    }

    TPoint3<T> ori;
    TVector3<T> dir;
    T time, tmin, tmax;
};

template<typename T>
std::ostream& operator<<(std::ostream &os, const TRay3<T> &ray) {
    os << "Ray3:\n"
       << "\torigin: " << ray.ori << std::endl
       << "\tdir   : " << ray.dir << std::endl
       << "\ttmin = " << ray.tmin << ", tmax = " << ray.tmax;
    return os;
}

/**
 * @brief record the context at the hitting point
 * 
 */
class Mesh;

struct RayIntersectionRec {
    bool isValid;
    
    // the distance bewteen ray ori and hit point
    float t;

    // the hit point
    Point3f p;

    // the normal at hit point
    // geometryNormal and shadingNormal respectively
    Normal3f geoN, shdN;

    // the uv coordinate
    Vector2f UV;

    // the hitted object
    std::shared_ptr<Mesh> meshPtr;

    RayIntersectionRec() : isValid(false), t(FLOATMAX), meshPtr(nullptr) { }
};