#include "meshSet.h"

void MeshSet::addMesh(std::unique_ptr<Mesh> meshPtr) {
    mAABB.expands(meshPtr->getAABB3());
    if (count.empty()) {
        count.emplace_back(meshPtr->getFaceNum());
    } else {
        count.emplace_back(meshPtr->getFaceNum() + count.back());
    }
    mMeshes.emplace_back(std::move(meshPtr));
}

void MeshSet::mergeMeshSet(std::unique_ptr<MeshSet> meshSet) {
    mAABB.expands (meshSet->getAABB3());
    for (int i = 0; i < meshSet->size(); ++i) {
        if (count.empty()) {
            count.emplace_back(meshSet->getFaceNum(i));
        } else {
            count.emplace_back(meshSet->getFaceNum(i) + count.back());
        }
    }
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

uint32_t MeshSet::getFaceNum(int meshIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getFaceNum();
}

AABB3f MeshSet::getAABB3(int meshIdx) const {
    const auto &meshPtr = mMeshes[meshIdx];
    return meshPtr->getAABB3();
}

AABB3f MeshSet::getAABB3() const {
    return mAABB;
}

int MeshSet::size() const {
    return mMeshes.size();
}

bool MeshSet::rayIntersectMeshFace(Ray3f &ray, RayIntersectionRec &iRec, int i, int j) const {
    // check whether the ray hit the ith mesh's jth face
    // getThe triangle face first
    const auto &meshPtr = mMeshes[i];
    Point3ui faceBuf = meshPtr->getFBuf(j);
    
    Point3f p0 = meshPtr->getVtxBuf(faceBuf[0]),
            p1 = meshPtr->getVtxBuf(faceBuf[1]),
            p2 = meshPtr->getVtxBuf(faceBuf[2]);
    Vector3f edge1 = p1 - p0,
             edge2 = p2 - p0,
             h = cross(ray.dir, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return false;
    
    Vector3f s = ray.ori - p0;
    float f = 1.0 / a,
          u = f * dot(s, h);
    if (u < .0f || u > 1.f) return false;
    
    Vector3f q = cross(s, edge1);
    float v = f * dot(ray.dir, q);
    if (v < .0f || u + v > 1.f) return false;

    // yes, hit the triangle
    // check the t range
    float t = f * dot(edge2, q);
    if (t <= ray.tmin || t >= ray.tmax) return false;
    // yes, fit the range
    // fill the hitRecord
    ray.tmax = t;

    iRec.isValid = true;
    iRec.t = t;
    iRec.p = 
        (1.f - u - v) * p0 +
        u * p1 +
        v * p2;
    iRec.geoN = Normal3f {cross(edge1, edge2)};
    if (meshPtr->hasNormal()) {
        iRec.shdN = Normal3f {
            (1.f - u - v) * meshPtr->getNmlBuf(faceBuf[0]) +
            u * meshPtr->getNmlBuf(faceBuf[1]) +
            v * meshPtr->getNmlBuf(faceBuf[2])
        };
    } else {
        iRec.shdN = iRec.geoN;
    }
    return true;
}

bool MeshSet::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
    Ray3f _ray {ray};
    RayIntersectionRec _iRec;
    bool hit = false;
    for (int i = 0; i < mMeshes.size(); ++i) {
        const auto& mesh = mMeshes[i];
        for (int j = 0; j < mesh->getFaceNum(); ++j) {
            if (rayIntersectMeshFace(_ray, _iRec, i, j))
                hit = true;
        }
    }
    if (hit) {
        iRec = _iRec;
    }
    return hit;
}

std::string MeshSet::toString() const {
    std::string result;
    for (const auto &meshPtr : mMeshes) {
        result += meshPtr->toString();
        result += "\n";
    }
    return result;
}