#include "meshLoader.h"
#include "mesh.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

std::unique_ptr<Mesh> MeshLoader::loadMesh(const std::string &filePath) const {
    
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

    std::cout << "Constructing the mesh\n";
    return nullptr;
}