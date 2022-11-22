#include "octree.h"

std::vector<AABB3f> OcNode::getSubBounds() const {
  // std::cout << nodeBox.toString() << std::endl;

  Point3f center{bounds.getCentroid()};
  Vector3f halfEdge{center - bounds.min};

  std::vector<AABB3f> subBoxes;

  subBoxes.emplace_back(AABB3f{bounds.min, center});
  subBoxes.emplace_back(AABB3f{bounds.min + Vector3f{halfEdge[0], .0f, .0f},
                               center + Vector3f{halfEdge[0], .0f, .0f}});
  subBoxes.emplace_back(
      AABB3f{bounds.min + Vector3f{halfEdge[0], halfEdge[1], .0f},
             center + Vector3f{halfEdge[0], halfEdge[1], .0f}});
  subBoxes.emplace_back(AABB3f{bounds.min + Vector3f{.0f, halfEdge[1], .0f},
                               center + Vector3f{.0f, halfEdge[1], .0f}});
  subBoxes.emplace_back(AABB3f{bounds.min + Vector3f{.0f, .0f, halfEdge[2]},
                               center + Vector3f{.0f, .0f, halfEdge[2]}});
  subBoxes.emplace_back(
      AABB3f{bounds.min + Vector3f{halfEdge[0], .0f, halfEdge[2]},
             center + Vector3f{halfEdge[0], .0f, halfEdge[2]}});
  subBoxes.emplace_back(
      AABB3f{bounds.min + Vector3f{halfEdge[0], halfEdge[1], halfEdge[2]},
             center + Vector3f{halfEdge[0], halfEdge[1], halfEdge[2]}});
  subBoxes.emplace_back(
      AABB3f{bounds.min + Vector3f{.0f, halfEdge[1], halfEdge[2]},
             center + Vector3f{.0f, halfEdge[1], halfEdge[2]}});

  // for (const auto &box : subBoxes) {
  //     std::cout << box.toString () << std::endl;
  // }

  return subBoxes;
}

bool OcNode::isLeaf() const {
  return faceBufPtr != nullptr && faceBufPtr->size() != 0;
}

bool OcNode::rayIntersect(Ray3f &ray, RayIntersectionRec &iRec,
                          const MeshSet &meshSet) const {
  // miss the bounds, return false
  if (!bounds.rayIntersect(ray)) return false;
  // if leafNode
  if (isLeaf()) {
    // force brute check every tri
    for (uint32_t _triIdx : *faceBufPtr) {
      meshSet.rayIntersectTri(ray, iRec, _triIdx);
    }
  } else {
    // force brute its every child
    for (int i = 0; i < nSubs; ++i) {
      if (subNodes[i] != nullptr) subNodes[i]->rayIntersect(ray, iRec, meshSet);
    }
  }
  return iRec.isValid;
}

bool OcNode::rayIntersect(const Ray3f &ray, const MeshSet &meshSet) const {
  if (!bounds.rayIntersect(ray)) return false;

  if (isLeaf()) {
    for (uint32_t _triIdx : *faceBufPtr) {
      if (meshSet.rayIntersectTri(ray, _triIdx)) return true;
    }
  } else {
    for (int i = 0; i < nSubs; ++i) {
      if (subNodes[i] != nullptr) {
        if (subNodes[i]->rayIntersect(ray, meshSet)) return true;
      }
    }
  }

  return false;
}
