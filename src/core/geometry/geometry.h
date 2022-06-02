/**
 * @file geometry.h
 * @author zcx
 * @brief forward decleration
 * @version 0.1
 * @date 2022-05-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include "normal.h"

#include "point.h"
using Point2f = TPoint2<float>;
using Point3f = TPoint3<float>;
using Point3ui = TPoint3<unsigned int>;

#include "vector.h"
using Vector2f = TVector2<float>;
using Vector3f = TVector3<float>;
using Vector2i = TVector2<int>;


#include "ray.h"
using Ray3f = TRay3<float>;
using Ray3d = TRay3<double>;

#include "bbox.h"
using AABB3f = TAABB3<float>;

#include "frame.h"