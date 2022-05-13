#include "meshLoader.h"
#include "mesh.h"
#include "meshSet.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

std::unique_ptr<MeshSet> MeshLoader::loadFromFile(const std::string &filePath) const {
    
    Assimp::Importer importer;
    
    const aiScene *scene = importer.ReadFile( filePath, 
        aiProcess_CalcTangentSpace      |
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType
    );

    if (nullptr == scene) {
        std::cerr << "Error when loading the file " << filePath << std::endl;
        exit(0);
    }

    std::cout << "Convert the format\n";
    
    auto meshSetPtr = std::make_unique<MeshSet>();
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        auto meshPtr = convertFromAIMesh(*(scene->mMeshes[i]));
        meshSetPtr->addMesh(std::move(meshPtr));
    }

    std::cout << filePath << " contains " << meshSetPtr->size() << " meshes\n";

    return meshSetPtr;
}

std::unique_ptr<Mesh> MeshLoader::convertFromAIMesh (const aiMesh &_aiMesh) const {
    // convert the assimp data structure to specified format
    
    std::vector<Point3f>  vertices;
    std::vector<Normal3f> normals;
    std::vector<Point3ui> faces;
    std::vector<Point2f>  UVs;
    
    // the two optional data
    bool hasNormals = true, hasUVs = true;
    
    // load the vertices
    if (!_aiMesh.HasPositions()) {
        std::cerr << "Fatal : Meshes with no vertices\n";
        exit(1);
    } 
    for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
        auto vertex = _aiMesh.mVertices[i];
        vertices.emplace_back(Point3f {vertex[0], vertex[1], vertex[2]});
    }

    // load the normals
    if (!_aiMesh.HasNormals())
        hasNormals = false;
    else {
        // The array is mNumVertices in size (According to the assimp-doc)
        for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
            const auto &normal = _aiMesh.mNormals[i];
            normals.emplace_back(Normal3f {normal[0], normal[1], normal[2]});
        }
    }
    
    // load the faces
    if (!_aiMesh.HasFaces()) {
        std::cerr << "Fatal : Meshes with no faces\n";
        exit(1);
    }
    for (int i = 0; i < _aiMesh.mNumFaces; ++i) {
        const auto &face = _aiMesh.mFaces[i];
        if (face.mNumIndices != 3) {
            std::cerr << "Fatal : Current mesh only support face channels = 3!\n";
            exit(1);
        }
        const auto &indexBuf = face.mIndices;

        faces.emplace_back(Point3ui {indexBuf[0], indexBuf[1], indexBuf[2]});
    }

    // load the UV
    if (_aiMesh.GetNumUVChannels() > 1) {
        std::cerr << "Fatal : Currently not support for uv channels > 1!\n";
        exit(1);
    } else if (_aiMesh.GetNumUVChannels() == 0) {
        hasUVs = false;
    } else if (_aiMesh.GetNumUVChannels() == 1) {
        if (_aiMesh.mNumUVComponents[0] !=2 ) {
            std::cerr << "Fatal : Current only support for uv channels = 2!\n";
            exit(1);
        }
        for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
            const auto &uv = _aiMesh.mTextureCoords[0];
            UVs.emplace_back(Point2f {uv[i][0], uv[i][1]});
        }
    } else {
        std::cerr << "Fatal : Undefined behavior when loading the UV!\n";
        exit(1);
    }

    std::unique_ptr<TriMesh> meshPtr = std::make_unique<TriMesh>(
        std::move(vertices),
        std::move(normals),
        std::move(faces),
        std::move(UVs)        
    );

    return meshPtr;
}
