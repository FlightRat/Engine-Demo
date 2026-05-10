#include "ModelLoader.h"
#include "Logger/Logger.h"
#include <filesystem>
#include "TextureCommon.h"

namespace ENGINE_RENDERING {
    namespace Primitives {

        void CalculateTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
            // 1. 清空/初始化所有切线和副切线为 0 向量
            for (auto& v : vertices) {
                v.Tangent = glm::vec3(0.0f);
                v.Bitangent = glm::vec3(0.0f);
            }

            // 2. 遍历每个三角形面，计算切线并累加到顶点
            for (size_t i = 0; i < indices.size(); i += 3) {
                Vertex& v1 = vertices[indices[i]];
                Vertex& v2 = vertices[indices[i + 1]];
                Vertex& v3 = vertices[indices[i + 2]];

                glm::vec3 edge1 = v2.Position - v1.Position;
                glm::vec3 edge2 = v3.Position - v1.Position;
                glm::vec2 deltaUV1 = v2.TexCoords - v1.TexCoords;
                glm::vec2 deltaUV2 = v3.TexCoords - v1.TexCoords;

                float determinant = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                float f = (determinant == 0.0f) ? 0.0f : 1.0f / determinant;

                glm::vec3 tangent;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                glm::vec3 bitangent;
                bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                v1.Tangent += tangent;
                v2.Tangent += tangent;
                v3.Tangent += tangent;

                v1.Bitangent += bitangent;
                v2.Bitangent += bitangent;
                v3.Bitangent += bitangent;
            }

            // 3. Gram-Schmidt 正交化与归一化
            for (auto& v : vertices) {
                const glm::vec3& n = v.Normal;
                const glm::vec3& t = v.Tangent;
                const glm::vec3& b = v.Bitangent;

                if (glm::length(t) > 0.0001f && glm::length(n) > 0.0001f) {
                    v.Tangent = glm::normalize(t - n * glm::dot(n, t));
                }

                if (glm::length(b) > 0.0001f && glm::length(n) > 0.0001f) {
                    glm::vec3 orthogonalized_b = b - n * glm::dot(n, b);
                    orthogonalized_b = orthogonalized_b - v.Tangent * glm::dot(v.Tangent, orthogonalized_b);
                    v.Bitangent = glm::normalize(orthogonalized_b);
                }
            }
        }

