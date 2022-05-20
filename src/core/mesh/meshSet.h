#pragma once
#include "mesh.h"

class MeshSet {
public:
    MeshSet() = default;

    void addMesh (std::unique_ptr<Mesh> meshPtr);

    void mergeMeshSet (std::unique_ptr<MeshSet> meshSet);
    
    int size() const;

    Point3ui getFBuf(int meshIdx, int triIdx) const;

    Point3f getVtxBuf(int meshIdx, int vtxIdx) const;

    Normal3f getNmlBuf(int meshIdx, int vtxIdx) const;

    AABB3f getAABB3(int meshIdx) const;

    AABB3f getAABB3() const;

    uint32_t getFaceNum(int meshIdx) const;

    bool rayIntersectMeshFace(Ray3f &ray, RayIntersectionRec &iRec, int meshIdx, int faceIdx) const ;

    bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const;

    void initAccel();

    std::string toString() const;
    
private:
    std::vector<std::unique_ptr<Mesh>> mMeshes;
    std::vector<uint32_t> count;
    AABB3f mAABB;
};