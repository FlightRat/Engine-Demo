#pragma once
#include <vector>
#include "Mesh.h"

namespace ENGINE_RENDERING {
    class Model {
    public:
        std::string directory = "";
        std::vector<Mesh> meshes;

        Model() = default;
        Model(std::vector<Mesh> meshes);    // 注意：此处参数不能传 const ref，因为 Mesh 无法拷贝，必须传值并 move，或者传右值引用

        void Draw() const;
    };
}