        // ===========================================================
        // Cube —— 缠绕已正确（CCW），无需改动
        // ===========================================================
        void LoadCube(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            glm::vec3 p0(-1.0f, -1.0f, 1.0f);
            glm::vec3 p1(1.0f, -1.0f, 1.0f);
            glm::vec3 p2(1.0f, 1.0f, 1.0f);
            glm::vec3 p3(-1.0f, 1.0f, 1.0f);
            glm::vec3 p4(-1.0f, -1.0f, -1.0f);
            glm::vec3 p5(1.0f, -1.0f, -1.0f);
            glm::vec3 p6(1.0f, 1.0f, -1.0f);
            glm::vec3 p7(-1.0f, 1.0f, -1.0f);

            // Front (+Z)
            vertex_data.push_back({ p0, {0, 0, 1}, {0, 0} });
            vertex_data.push_back({ p1, {0, 0, 1}, {1, 0} });
            vertex_data.push_back({ p2, {0, 0, 1}, {1, 1} });
            vertex_data.push_back({ p3, {0, 0, 1}, {0, 1} });
            // Back (-Z)
            vertex_data.push_back({ p4, {0, 0, -1}, {1, 0} });
            vertex_data.push_back({ p5, {0, 0, -1}, {0, 0} });
            vertex_data.push_back({ p6, {0, 0, -1}, {0, 1} });
            vertex_data.push_back({ p7, {0, 0, -1}, {1, 1} });
            // Right (+X)
            vertex_data.push_back({ p1, {1, 0, 0}, {0, 0} });
            vertex_data.push_back({ p5, {1, 0, 0}, {1, 0} });
            vertex_data.push_back({ p6, {1, 0, 0}, {1, 1} });
            vertex_data.push_back({ p2, {1, 0, 0}, {0, 1} });
            // Left (-X)
            vertex_data.push_back({ p4, {-1, 0, 0}, {0, 0} });
            vertex_data.push_back({ p0, {-1, 0, 0}, {1, 0} });
            vertex_data.push_back({ p3, {-1, 0, 0}, {1, 1} });
            vertex_data.push_back({ p7, {-1, 0, 0}, {0, 1} });
            // Top (+Y)
            vertex_data.push_back({ p3, {0, 1, 0}, {0, 0} });
            vertex_data.push_back({ p2, {0, 1, 0}, {1, 0} });
            vertex_data.push_back({ p6, {0, 1, 0}, {1, 1} });
            vertex_data.push_back({ p7, {0, 1, 0}, {0, 1} });
            // Bottom (-Y)
            vertex_data.push_back({ p4, {0, -1, 0}, {0, 0} });
            vertex_data.push_back({ p5, {0, -1, 0}, {1, 0} });
            vertex_data.push_back({ p1, {0, -1, 0}, {1, 1} });
            vertex_data.push_back({ p0, {0, -1, 0}, {0, 1} });

            unsigned int indices[] = {
                0, 1, 2,   0, 2, 3,
                4, 5, 6,   4, 6, 7,
                8, 9,10,   8,10,11,
               12,13,14,  12,14,15,
               16,17,18,  16,18,19,
               20,21,22,  20,22,23
            };

            index_data.assign(indices, indices + sizeof(indices) / sizeof(unsigned int));
            CalculateTangents(vertex_data, index_data);
        }

