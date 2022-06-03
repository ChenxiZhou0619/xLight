#include "mesh.h"
#include "tinyformat/tinyformat.h"
#include "core/render-core/emitter.h"
#include "core/render-core/sampler.h"

void Mesh::setBSDF(BSDF *_bsdf) {
    mBSDF = _bsdf;
}

BSDF* Mesh::getBSDF() const {
    return mBSDF;
}

bool Mesh::isEmitter() const {
    return mEmitter != nullptr;
}

Emitter* Mesh::getEmitter() const {
    return mEmitter;
}

void Mesh::setEmitter(Emitter *_emitter) {
    mEmitter = _emitter;
}

float Mesh::getMeshSurfaceArea() const {
    return mTotalArea;
}


TriMesh::TriMesh( std::vector<Point3f> &&_mVerticesBuf, std::vector<Normal3f> &&_mNormalsBuf,
    std::vector<Point3ui> &&_mFacesBuf, std::vector<Point2f> &&_mUVsBuf, const char *_name
): mVerticesBuf(std::make_unique<std::vector<Point3f>>(_mVerticesBuf)),
   mNormalsBuf(std::make_unique<std::vector<Normal3f>>(_mNormalsBuf)),
   mFacesBuf(std::make_unique<std::vector<Point3ui>>(_mFacesBuf)),
   mUVsBuf(std::make_unique<std::vector<Point2f>>(_mUVsBuf)) {
    mName = std::string(_name);
    // initialize for aabb
    for (const Point3f &p : *mVerticesBuf) {
        mAABB.expands(p);
    }
    for (uint32_t triIdx = 0; triIdx < mFacesBuf->size(); ++triIdx) {
        mTriDistribution.append(getTriArea(triIdx));
    }
    
    mTotalArea = mTriDistribution.normalize();
}

float TriMesh::getTriArea(uint32_t triIdx) const {
    const auto &fBuf = getFBuf(triIdx);
    Point3f
        p0 = getVtxBuf(fBuf[0]),
        p1 = getVtxBuf(fBuf[1]),
        p2 = getVtxBuf(fBuf[2]);
    return .5f * cross(p0 - p1, p0 - p2).length();
}

Point3ui TriMesh::getFBuf(int triIdx) const {
    return (*mFacesBuf)[triIdx];
}

Point3f TriMesh::getVtxBuf(int vtxIdx) const {
    return (*mVerticesBuf)[vtxIdx];
}

Normal3f TriMesh::getNmlBuf(int vtxIdx) const {
    if (mNormalsBuf->size() == 0) {
        // caculate the Normal
        std::cout << "Not implement getNmlBuf when need to compute the normal\n";
        exit(1);
    }
    return (*mNormalsBuf)[vtxIdx];
}

AABB3f Mesh::getAABB3() const {
    return mAABB;
}

AABB3f TriMesh::getTriBounds(uint32_t triIdx) const {
    Point3ui fBuf = getFBuf(triIdx);
    Point3f p0 = getVtxBuf(fBuf[0]),
            p1 = getVtxBuf(fBuf[1]),
            p2 = getVtxBuf(fBuf[2]);
    AABB3f bounds;
    bounds.expands(p0);bounds.expands(p1),bounds.expands(p2);
    return bounds;
}

bool TriMesh::hasNormal() const {
    return !mNormalsBuf->empty();
}

int TriMesh::getFaceNum() const {
    return mFacesBuf->size();
}

std::string TriMesh::toString() const {
    return tfm::format("numVertices = %d, numFaces = %d, numNormals = %d, numUVs = %d", 
        mVerticesBuf->size(), mFacesBuf->size(), mNormalsBuf->size(), mUVsBuf->size());
}

void TriMesh::sampleOnSurface(PointQueryRecord &pRec, Sampler *sampler) const {
    uint32_t triIdx = mTriDistribution.sample(sampler->next1D());
    const auto &fBuf
        = getFBuf(triIdx);
    Point3f p0 = getVtxBuf(fBuf[0]),
            p1 = getVtxBuf(fBuf[1]),
            p2 = getVtxBuf(fBuf[2]);
    Point2f uv = sampler->next2D();
    Point3f barycentric {
        1.f - std::sqrt(1 - uv.x),
        uv.y * std::sqrt(1 - uv.x),
        .0f
    };
    barycentric[2] = std::max(.0f, 1.f - barycentric[0] - barycentric[1]);
    pRec.p = barycentric.x * p0 + barycentric.y * p1 + barycentric.z * p2;
    if (hasNormal()) {
        Normal3f n0 = getNmlBuf(fBuf[0]),
                 n1 = getNmlBuf(fBuf[1]),
                 n2 = getNmlBuf(fBuf[2]);
        pRec.normal = barycentric.x * n0 + barycentric.y * n1 + barycentric.z * n2;
    } else {
        // TODO maybe need to check the clockwise
        Vector3f A = p1 - p0,
                 B = p2 - p0;
        pRec.normal = Normal3f {normalize(cross(A, B))};
    }
    pRec.mesh = this;
}