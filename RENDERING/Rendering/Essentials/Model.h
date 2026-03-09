#pragma once
#include <vector>
#include "Mesh.h"

namespace ENGINE_RENDERING {
    class Model {
    private:
        std::string m_sDirectory = "";
        std::vector<Mesh> meshes;

        bool m_bEditorModel = false;
    public:

        Model() = default;
        Model(std::vector<Mesh> meshes);    // 注意：此处参数不能传 const ref，因为 Mesh 无法拷贝，必须传值并 move，或者传右值引用

        inline const std::string GetDir() { return m_sDirectory; }
        inline void SetDir(const std::string dir) { m_sDirectory = dir; }

        inline const std::vector<Mesh>& GetMeshes() const { return meshes; }
        inline const int GetMeshCount() { return meshes.size(); }

        inline const bool IsEditorModel() const { return m_bEditorModel; }
        inline void SetIsEditorModel(bool bIsEditorModel) { m_bEditorModel = bIsEditorModel; }

        void Draw() const;
    };
}