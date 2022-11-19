#include <chrono>

#include <tbb/tbb.h>
#include <core/task/task.h>
#include <core/render-core/integrator.h>
#include <core/render-core/film.h>
#include <core/render-core/info.h>

class BidirectionalPathTracer : public Integrator {
private:
    enum class TransportMode {Radiance, Importance};

    //* The data structure only use for bdpt
    struct PathVertex {
        //* Indentify the vertex type
        enum class VertexType {Camera, Light, Surface} type;
        float pdf_fwd = .0f, pdf_rev = .0f;
        std::shared_ptr<IntersectionInfo> info = nullptr;
        bool is_delta = false;
        SpectrumRGB beta{.0f};

        static PathVertex create_camera(std::shared_ptr<Camera> camera, Ray3f ray, SpectrumRGB beta);
        static PathVertex create_light(std::shared_ptr<Emitter> light, Ray3f ray, SpectrumRGB beta, float pdf);
        static PathVertex create_surface(std::shared_ptr<IntersectionInfo> si, SpectrumRGB beta, float pdf, const PathVertex &prev);

        bool on_surface() const {

        }

        Point3f p() const {

        } 

        bool is_infinite_light() const {

        }

        bool is_light() const {

        }

        SpectrumRGB Le(const Scene &scene, const PathVertex &to) const {

        }

        bool is_connectible() const {

        }

        Normal3f normal() const {

        }

        float convert_pdf(float pdf, PathVertex next) const {
            if (pdf == FINF) return FINF;
            //TODO* if (next.is_infinite_light()) return pdf;
            Vector3f w = next.p() - p();
            if (w.length2() == 0) return 0;
            float inv_dist2 = 1.f / w.length2();
            if (next.on_surface())
                pdf *= std::abs(dot(next.normal(), w * std::sqrt(inv_dist2)));
            return pdf * inv_dist2;
        }
    };

    // TODO
    int random_walk(const Scene &scene, Sampler *sampler, int max_depth, Ray3f ray,
                    TransportMode mode, float pdf_dir, SpectrumRGB beta,
                    std::vector<PathVertex> *subpath) const
    {
        if (max_depth == 0) return 0;
        int bounces = 0;
        float pdf_fwd = pdf_dir, pdf_rev = .0f;

        while(true) {
            auto surface_its = scene.intersectWithSurface(ray);
            PathVertex &vertex = (*subpath)[bounces + 1], &prev = (*subpath)[bounces];

            if (!surface_its->shape) {
                if (mode == TransportMode::Radiance) {
                    vertex = PathVertex::create_light(surface_its->light, ray, beta, pdf_fwd);
                    ++bounces;
                }
                break;
            }

            //TODO skip the empty boundary

            vertex = PathVertex::create_surface(surface_its, beta, pdf_fwd, prev);
            if (++bounces >= max_depth) break;

            ScatterInfo scatter_info = surface_its->sampleScatter(sampler->next2D());
            if (scatter_info.pdf == 0 || scatter_info.weight.isZero()) break;

            beta *= scatter_info.weight;
            pdf_fwd = scatter_info.pdf;
            pdf_rev = surface_its->pdfScatter(scatter_info.wo, surface_its->wi);
            
            if (pdf_fwd == FINF) {
                vertex.is_delta = true;
                pdf_fwd = pdf_rev = 0;
            }

            // TODO handle the delta distribution
            ray = surface_its->scatterRay(scene, scatter_info.wo);

            prev.pdf_rev = vertex.convert_pdf(pdf_rev, prev);
        }

        return bounces;

    }

    // TODO
    int generate_camera_subpath(const Scene &scene, Sampler *sampler, int max_depth, 
                                std::shared_ptr<Camera> camera, Point2i pixel, Point2i resolution,
                                std::vector<PathVertex> *camera_subpath) const
    {
        Ray3f ray = camera->sampleRayDifferential(pixel, resolution, sampler->getCameraSample());
        camera_subpath->emplace_back(PathVertex::create_camera(camera, ray, SpectrumRGB{1}));

        //* pdf position and pdf direction is 1
        return random_walk(scene, sampler, max_depth-1, ray, 
                           TransportMode::Radiance, 1.f, 
                           SpectrumRGB{1.f}, camera_subpath) + 1;
    }

    // TODO
    int generate_light_subpath(const Scene &scene, Sampler *sampler, int max_depth,
                               std::shared_ptr<Camera> camera, Point2i pixel,
                               std::vector<PathVertex> *light_subpath) const
    {
        // todo
        return 0;
    }

