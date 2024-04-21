
#include "../UI/Screens/Loading.h"
#include "../Util/Task.h"

#pragma region ForwardDecl
namespace loader {
class IblEnv;
struct FloatImage;
}  // namespace loader
namespace tinygltf {
class Model;
class LoadingScreen;
}  // namespace tinygltf

#pragma endregion

class MainControllerLoader {
   public:
    struct Data {
        std::shared_ptr<loader::IblEnv> environment;
        std::shared_ptr<loader::IblEnv> environmentSpecular;
        std::shared_ptr<loader::IblEnv> environmentDiffuse;
        std::shared_ptr<loader::FloatImage> iblBrdfLut;

        std::shared_ptr<const tinygltf::Model> gltf;
    };

   private:
    bool firstTime_ = true;
    std::unique_ptr<Task<Data>> task_;
    std::unique_ptr<LoadingScreen> screen_;
    bool loading_ = false;

    static void load_(Data& out, bool load_gltf);

   public:
    MainControllerLoader();
    ~MainControllerLoader() = default;

    void load();

    void update() {
        // isLoading is called multiple times per frame, but it must always return the same value;
        loading_ = task_ != nullptr && !task_->isFinished();
    }

    bool isLoading() {
        return loading_;
    }

    bool isDone() {
        return !loading_ && task_ != nullptr;
    }

    void draw();

    /**
     * Returns the result and releases the task
     */
    Data result();
};