#include "accel.h"

#include "octree.h"

/**
 * @brief build the acceleration structure
 *
 */
void Accel::init() {
  AABB3f bounds = meshSetPtr->getAABB3();
  std::vector<uint32_t> _triBuf;
  _triBuf.reserve(meshSetPtr->getTotalTriNum());

  for (uint32_t i = 0; i < meshSetPtr->getTotalTriNum(); ++i)
    _triBuf.emplace_back(i);

  root = buildOcTree(bounds, _triBuf, 0);
}

OcNode *Accel::buildOcTree(const AABB3f &_bounds,
                           const std::vector<uint32_t> &_triBuf, int _depth) {
  if (_triBuf.size() == 0) return nullptr;

  if (_triBuf.size() < ocLeafMaxSize) {
    OcNode *node = new OcNode{_bounds, _triBuf, _depth};
    return node;
  }

  if (_depth > maxDepth) {
    OcNode *node = new OcNode{_bounds, _triBuf, _depth};
    return node;
  }

  std::vector<uint32_t> _triSubBufs[nSubs];
  OcNode *node = new OcNode{_bounds, _depth};
  const auto &subBounds = node->getSubBounds();

  for (const auto &_triIdx : _triBuf) {
    const auto &triBounds = meshSetPtr->getTriBounds(_triIdx);
    for (int i = 0; i < nSubs; ++i) {
      if (triBounds.overlaps(subBounds[i])) {
        _triSubBufs[i].emplace_back(_triIdx);
      }
    }
  }
  for (int i = 0; i < nSubs; ++i) {
    node->subNodes[i] = buildOcTree(subBounds[i], _triSubBufs[i], _depth + 1);
  }
  return node;
}

bool Accel::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
  Ray3f _ray{ray};
  return root->rayIntersect(_ray, iRec, *meshSetPtr);
}

bool Accel::rayIntersect(const Ray3f &ray) const {
  return root->rayIntersect(ray, *meshSetPtr);
}