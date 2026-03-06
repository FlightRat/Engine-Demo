#include "Model.h"
#include <utility>

namespace ENGINE_RENDERING {
    Model::Model(std::vector<Mesh> meshes) {
        // 【关键】Mesh 不可拷贝，必须使用 std::move 将所有权转移给 Model
        this->meshes = std::move(meshes);
    }

    void Model::Draw() const {
        for (const auto& mesh : meshes) {
            mesh.Draw();
        }
    }
}