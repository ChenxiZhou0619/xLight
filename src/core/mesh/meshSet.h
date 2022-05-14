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

    std::string toString() const;
    
private:
    std::vector<std::unique_ptr<Mesh>> mMeshes;
};