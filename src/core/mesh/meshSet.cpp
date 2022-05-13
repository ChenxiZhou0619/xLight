#include "meshSet.h"

void MeshSet::addMesh(std::unique_ptr<Mesh> meshPtr) {
    mMeshes.emplace_back(std::move(meshPtr));
}

void MeshSet::mergeMeshSet(std::unique_ptr<MeshSet> meshSet) {
    mMeshes.insert (
        mMeshes.end(),
        std::make_move_iterator(meshSet->mMeshes.begin()),
        std::make_move_iterator(meshSet->mMeshes.end())
    );
}

int MeshSet::size() const {
    return mMeshes.size();
}

std::string MeshSet::toString() const {
    std::string result;
    for (const auto &meshPtr : mMeshes) {
        result += meshPtr->toString();
        result += "\n";
    }
    return result;
}