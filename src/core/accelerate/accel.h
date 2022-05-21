#pragma once

#include "core/geometry/geometry.h"
#include "core/mesh/meshSet.h"
struct OcNode;

struct Accel {
    Accel() = default;

    Accel(std::unique_ptr<MeshSet> _meshSetPtr):meshSetPtr(std::move(_meshSetPtr)) { }

    OcNode* buildOcTree(const AABB3f &_bounds, const std::vector<uint32_t> &faceBuf, int _depth);

    void init();

    bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const ;
    /*data*/
    std::unique_ptr<MeshSet> meshSetPtr;

    std::unique_ptr<AccelNode> root;
};

struct AccelNode {
    AccelNode() = default;

    virtual bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const = 0;
};
