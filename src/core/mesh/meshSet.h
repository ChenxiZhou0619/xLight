#pragma once
#include "mesh.h"

class MeshSet {
public:
    MeshSet() = default;

    void addMesh (std::unique_ptr<Mesh> meshPtr);

    void mergeMeshSet (std::unique_ptr<MeshSet> meshSet);
    
    AABB3f getAABB3() const;

    AABB3f getTriBounds(uint32_t _triIdx) const;

    uint32_t getTotalTriNum() const;

    void initAccel();

    std::string toString() const;

private:
    uint32_t getMeshNum() const;

    uint32_t getFaceNum(int meshIdx) const;

    decltype(auto) idxConvert(uint32_t idx) const;

    Point3ui getFBuf(int meshIdx, int triIdx) const;

    Point3f getVtxBuf(int meshIdx, int vtxIdx) const;

    Normal3f getNmlBuf(int meshIdx, int vtxIdx) const;

    bool rayIntersectTri(Ray3f &ray, RayIntersectionRec &iRec, uint32_t _triIdx) const ;

    AABB3f getAABB3(int meshIdx) const;

    
private:
    std::vector<std::unique_ptr<Mesh>> mMeshes;
    std::vector<uint32_t> count;
    AABB3f mAABB;
};