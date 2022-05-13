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