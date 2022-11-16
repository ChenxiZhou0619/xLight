#include <iostream>
#include <core/shape/shape.h>
#include <core/shape/mesh.h>
#include <core/scene/scene.h>
#include <core/task/task.h>

#include <chrono>
#include <tbb/tbb.h>
#include <mutex>

#include <gperftools/profiler.h>

int main(int argc, char **argv) {
    auto task = createTask(argv[1]);
    auto integrator = task->integrator;
    integrator->render(task);
}