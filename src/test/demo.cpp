#include <core/scene/scene.h>
#include <core/shape/mesh.h>
#include <core/shape/shape.h>
#include <core/task/task.h>
#include <gperftools/profiler.h>
#include <tbb/tbb.h>

#include <chrono>
#include <iostream>
#include <mutex>

int main(int argc, char **argv) {
  auto task = createTask(argv[1]);
  auto integrator = task->integrator;
  integrator->render(task);
}