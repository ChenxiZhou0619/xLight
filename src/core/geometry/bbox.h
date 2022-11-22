/**
 * @file bbox.h
 * @author zcx
 * @brief types of bounding box
 * @version 0.1
 * @date 2022-05-13
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <limits>

#include "ray.h"
/**
 * @brief Base class of bounding box
 *
 * @tparam T
 */
template <typename T>
struct TBBox3 {
  virtual bool rayIntersect(const TRay3<T> &ray) const = 0;

  virtual void expands(const TPoint3<T> &p) = 0;

  virtual bool contains(const TPoint3<T> &p) const = 0;

  virtual bool isValid() const = 0;
};

template <typename T>
struct TAABB3 : public TBBox3<T> {
  TAABB3() : min(FLOATMAX), max(FLOATMIN){};

  TAABB3(const TPoint3<T> p) : min(p), max(p){};

  TAABB3(const TPoint3<T> _min, const TPoint3<T> _max) : min(_min), max(_max){};

  virtual bool rayIntersect(const TRay3<T> &ray) const {
    if (!isValid()) return false;
    float nearT = -std::numeric_limits<float>::infinity();
    float farT = std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < 3; ++axis) {
      float origin = ray.ori[axis], minVal = min[axis], maxVal = max[axis];
      if (ray.dir[axis] == 0) {
        if (origin < minVal || origin > maxVal) return false;
      } else {
        float invDir = 1 / ray.dir[axis], t0 = (minVal - origin) * invDir,
              t1 = (maxVal - origin) * invDir;
        if (t0 > t1) std::swap(t0, t1);

        nearT = std::max(t0, nearT);
        farT = std::min(t1, farT);

        if (nearT > farT) return false;
      }
    }
    return ray.tmin <= farT && nearT <= ray.tmax;
  }

  virtual void expands(const TPoint3<T> &p) {
    min.x = std::min(min.x, p.x) - EPSILON;
    min.y = std::min(min.y, p.y) - EPSILON;
    min.z = std::min(min.z, p.z) - EPSILON;

    max.x = std::max(max.x, p.x) + EPSILON;
    max.y = std::max(max.y, p.y) + EPSILON;
    max.z = std::max(max.z, p.z) + EPSILON;
  }

  virtual void expands(const TAABB3<T> &box) {
    expands(box.min);
    expands(box.max);
  }

  virtual bool contains(const TPoint3<T> &p) const {
    return min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y &&
           min.z <= p.z && p.z <= max.z;
  }

  virtual bool contains(const TAABB3<T> &box) const {
    return contains(box.min) && contains(box.max);
  }

  virtual bool overlaps(const TAABB3<T> &box) const {
    return (max.x >= box.min.x && min.x <= box.max.x) &&
           (max.y >= box.min.y && min.y <= box.max.y) &&
           (max.z >= box.min.z && min.z <= box.max.z);
  }

  virtual bool isValid() const {
    return max.x > min.x && max.y > min.y && max.z > min.z;
  }

  virtual TPoint3<T> getCentroid() const { return (T)0.5 * (min + max); }

  TPoint3<T> min, max;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const TAABB3<T> &box) {
  os << "AABB: min = [ " << box.min << " ], max = [ " << box.max << " ]";
  return os;
}