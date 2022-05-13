#pragma once
#include "mesh.h"

class MeshSet {
public:
    MeshSet() = default;

    void addMesh (std::unique_ptr<Mesh> meshPtr);

    void mergeMeshSet (std::unique_ptr<MeshSet> meshSet);
    
    int size() const;

    std::string toString() const;
    
private:
    std::vector<std::unique_ptr<Mesh>> mMeshes;
};