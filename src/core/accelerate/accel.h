#pragma once

#include "core/geometry/geometry.h"
#include "core/mesh/meshSet.h"
#include <memory>
struct OcNode;
struct AccelNode {
    AccelNode() = default;

    virtual ~AccelNode() = default; 

    virtual bool rayIntersect(Ray3f &ray, RayIntersectionRec &iRec, const MeshSet &meshSet) const = 0;
};
struct Accel {
    Accel() = default;

    Accel(MeshSet *_meshSetPtr):meshSetPtr(_meshSetPtr) { }

    ~Accel() {
        delete root;
    }

    OcNode* buildOcTree(const AABB3f &_bounds, const std::vector<uint32_t> &faceBuf, int _depth);

    void init();

    bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const ;
    /*data*/
    std::unique_ptr<MeshSet> meshSetPtr;

    AccelNode* root;
};