        // ===========================================================
        // Sphere —— 缠绕已正确（CCW），无需改动
        // ===========================================================
        void LoadSphere(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec2> uv;
            std::vector<glm::vec3> normals;

            const unsigned int X_SEGMENTS = 64;
            const unsigned int Y_SEGMENTS = 64;
            const float PI = 3.14159265359f;

            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                    float xSegment = (float)x / (float)X_SEGMENTS;
                    float ySegment = (float)y / (float)Y_SEGMENTS;
                    float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                    float yPos = std::cos(ySegment * PI);
                    float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                    positions.push_back(glm::vec3(xPos, yPos, zPos));
                    uv.push_back(glm::vec2(xSegment, ySegment));
                    normals.push_back(glm::vec3(xPos, yPos, zPos));
                }
            }

            for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
                for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
                    unsigned int p0 = y * (X_SEGMENTS + 1) + x;
                    unsigned int p1 = y * (X_SEGMENTS + 1) + x + 1;
                    unsigned int p2 = (y + 1) * (X_SEGMENTS + 1) + x + 1;
                    unsigned int p3 = (y + 1) * (X_SEGMENTS + 1) + x;

                    index_data.push_back(p0);
                    index_data.push_back(p1);
                    index_data.push_back(p2);
                    index_data.push_back(p0);
                    index_data.push_back(p2);
                    index_data.push_back(p3);
                }
            }

            for (size_t i = 0; i < positions.size(); ++i) {
                Vertex v;
                v.Position = positions[i];
                v.Normal = normals[i];
                v.TexCoords = uv[i];
                vertex_data.push_back(v);
            }

            CalculateTangents(vertex_data, index_data);
        }

        // ===========================================================
        // Capsule —— 【修复】三段全部反转缠绕
        //   原因：v 增大方向 y 增大，与 Sphere 相反，原代码导致整体 CW
        // ===========================================================
        void LoadCapsule(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            const float RADIUS = 1.0f;
            const float HALF_HEIGHT = 1.0f;
            const unsigned int SEGMENTS = 32;
            const unsigned int Y_SEGMENTS_HALF = SEGMENTS / 2;
            const float PI = 3.14159265359f;

            std::vector<glm::vec3> positions;
            std::vector<glm::vec2> uv;
            std::vector<glm::vec3> normals;
            std::vector<unsigned int> indices;

            unsigned int base_vertex_index = 0;

            // ---------------- A. 顶部半球 ----------------
            for (unsigned int y = 0; y <= Y_SEGMENTS_HALF; ++y) {
                for (unsigned int x = 0; x <= SEGMENTS; ++x) {
                    float xSegment = (float)x / (float)SEGMENTS;
                    float ySegment = (float)y / (float)SEGMENTS;
                    float phi = (0.5f - ySegment) * PI;

                    float xPos_s = RADIUS * std::cos(xSegment * 2.0f * PI) * std::sin(phi);
                    float zPos_s = RADIUS * std::sin(xSegment * 2.0f * PI) * std::sin(phi);
                    float yPos_s = RADIUS * std::cos(phi);

                    glm::vec3 pos = glm::vec3(xPos_s, yPos_s + HALF_HEIGHT, zPos_s);
                    glm::vec3 normal = glm::normalize(pos - glm::vec3(0.0f, HALF_HEIGHT, 0.0f));

                    positions.push_back(pos);
                    normals.push_back(normal);
                    uv.push_back(glm::vec2(xSegment, ySegment * 2.0f));
                }
            }
            for (unsigned int y = 0; y < Y_SEGMENTS_HALF; ++y) {
                for (unsigned int x = 0; x < SEGMENTS; ++x) {
                    unsigned int p0 = y * (SEGMENTS + 1) + x;
                    unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
                    unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
                    unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

                    // 【修复】反转缠绕：CCW from outside
                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p2);
                    indices.push_back(base_vertex_index + p1);

                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p3);
                    indices.push_back(base_vertex_index + p2);
                }
            }
            base_vertex_index = positions.size();

            // ---------------- B. 中间圆柱体 ----------------
            const unsigned int H_SEGMENTS = 1;
            for (unsigned int y = 0; y <= H_SEGMENTS; ++y) {
                float y_normalized = (float)y / (float)H_SEGMENTS;
                for (unsigned int x = 0; x <= SEGMENTS; ++x) {
                    float xSegment = (float)x / (float)SEGMENTS;
                    float xPos = RADIUS * std::cos(xSegment * 2.0f * PI);
                    float zPos = RADIUS * std::sin(xSegment * 2.0f * PI);
                    float yPos = -HALF_HEIGHT + y_normalized * (2.0f * HALF_HEIGHT);

                    positions.push_back(glm::vec3(xPos, yPos, zPos));
                    normals.push_back(glm::normalize(glm::vec3(xPos, 0.0f, zPos)));
                    uv.push_back(glm::vec2(xSegment, y_normalized));
                }
            }
            for (unsigned int y = 0; y < H_SEGMENTS; ++y) {
                for (unsigned int x = 0; x < SEGMENTS; ++x) {
                    unsigned int p0 = y * (SEGMENTS + 1) + x;
                    unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
                    unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
                    unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

                    // 【修复】反转缠绕
                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p2);
                    indices.push_back(base_vertex_index + p1);

                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p3);
                    indices.push_back(base_vertex_index + p2);
                }
            }
            base_vertex_index = positions.size();

            // ---------------- C. 底部半球 ----------------
            for (unsigned int y = 0; y <= Y_SEGMENTS_HALF; ++y) {
                for (unsigned int x = 0; x <= SEGMENTS; ++x) {
                    float xSegment = (float)x / (float)SEGMENTS;
                    float ySegment_rel = (float)y / (float)Y_SEGMENTS_HALF;
                    float phi = ySegment_rel * PI / 2.0f;

                    float xPos_s = RADIUS * std::cos(xSegment * 2.0f * PI) * std::sin(phi);
                    float zPos_s = RADIUS * std::sin(xSegment * 2.0f * PI) * std::sin(phi);
                    float yPos_s = -RADIUS * std::cos(phi);

                    glm::vec3 pos = glm::vec3(xPos_s, yPos_s - HALF_HEIGHT, zPos_s);
                    glm::vec3 normal = glm::normalize(pos - glm::vec3(0.0f, -HALF_HEIGHT, 0.0f));

                    positions.push_back(pos);
                    normals.push_back(normal);
                    uv.push_back(glm::vec2(xSegment, 1.0f - ySegment_rel));
                }
            }
            for (unsigned int y = 0; y < Y_SEGMENTS_HALF; ++y) {
                for (unsigned int x = 0; x < SEGMENTS; ++x) {
                    unsigned int p0 = y * (SEGMENTS + 1) + x;
                    unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
                    unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
                    unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

                    // 【修复】反转缠绕
                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p2);
                    indices.push_back(base_vertex_index + p1);

                    indices.push_back(base_vertex_index + p0);
                    indices.push_back(base_vertex_index + p3);
                    indices.push_back(base_vertex_index + p2);
                }
            }

            for (size_t i = 0; i < positions.size(); ++i) {
                Vertex v;
                v.Position = positions[i];
                v.Normal = normals[i];
                v.TexCoords = uv[i];
                vertex_data.push_back(v);
            }

            index_data = indices;
            CalculateTangents(vertex_data, index_data);
        }

        // ===========================================================
        // Plane —— 【修复】反转缠绕，让法线指向 +Y
        // ===========================================================
        void LoadPlane(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data) {
            Vertex unique_vertices[] = {
                { glm::vec3(10.0f, 0.0f,  10.0f), glm::vec3(0, 1, 0), glm::vec2(10.0f, 0.0f) }, // 0
                { glm::vec3(-10.0f, 0.0f,  10.0f), glm::vec3(0, 1, 0), glm::vec2(0.0f, 0.0f) }, // 1
                { glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(0, 1, 0), glm::vec2(0.0f,10.0f) }, // 2
                { glm::vec3(10.0f, 0.0f, -10.0f), glm::vec3(0, 1, 0), glm::vec2(10.0f,10.0f) }  // 3
            };
            vertex_data.assign(unique_vertices, unique_vertices + 4);

            // 【修复】反转缠绕：原 (0,1,2 / 0,2,3) 法线为 -Y，现改为 (0,2,1 / 0,3,2)
            unsigned int indices[] = {
                0, 2, 1,
                0, 3, 2
            };
            index_data.assign(indices, indices + 6);

            CalculateTangents(vertex_data, index_data);
        }

        // ===========================================================
        // Skybox —— 保留 CCW from inside
        //   渲染时请使用 glCullFace(GL_FRONT) 或 glDisable(GL_CULL_FACE)
        // ===========================================================
        void LoadSkybox(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            glm::vec3 p[] = {
                glm::vec3(-1.0f,  1.0f, -1.0f), // 0
                glm::vec3(-1.0f, -1.0f, -1.0f), // 1
                glm::vec3(1.0f, -1.0f, -1.0f), // 2
                glm::vec3(1.0f,  1.0f, -1.0f), // 3
                glm::vec3(-1.0f,  1.0f,  1.0f), // 4
                glm::vec3(-1.0f, -1.0f,  1.0f), // 5
                glm::vec3(1.0f, -1.0f,  1.0f), // 6
                glm::vec3(1.0f,  1.0f,  1.0f)  // 7
            };

            vertex_data.clear();
            for (int i = 0; i < 8; ++i) {
                Vertex v;
                v.Position = p[i];
                v.Normal = glm::vec3(0.0f);
                v.TexCoords = glm::vec2(0.0f);
                vertex_data.push_back(v);
            }

            // 从内部观察为 CCW（适用于 Skybox）
            unsigned int indices[] = {
                2, 6, 7,  2, 7, 3,   // 右
                5, 1, 0,  5, 0, 4,   // 左
                4, 0, 3,  4, 3, 7,   // 顶
                1, 5, 6,  1, 6, 2,   // 底
                1, 2, 3,  1, 3, 0,   // 后
                6, 5, 4,  6, 4, 7    // 前
            };
            index_data.assign(indices, indices + 36);
        }

        // ===========================================================
        // HudQuad —— 缠绕已正确（CCW，法线 +Z）
        // ===========================================================
        void LoadHudQuad(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            Vertex unique_vertices[] = {
                { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 0) }, // 0 BL
                { glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(1, 0) }, // 1 BR
                { glm::vec3(1.0f,  1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(1, 1) }, // 2 TR
                { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 1) }  // 3 TL
            };
            vertex_data.assign(unique_vertices, unique_vertices + 4);

            unsigned int indices[] = {
                0, 1, 2,
                0, 2, 3
            };
            index_data.assign(indices, indices + 6);
        }

        // ===========================================================
        // GbufferQuad —— 【修复】反转缠绕至 CCW
        //   原索引 (0,2,1 / 1,2,3) 使法线指向 -Z，开启背面剔除时全屏 Quad 被剔除
        // ===========================================================
        void LoadGbufferQuad(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
        {
            // 顶点布局：0=TL, 1=BL, 2=TR, 3=BR
            GLfloat quadVertices[] = {
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, // 0: TL
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 1: BL
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // 2: TR
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 3: BR
            };

            vertex_data.clear();
            index_data.clear();

            glm::vec3 quadNormal(0.0f, 0.0f, 1.0f);
            glm::vec3 quadTangent(1.0f, 0.0f, 0.0f);
            glm::vec3 quadBitangent(0.0f, 1.0f, 0.0f);

            for (int i = 0; i < 4; ++i) {
                Vertex v;
                v.Position = glm::vec3(quadVertices[i * 5 + 0], quadVertices[i * 5 + 1], quadVertices[i * 5 + 2]);
                v.TexCoords = glm::vec2(quadVertices[i * 5 + 3], quadVertices[i * 5 + 4]);
                v.Normal = quadNormal;
                v.Tangent = quadTangent;
                v.Bitangent = quadBitangent;
                vertex_data.push_back(v);
            }

            // 【修复】CCW from +Z：
            // 三角1: TL(0) -> BL(1) -> TR(2)
            // 三角2: BL(1) -> BR(3) -> TR(2)
            unsigned int indices[] = {
                0, 1, 2,
                1, 3, 2
            };
            index_data.assign(indices, indices + 6);
        }
    }

	std::string ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,
		std::map<std::string, std::string>& textures, const std::string& directory)
	{
		std::string texName = "";
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			if (mat->GetTexture(type, i, &str) == AI_SUCCESS)
			{
				std::string assimpPath = str.C_Str();
				std::replace(assimpPath.begin(), assimpPath.end(), '\\', '/');

				// 1. [核心修复：强制声明 UTF-8] 获取模型目录路径
				std::filesystem::path dirPath(reinterpret_cast<const char8_t*>(directory.c_str()));
				std::string folderPrefix = reinterpret_cast<const char*>(dirPath.filename().u8string().c_str());

				// 2. [核心修复：强制声明 UTF-8] 处理 Assimp 解析出的贴图文件名
				std::filesystem::path texFile(reinterpret_cast<const char8_t*>(assimpPath.c_str()));
				std::string fileNameOnly = reinterpret_cast<const char*>(texFile.stem().u8string().c_str());

				// 3. 生成 Key
				texName = folderPrefix + "_" + fileNameOnly;

				// 4. [核心修复] 拼接完整物理路径，并无损转回 UTF-8 字符串
				std::filesystem::path fullTexPath = dirPath / texFile;
				std::string texPath = reinterpret_cast<const char*>(fullTexPath.u8string().c_str());

				if (!textures.contains(texName))
				{
					textures.emplace(texName, texPath);
				}
			}
		}
		return texName;
	}

	void ModelLoader::processNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes, std::map<std::string, std::string>& textures, const std::string& directory)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene, textures, directory));
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene, meshes, textures, directory);
		}
	}

	Mesh ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene, std::map<std::string, std::string>& textures, const std::string& directory) 
	{
		// data to fill
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::map<std::string, std::string> default_textures;

		// 遍历mesh的顶点
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
				// tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;
				// bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		// 遍历mesh索引
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			// 传递 directory 参数
			for (const auto& slot : TextureRegistry::GetSlots()) {
				std::string texName = loadMaterialTextures(material, slot.assimpType, slot.key, textures, directory);
				default_textures.emplace(slot.key, texName);
			}
		}

		// return a mesh object created from the extracted mesh data
		// return Mesh(vertices, indices, textures);
		return Mesh(vertices, indices, default_textures);
	}

	bool ModelLoader::LoadModel(const std::string& modelPath, std::vector<Mesh>& meshes, std::map<std::string, std::string>& textures)
	{
		Assimp::Importer importer;

		std::filesystem::path p(reinterpret_cast<const char8_t*>(modelPath.c_str()));
		std::string extension = p.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) {return static_cast<char>(std::tolower(c)); });

		// Assimp check
		if (!importer.IsExtensionSupported(extension))
		{
			ENGINE_ERROR("ASSIMP: extension [{0}] is not supported by current imported build.", extension);
			return false;
		}

		const aiScene* scene = importer.ReadFile(
			modelPath,
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			ENGINE_ERROR("ASSIMP: {0}", importer.GetErrorString());
			return false;
		}

		std::filesystem::path pathObj(reinterpret_cast<const char8_t*>(modelPath.c_str()));
		std::string directory = reinterpret_cast<const char*>(pathObj.parent_path().u8string().c_str());

		processNode(scene->mRootNode, scene, meshes, textures, directory);

		return true;
	}

	bool ModelLoader::LoadModelFromMemory(const std::string& shapeName, std::vector<Mesh>& meshes)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		bool found = false;

		// 使用 if-else 链条，或者 map 映射字符串到函数指针。
		// 这里为了简单，重构你的 if-else 逻辑，避免重复创建 Mesh 对象
		if (shapeName == "cube") {
			Primitives::LoadCube(vertices, indices);
			found = true;
		}
		else if (shapeName == "sphere") {
			Primitives::LoadSphere(vertices, indices);
			found = true;
		}
		else if (shapeName == "capsule") {
			Primitives::LoadCapsule(vertices, indices);
			found = true;
		}
		else if (shapeName == "plane") {
			Primitives::LoadPlane(vertices, indices);
			found = true;
		}
		else if (shapeName == "skybox")
		{
			Primitives::LoadSkybox(vertices, indices);
			found = true;
		}
		else if (shapeName == "hud_quad") {
			Primitives::LoadHudQuad(vertices, indices);
			found = true;
		}
		else if (shapeName == "quad") {
			Primitives::LoadGbufferQuad(vertices, indices);
			found = true;
		}

		if (found && !vertices.empty()) {
			// 修复：补充第三个参数（空的 default_texture），匹配 Mesh 构造函数
			std::map<std::string, std::string> empty_tex;
			meshes.emplace_back(std::move(vertices), std::move(indices), std::move(empty_tex));
			return true;
		}

		return false;
	}

	std::shared_ptr<Model> ModelLoader::CreateModel(const std::string& modelPath, std::map<std::string, std::string>& textures) {
		std::vector<Mesh> meshes;
		if (LoadModel(modelPath, meshes, textures)) {
			return std::make_shared<Model>(std::move(meshes));
		}
		return nullptr;
	}

	std::shared_ptr<Model> ModelLoader::CreateModelFromMemory(const std::string& shapeName) {
		std::vector<Mesh> meshes;
		if (LoadModelFromMemory(shapeName, meshes)) {
			// 【关键】使用 std::move，因为 meshes vector 现在拥有 Mesh 对象，而 Mesh 对象不可拷贝
			return std::make_shared<Model>(std::move(meshes));
		}
		// 可以加一个 Log 警告：Shape not found
		return nullptr;
	}
}