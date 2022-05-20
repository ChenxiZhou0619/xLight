#pragma once

#include "accel.h"
#include "core/mesh/mesh.h"
#include <memory>
#include <vector>

constexpr int nSubs = 8;
constexpr int ocLeafMaxSize = 64;
constexpr int maxDepth = 10;
struct OcNode : public AccelNode {
    AABB3f bounds;
    int depth;
    std::unique_ptr<std::vector<uint32_t>> faceBufPtr {nullptr};
    OcNode *subNodes[nSubs] {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    std::shared_ptr<Mesh> meshPtr;

    OcNode(const AABB3f &_bounds, int _depth) : bounds(_bounds), depth(_depth) { }

    OcNode(const AABB3f &_bounds, const std::vector<uint32_t> &faceBuf, int _depth)
        :bounds(_bounds), faceBufPtr(std::make_unique<std::vector<uint32_t>>(faceBuf)), depth(_depth) { }

    virtual ~OcNode() {
        for (int i = 0; i < nSubs; ++i)
            delete subNodes[i];
    }

    std::vector<AABB3f> getSubBounds();

    virtual bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const;

};