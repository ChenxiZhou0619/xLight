#pragma once

#include "core/geometry/geometry.h"
#include "core/mesh/meshSet.h"

struct Accel {
    Accel() = default;

    Accel(std::unique_ptr<MeshSet> _meshSetPtr):meshSetPtr(std::move(_meshSetPtr)) { }

    std::unique_ptr<MeshSet> meshSetPtr;

    std::unique_ptr<AccelNode> root;

    OcNode* buildOcTree(const AABB3f &_bounds, const std::vector<uint32_t> &faceBuf, int _depth);
};

struct AccelNode {
    AccelNode() = default;

    virtual bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const = 0;
};
