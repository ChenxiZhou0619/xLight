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
#include "frame.h"
#include <memory>
#include "common.h"
template<typename T>
struct TRay3 {
public:

    TRay3() = default;

    TRay3(const TPoint3<T> &_ori, const TVector3<T> &_dir, T _time = .0f, T _tmin = EPSILON, T _tmax = FLOATMAX)
        : ori(_ori), dir(normalize(_dir)), time(_time), tmin(_tmin), tmax(_tmax) { };

    TRay3(const TPoint3<T> &_ori, const TPoint3<T> &_end, T _time = .0f) : ori(_ori), time(_time) {
        dir = Vector3f{_end - _ori};
        tmin = EPSILON;
        tmax = dir.length() - EPSILON;
        dir = normalize(dir);        
    }
    
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
class Frame;

using Vector3f = TVector3<float>;
using Vector2f = TVector2<float>;
using Point3f  = TPoint3<float>;
using Point2f = TPoint2<float>;

struct RayIntersectionRec {
    bool isValid;
    
    // the distance bewteen ray ori and hit point
    float t;

    // the hit point
    Point3f p;

    // the normal at hit point
    // geometryNormal and shadingNormal respectively
    Normal3f geoN, shdN;

    // the two local frame using normals as Y axis
    Frame geoFrame, shdFrame;

    // the uv coordinate
    Point2f UV;

    // the hitted object
    Mesh* meshPtr;

    RayIntersectionRec() : isValid(false), t(FLOATMAX), meshPtr(nullptr) { }

    Vector3f toWorld(const Vector3f &local) const {
        return shdFrame.toWorld(local);
    }

    Vector3f toLocal(const Vector3f &world) const {
        return shdFrame.toLocal(world);
    }
};