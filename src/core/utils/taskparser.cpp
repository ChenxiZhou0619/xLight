#include "taskparser.h"
#include "rapidjson/document.h"
#include "core/file/image.h"
#include "core/utils/configurable.h"
#include <fstream>
#include <iostream>

Vector2i RenderTask::getImgSize() const {
    return image->getSize();
}
uint32_t RenderTask::getSpp() const {
    return image->getSpp();
}

std::unique_ptr<RenderTask> RenderTaskParser::createTask(const std::string &jsonFilePath) {
    RenderTask *task = new RenderTask();

    /*=========== read the file and parse ==============*/
    std::ifstream jsonFile;
    jsonFile.open(jsonFilePath, std::ios::in);
    std::streampos begin, end;
    begin = jsonFile.tellg();
    jsonFile.seekg(0, std::ios::end);
    end = jsonFile.tellg();
    jsonFile.seekg(0, std::ios::beg);
    char *json = new char[end - begin];
    jsonFile.read(json, end-begin);
    jsonFile.close();
    rapidjson::Document document;
    document.Parse(json);
    delete json;

    /*============= traverse the document object =========*/
    for (auto itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
        if (strcmp(itr->name.GetString(), "output") == 0) {
            // configure the img object
            const std::string &filename 
                = (*itr).value["filename"].GetString();
            const auto &size 
                = (*itr).value["size"].GetArray();
            int spp 
                = (*itr).value["spp"].GetInt();
            task->image = std::make_unique<Image>(
                Vector2i {size[0].GetInt(), size[1].GetInt()},
                filename,
                spp
            );
            std::cout << "========== Output setting ==========\n";
            std::cout << *task->image << std::endl;
            std::cout << "====================================\n";
        } 
        else if (strcmp(itr->name.GetString(), "scene") == 0) {
            // configure the scene object
            MeshLoader meshLoader;
            const auto &entities = (*itr).value["entities"].GetArray();
            const std::string &filepath = entities[0].GetObject()["filepath"].GetString();
            task->scene = std::make_unique<Scene>(
                meshLoader.loadFromFile(filepath)
            );
            task->scene->preprocess();

        } else if (strcmp(itr->name.GetString(), "renderer") == 0) {
            // configure the renderer settings
            const std::string 
                &samplerType = (*itr).value["sampler"]["type"].GetString();
            task->sampler.reset(static_cast<Sampler *> (ObjectFactory::createInstance(samplerType, (*itr).value["sampler"]))); 
            std::cout << "Configure the sampler : " << samplerType << std::endl;        

            const std::string
                &cameraType = (*itr).value["camera"]["type"].GetString();
            task->camera.reset(static_cast<Camera *> (ObjectFactory::createInstance(cameraType, (*itr).value["camera"])));
            std::cout << "Configure the camera : " << cameraType << std::endl;

            const std::string 
                &integratorType = (*itr).value["integrator"]["type"].GetString();
            task->integrator.reset(static_cast<Integrator *> (ObjectFactory::createInstance(integratorType, (*itr).value["integrator"])));
            std::cout << "Configure the integrator : " << integratorType << std::endl;
        }
    }

    std::unique_ptr<RenderTask> res {nullptr}; res.reset(task);
    return res;
}