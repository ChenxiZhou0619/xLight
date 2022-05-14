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

Point3ui MeshSet::getFBuf(int meshIdx, int triIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getFBuf(triIdx);
}

Point3f MeshSet::getVtxBuf(int meshIdx, int vtxIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getVtxBuf(vtxIdx);
}

Normal3f MeshSet::getNmlBuf(int meshIdx, int vtxIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getNmlBuf(vtxIdx);
}

AABB3f MeshSet::getAABB3(int meshIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getAABB3();
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