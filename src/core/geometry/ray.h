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
#include <cmath>
#include "common.h"
#include "core/math/common.h"
#include <embree3/rtcore_ray.h>
//* fwd
class Medium;

template<typename T>
struct TRay3 {
public:

    TRay3() = default;

    TRay3(const TPoint3<T> &_ori, const TVector3<T> &_dir,
          std::shared_ptr<Medium> _medium = nullptr, 
          T _time = .0f, T _tmin = 0.0005, T _tmax = 1e10)
        : ori(_ori), dir(normalize(_dir)), time(_time), tmin(_tmin), tmax(_tmax), medium(_medium.get()) { };

    TRay3(const TPoint3<T> &_ori, const TPoint3<T> &_end, 
          std::shared_ptr<Medium> _medium = nullptr ,T _time = .0f) : ori(_ori), time(_time), medium(_medium.get()) 
    {
        dir = Vector3f{_end - _ori};
        tmin = 0.0005;
        tmax = dir.length() - EPSILON * 100;
        dir = normalize(dir);        
    }
    
    TPoint3<T> at(T dist) const {
        return ori + dist * dir;
    }

    TPoint3<T> ori;
    TVector3<T> dir;
    T time, tmin, tmax;
    const Medium *medium;

    //* data for ray differential
    bool is_ray_differential = false;   // set to true when camera generate ray-differential
    Vector3f direction_dx, direction_dy;

    RTCRay toRTC() const {
        RTCRay rtcRay;
        rtcRay.org_x = ori.x;
        rtcRay.org_y = ori.y;
        rtcRay.org_z = ori.z;

        rtcRay.dir_x = dir.x;
        rtcRay.dir_y = dir.y;
        rtcRay.dir_z = dir.z;

        rtcRay.tnear = tmin;
        rtcRay.tfar  = tmax;

        rtcRay.mask = -1;
        rtcRay.flags = 0;

        return rtcRay;
    }

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

    // tangent
    bool hasTangent = false;
    Vector3f tangent;

    // the two local frame using normals as Y axis
    Frame geoFrame, shdFrame;

    // the uv coordinate
    Point2f UV;

    // the hitted object
    Mesh* meshPtr;

    //* dpdu & dpdv
    bool can_diff = false;
    Vector3f dpdx, dpdy;
    Vector3f dpdu, dpdv;
    float dudx, dudy, dvdx, dvdy;

    RayIntersectionRec() : isValid(false), t(FLOATMAX), meshPtr(nullptr) { }

    Vector3f toWorld(const Vector3f &local) const {
        return shdFrame.toWorld(local);
    }

    Vector3f toLocal(const Vector3f &world) const {
        return shdFrame.toLocal(world);
    }

    void clear() {
        *this = RayIntersectionRec();
    }

    //TODO, just for perspective
    void computeRayDifferential(const TRay3<float> &ray) {
        if (ray.is_ray_differential && can_diff) {
            //* compute
            float d = dot(geoN, Vector3f(p.x, p.y, p.z));
            float tx = (d - dot(geoN, Vector3f(ray.ori.x, ray.ori.y, ray.ori.z)))
                       / dot(geoN, ray.direction_dx);
            assert(!std::isinf(tx) && !std::isnan(tx));
            float ty = (d - dot(geoN, Vector3f(ray.ori.x, ray.ori.y, ray.ori.z)))
                       / (dot(geoN, ray.direction_dy));
            assert(!std::isinf(ty) && !std::isnan(ty));
            Point3f px = ray.ori + ray.direction_dx * tx,
                    py = ray.ori + ray.direction_dy * ty;
            dpdx = px - p;
            dpdy = py - p;

            //* compute dudx, dudy, dvdx, dvdy
            int dim[2];
            Normal3f n = geoN;
            if (std::abs(n.x) > std::abs(n.y) && std::abs(n.x) > std::abs(n.z)) {
                dim[0] = 1;
                dim[1] = 2;
            } else if (std::abs(n.y) > std::abs(n.z)) {
                dim[0] = 0;
                dim[1] = 2;
            } else {
                dim[0] = 0;
                dim[1] = 1;
            }

            float A[2][2] = {
                {dpdu[dim[0]], dpdv[dim[0]]},
                {dpdu[dim[1]], dpdv[dim[1]]}            
            };
            float Bx[2] = {px[dim[0]] - p[dim[0]], px[dim[1]] - p[dim[1]]};
            float By[2] = {py[dim[0]] - p[dim[0]], py[dim[1]] - p[dim[1]]};
            if (!solveLinearSys2X2(A, Bx, &dudx, &dvdx)) {dudx = 0; dvdx = 0;}
            if (!solveLinearSys2X2(A, By, &dudy, &dvdy)) {dudy = 0; dvdy = 0;}
        }
    }
};