    // TODO
    SpectrumRGB connect_subpath(const Scene &scene, std::shared_ptr<Sampler> sampler, int t, int s,
                                const std::vector<PathVertex> &camera_subpath,
                                const std::vector<PathVertex> &light_subpath,
                                std::shared_ptr<Camera> camera, Point2i *pixel_new) const
    {
        SpectrumRGB L{.0f};
        if (t > 1 && s != 0 && camera_subpath[t - 1].type == PathVertex::VertexType::Light) return L;

        PathVertex sampled;
        if (s == 0) {
            const PathVertex &pt = camera_subpath[t - 1];
            if (pt.is_light()) L = pt.Le(scene, camera_subpath[t - 2]) * pt.beta;
        } else if (t == 1) {
            //* Directly connect the camera and light_subpath
            // todo
        } else if (s == 1){
            //* Sample a light source and connet it to camera subpath
            const PathVertex &pt = camera_subpath[t - 1];
            if (pt.is_connectible()) {
                LightSourceInfo light_info = scene.sampleLightSource(*pt.info, sampler.get());
                sampled = PathVertex::create_light(nullptr, Ray3f(), SpectrumRGB(), .0f);
                
            }
        }
    }


public:
    BidirectionalPathTracer() : mMaxDepth(5) { }

    BidirectionalPathTracer(const rapidjson::Value &_value)
    {
        mMaxDepth = getInt("maxDepth", _value);
    }

    virtual ~BidirectionalPathTracer() = default;

    virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                              Sampler *sampler) const override
    {
        // no implementation
        std::cerr << "BidirectionalPathTracer::getLi not implement!\n";
        std::exit(1);
    }

    virtual void render(std::shared_ptr<RenderTask> task) const override 
    {
        auto start = std::chrono::high_resolution_clock::now();

        Film film {task->film_size, 32};
        auto [x, y] = film.tile_range();

        std::vector<std::shared_ptr<FilmTile>> film_tiles; film_tiles.reserve(x * y);

        int finished_tiles = 0,
            tile_size = film.tile_size;
        double total_tiles = x * y;

        auto integrator = task->integrator;
        auto ori_sampler = task->sampler;
        auto camera = task->camera;
        auto scene = task->scene;

        tbb::parallel_for(
            tbb::blocked_range2d<size_t>(0, x, 0, y), 
            [&](const tbb::blocked_range2d<size_t> &r){
                    for (int row = r.rows().begin(); row != r.rows().end(); ++row)
                        for (int col = r.cols().begin(); col != r.cols().end(); ++col){

                            auto tile = film.get_tile({row, col});
                            auto sampler = ori_sampler->clone();

                            for (int i = 0; i < tile_size; ++i)
                                for (int j = 0; j < tile_size; ++j) {
                                    Point2i p_pixel = tile->pixel_location({i, j});
                                    sampler->startPixel(p_pixel);      
                                    // TODO only consider the situation without medium now
                                    for (int spp = 0; spp < task->getSpp(); ++spp) {                                        
                                        //* Here, the integrator is bi-directional path tracer
                                        
                                        // For every sample per pixel, two paths should be constructed
                                        std::vector<PathVertex> camera_subpath(mMaxDepth + 2); 
                                        std::vector<PathVertex> light_subpath(mMaxDepth + 1);

                                        int len_camera_subpath = generate_camera_subpath(*scene, sampler.get(), mMaxDepth + 2, camera, p_pixel, task->film_size ,&camera_subpath);
                                        int len_light_subpath  = generate_light_subpath(*scene, sampler.get(), mMaxDepth + 1, camera, p_pixel, &light_subpath);
                                        
                                        SpectrumRGB Li{.0f};
                                        for (int t = 1; t <= len_camera_subpath; ++t) {
                                            for (int s = 0; s <= len_light_subpath; ++s) {
                                                int depth = t + s - 2;
                                                if (depth < 0 || depth > mMaxDepth || (s == 1 && t == 1)) continue;
                                                Point2i pixel_new;
                                                SpectrumRGB Lpath = connect_subpath(*scene, sampler, t, s, camera_subpath, light_subpath, camera, &pixel_new);
                                                if (t != 1) Li += Lpath;
                                                else film.add_splat(pixel_new, Lpath, 1.f); //TODO the splat weight
                                            }
                                        }
                                        tile->add_sample(p_pixel, Li, 1.f);
                                        sampler->nextSample();
                                    }
                                }

                            film_tiles.emplace_back(tile);
                            finished_tiles++;
                            if (finished_tiles % 5 == 0) {
                                printProgress((double)finished_tiles/total_tiles);
                            }
                        }
                }
        );
        printProgress(1);
        for (auto tile : film_tiles) {
            film.fill_tile(tile);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << tfm::format("\nRendering costs : %.2f seconds\n",
            (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f);
        film.save_film(task->file_name);  

    }

private:
    int mMaxDepth;
};

REGISTER_CLASS(BidirectionalPathTracer, "bdpt")