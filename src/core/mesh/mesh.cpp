#include "mesh.h"
#include "tinyformat/tinyformat.h"

std::string TriMesh::toString() const {
    return tfm::format("numVertices = %d, numFaces = %d, numNormals = %d, numUVs = %d", 
        mVerticesBuf->size(), mFacesBuf->size(), mNormalsBuf->size(), mUVsBuf->size());
}