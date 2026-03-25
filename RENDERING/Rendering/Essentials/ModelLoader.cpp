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

				// 计算行列式
				float determinant = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

				// 【新增安全检查】：防止 UV 退化或未分配导致除以 0 产生 NaN
				float f = (determinant == 0.0f) ? 0.0f : 1.0f / determinant;

				glm::vec3 tangent;
				tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
				tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
				tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

				glm::vec3 bitangent;
				bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
				bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
				bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

				// 累加到顶点（如果是平滑表面，共享顶点会得到加权平均值）
				v1.Tangent += tangent;
				v2.Tangent += tangent;
				v3.Tangent += tangent;

				v1.Bitangent += bitangent;
				v2.Bitangent += bitangent;
				v3.Bitangent += bitangent;
			}

			// 3. 【核心修复】：遍历所有顶点，进行 Gram-Schmidt 正交化与归一化
			for (auto& v : vertices) {
				const glm::vec3& n = v.Normal;
				const glm::vec3& t = v.Tangent;
				const glm::vec3& b = v.Bitangent;

				// 避免极小向量导致归一化失败
				if (glm::length(t) > 0.0001f && glm::length(n) > 0.0001f) {
					// Gram-Schmidt 正交化：T' = T - (T · N) * N
					// 强制让切线与法线保持绝对垂直
					v.Tangent = glm::normalize(t - n * glm::dot(n, t));
				}

				if (glm::length(b) > 0.0001f && glm::length(n) > 0.0001f) {
					// 同样处理副切线，使其与法线绝对垂直
					glm::vec3 orthogonalized_b = b - n * glm::dot(n, b);

					// 进一步让副切线也与切线垂直，构成完美的 TBN 正交基
					orthogonalized_b = orthogonalized_b - v.Tangent * glm::dot(v.Tangent, orthogonalized_b);

					v.Bitangent = glm::normalize(orthogonalized_b);
				}
			}
		}

		void LoadCube(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{
			// 定义唯一的顶点 (8个角)
			// 格式：Position (3 floats) | Normal (3 floats) | UV (2 floats)
			// 注意：立方体需要14个独特的顶点才能正确处理法线和UV缝隙，但为了简单起见，我们先用8个角作为基础。
			// *真正的* 渲染立方体通常需要 24 个顶点 (6 面 * 4 顶点/面) 或 36 个顶点 (6 面 * 6 顶点/面, 非索引)
			// 我们将采用 24 个独特的顶点 + 索引的方案，确保正确的法线和 UV。

			// ----------------------------------------------------------------------
			// 立方体的 8 个物理角坐标
			glm::vec3 p0(-1.0f, -1.0f, 1.0f); // 前下左
			glm::vec3 p1(1.0f, -1.0f, 1.0f); // 前下右
			glm::vec3 p2(1.0f, 1.0f, 1.0f); // 前上右
			glm::vec3 p3(-1.0f, 1.0f, 1.0f); // 前上左
			glm::vec3 p4(-1.0f, -1.0f, -1.0f); // 后下左
			glm::vec3 p5(1.0f, -1.0f, -1.0f); // 后下右
			glm::vec3 p6(1.0f, 1.0f, -1.0f); // 后上右
			glm::vec3 p7(-1.0f, 1.0f, -1.0f); // 后上左
			// ----------------------------------------------------------------------

			// ** 优化：为每个面创建独立的顶点，以确保正确的法线和UV **

			// Front face (index: 0-3, Normal: Z+)
			vertex_data.push_back({ p0, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }); // 0
			vertex_data.push_back({ p1, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }); // 1
			vertex_data.push_back({ p2, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }); // 2
			vertex_data.push_back({ p3, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} }); // 3

			// Back face (index: 4-7, Normal: Z-)
			vertex_data.push_back({ p4, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} }); // 4 (UV flipped for example)
			vertex_data.push_back({ p5, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} }); // 5
			vertex_data.push_back({ p6, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} }); // 6
			vertex_data.push_back({ p7, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} }); // 7

			// Right face (index: 8-11, Normal: X+)
			vertex_data.push_back({ p1, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }); // 8 (p1)
			vertex_data.push_back({ p5, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }); // 9 (p5)
			vertex_data.push_back({ p6, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }); // 10 (p6)
			vertex_data.push_back({ p2, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }); // 11 (p2)

			// Left face (index: 12-15, Normal: X-)
			vertex_data.push_back({ p4, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }); // 12 (p4)
			vertex_data.push_back({ p0, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }); // 13 (p0)
			vertex_data.push_back({ p3, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }); // 14 (p3)
			vertex_data.push_back({ p7, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }); // 15 (p7)

			// Top face (index: 16-19, Normal: Y+)
			vertex_data.push_back({ p3, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} }); // 16 (p3)
			vertex_data.push_back({ p2, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} }); // 17 (p2)
			vertex_data.push_back({ p6, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }); // 18 (p6)
			vertex_data.push_back({ p7, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }); // 19 (p7)

			// Bottom face (index: 20-23, Normal: Y-)
			vertex_data.push_back({ p4, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} }); // 20 (p4)
			vertex_data.push_back({ p5, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} }); // 21 (p5)
			vertex_data.push_back({ p1, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }); // 22 (p1)
			vertex_data.push_back({ p0, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }); // 23 (p0)

			// 3. 定义索引 (每 4 个顶点构成一个四边形，用 2 个三角形表示)

			unsigned int indices[] = {
				// Front (0, 1, 2, 3)
				0, 1, 2,  0, 2, 3,
				// Back (4, 5, 6, 7)
				4, 5, 6,  4, 6, 7,
				// Right (8, 9, 10, 11)
				8, 9, 10, 8, 10, 11,
				// Left (12, 13, 14, 15)
				12, 13, 14, 12, 14, 15,
				// Top (16, 17, 18, 19)
				16, 17, 18, 16, 18, 19,
				// Bottom (20, 21, 22, 23)
				20, 21, 22, 20, 22, 23
			};

			// 4. 将索引数据拷贝到 vector
			size_t index_count = sizeof(indices) / sizeof(unsigned int);
			index_data.assign(indices, indices + index_count);

			CalculateTangents(vertex_data, index_data);
		}

		void LoadSphere(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{
			std::vector<glm::vec3> positions;
			std::vector<glm::vec2> uv;
			std::vector<glm::vec3> normals;

			const unsigned int X_SEGMENTS = 64; // 经线/扇区数量
			const unsigned int Y_SEGMENTS = 64; // 纬线/堆栈数量
			const float PI = 3.14159265359f;

			// -----------------------------------------------------------
			// 2. 生成唯一的顶点、法线和纹理坐标 (与你现有的代码相同)
			// -----------------------------------------------------------
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					float xSegment = (float)x / (float)X_SEGMENTS;
					float ySegment = (float)y / (float)Y_SEGMENTS;

					// 球坐标参数化 (r=1.0)
					float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
					float yPos = std::cos(ySegment * PI);
					float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

					positions.push_back(glm::vec3(xPos, yPos, zPos));
					uv.push_back(glm::vec2(xSegment, ySegment));
					normals.push_back(glm::vec3(xPos, yPos, zPos));
				}
			}

			// -----------------------------------------------------------
			// 3. 生成 GL_TRIANGLES 模式的索引
			// -----------------------------------------------------------
			// 每个四边形网格面需要 6 个索引 (2 个三角形)
			for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
			{
				for (unsigned int x = 0; x < X_SEGMENTS; ++x)
				{
					// 当前四边形的四个顶点索引 (P0, P1, P2, P3)
					unsigned int p0 = y * (X_SEGMENTS + 1) + x;         // 左下 (或左上，取决于极点定义)
					unsigned int p1 = y * (X_SEGMENTS + 1) + x + 1;     // 右下
					unsigned int p2 = (y + 1) * (X_SEGMENTS + 1) + x + 1; // 右上
					unsigned int p3 = (y + 1) * (X_SEGMENTS + 1) + x;     // 左上

					// 三角形 1: p0, p2, p1
					index_data.push_back(p0);
					index_data.push_back(p2);
					index_data.push_back(p1);

					// 三角形 2: p0, p3, p2
					index_data.push_back(p0);
					index_data.push_back(p3);
					index_data.push_back(p2);
				}
			}

			// -----------------------------------------------------------
			// 4. 组织最终的 Vertex 数据
			// -----------------------------------------------------------
			// 将 Pos/Normal/UV 组合成 Vertex 结构体列表 (与你现有的代码相同)
			for (size_t i = 0; i < positions.size(); ++i)
			{
				Vertex v;
				v.Position = positions[i];
				v.Normal = normals[i];
				v.TexCoords = uv[i];
				vertex_data.push_back(v);
			}

			CalculateTangents(vertex_data, index_data);
			// 渲染注意事项：
			// 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, index_data.size(), GL_UNSIGNED_INT, 0);
		}

		void LoadCapsule(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{
			const float RADIUS = 1.0f;          // 半球和圆柱体的半径
			const float HALF_HEIGHT = 1.0f;     // 圆柱体的一半高度 (总高度为 2.0)
			const unsigned int SEGMENTS = 32;   // 沿圆周和垂直方向的细分数
			const unsigned int Y_SEGMENTS_HALF = SEGMENTS / 2; // 半球的堆栈数
			const float PI = 3.14159265359f;

			std::vector<glm::vec3> positions;
			std::vector<glm::vec2> uv;
			std::vector<glm::vec3> normals;
			std::vector<unsigned int> indices;

			unsigned int base_vertex_index = 0; // 当前顶点块在最终数组中的起始索引

			// -----------------------------------------------------------
			// A. 顶部半球 (Y > 0): 从赤道 (phi=PI/2) 到北极 (phi=0)
			// -----------------------------------------------------------
			for (unsigned int y = 0; y <= Y_SEGMENTS_HALF; ++y)
			{
				for (unsigned int x = 0; x <= SEGMENTS; ++x)
				{
					float xSegment = (float)x / (float)SEGMENTS;
					float ySegment = (float)y / (float)SEGMENTS;

					float phi = (0.5f - ySegment) * PI; // 角度从 PI/2 减小到 0

					float xPos_s = RADIUS * std::cos(xSegment * 2.0f * PI) * std::sin(phi);
					float zPos_s = RADIUS * std::sin(xSegment * 2.0f * PI) * std::sin(phi);
					float yPos_s = RADIUS * std::cos(phi);

					// 向上平移 + HALF_HEIGHT
					glm::vec3 pos = glm::vec3(xPos_s, yPos_s + HALF_HEIGHT, zPos_s);
					// 法线相对于半球中心 (0, HALF_HEIGHT, 0)
					glm::vec3 normal = glm::normalize(pos - glm::vec3(0.0f, HALF_HEIGHT, 0.0f));

					positions.push_back(pos);
					normals.push_back(normal);
					uv.push_back(glm::vec2(xSegment, ySegment * 2.0f));
				}
			}

			// 索引生成 (顶部半球)
			for (unsigned int y = 0; y < Y_SEGMENTS_HALF; ++y)
			{
				for (unsigned int x = 0; x < SEGMENTS; ++x)
				{
					unsigned int p0 = y * (SEGMENTS + 1) + x;
					unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
					unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
					unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p2);
					indices.push_back(base_vertex_index + p1);

					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p3);
					indices.push_back(base_vertex_index + p2);
				}
			}

			// 更新基础顶点索引到圆柱体起点
			base_vertex_index = positions.size();


			// -----------------------------------------------------------
			// B. 中间圆柱体 (Y 范围: -HALF_HEIGHT 到 HALF_HEIGHT)
			// -----------------------------------------------------------
			const unsigned int H_SEGMENTS = 1; // 圆柱体只需上下两层顶点

			for (unsigned int y = 0; y <= H_SEGMENTS; ++y)
			{
				float y_normalized = (float)y / (float)H_SEGMENTS; // 0.0 或 1.0

				for (unsigned int x = 0; x <= SEGMENTS; ++x)
				{
					float xSegment = (float)x / (float)SEGMENTS;

					// XZ 平面上的圆周
					float xPos = RADIUS * std::cos(xSegment * 2.0f * PI);
					float zPos = RADIUS * std::sin(xSegment * 2.0f * PI);

					// Y 坐标从 -HALF_HEIGHT 到 HALF_HEIGHT
					float yPos = -HALF_HEIGHT + y_normalized * (2.0f * HALF_HEIGHT);

					positions.push_back(glm::vec3(xPos, yPos, zPos));
					normals.push_back(glm::normalize(glm::vec3(xPos, 0.0f, zPos))); // 法线指向外侧
					uv.push_back(glm::vec2(xSegment, y_normalized));
				}
			}

			// 索引生成 (中间圆柱体)
			for (unsigned int y = 0; y < H_SEGMENTS; ++y)
			{
				for (unsigned int x = 0; x < SEGMENTS; ++x)
				{
					unsigned int p0 = y * (SEGMENTS + 1) + x;
					unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
					unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
					unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p2);
					indices.push_back(base_vertex_index + p1);

					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p3);
					indices.push_back(base_vertex_index + p2);
				}
			}

			// 更新基础顶点索引到底部半球起点
			base_vertex_index = positions.size();


			// -----------------------------------------------------------
			// C. 底部半球 (Y < 0): 从赤道 (phi=0) 到南极 (phi=PI/2)
			// -----------------------------------------------------------
			// 我们只需要 Y_SEGMENTS_HALF 数量的堆栈，从圆柱体底部往下延伸
			for (unsigned int y = 0; y <= Y_SEGMENTS_HALF; ++y) // y 从 0 开始，生成 Y_SEGMENTS_HALF + 1 层
			{
				for (unsigned int x = 0; x <= SEGMENTS; ++x)
				{
					float xSegment = (float)x / (float)SEGMENTS;
					float ySegment_rel = (float)y / (float)Y_SEGMENTS_HALF; // 相对归一化 [0.0, 1.0]

					// 角度 phi: 从 0 (赤道) 增加到 PI/2 (南极)
					float phi = ySegment_rel * PI / 2.0f;

					float xPos_s = RADIUS * std::cos(xSegment * 2.0f * PI) * std::sin(phi);
					float zPos_s = RADIUS * std::sin(xSegment * 2.0f * PI) * std::sin(phi);
					float yPos_s = -RADIUS * std::cos(phi); // Y 分量为负

					// 向下平移 - HALF_HEIGHT
					glm::vec3 pos = glm::vec3(xPos_s, yPos_s - HALF_HEIGHT, zPos_s);
					// 法线相对于半球中心 (0, -HALF_HEIGHT, 0)
					glm::vec3 normal = glm::normalize(pos - glm::vec3(0.0f, -HALF_HEIGHT, 0.0f));

					positions.push_back(pos);
					normals.push_back(normal);
					// 底部 UV 从 1.0 递减到 0.0
					uv.push_back(glm::vec2(xSegment, 1.0f - ySegment_rel));
				}
			}

			// 索引生成 (底部半球)
			for (unsigned int y = 0; y < Y_SEGMENTS_HALF; ++y)
			{
				for (unsigned int x = 0; x < SEGMENTS; ++x)
				{
					// 注意：p0, p1, p2, p3 是相对于当前顶点块 (base_vertex_index) 的相对索引
					unsigned int p0 = y * (SEGMENTS + 1) + x;
					unsigned int p1 = y * (SEGMENTS + 1) + x + 1;
					unsigned int p2 = (y + 1) * (SEGMENTS + 1) + x + 1;
					unsigned int p3 = (y + 1) * (SEGMENTS + 1) + x;

					// 三角形 1: p0, p2, p1
					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p2);
					indices.push_back(base_vertex_index + p1);

					// 三角形 2: p0, p3, p2
					indices.push_back(base_vertex_index + p0);
					indices.push_back(base_vertex_index + p3);
					indices.push_back(base_vertex_index + p2);
				}
			}


			// -----------------------------------------------------------
			// D. 组织最终的 Vertex 数据
			// -----------------------------------------------------------
			for (size_t i = 0; i < positions.size(); ++i)
			{
				Vertex v;
				v.Position = positions[i];
				v.Normal = normals[i];
				v.TexCoords = uv[i];
				vertex_data.push_back(v);
			}

			// 拷贝索引数据
			index_data = indices;

			CalculateTangents(vertex_data, index_data);
		}

		void LoadPlane(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data) {
			// 定义 Plane 的 4 个唯一的角顶点
			// Plane 位于 XZ 平面 (Y=0)，法线指向 Y+ (0, 1, 0)
			// 注意：原始代码使用了 ±10.0 的坐标和纹理坐标，这通常用于平铺 (Tiling) 大地块。

			// A. 顶点数据 (4 个唯一的顶点)
			Vertex unique_vertices[] = {
				// Pos(X, Y, Z) | Normal(0, 1, 0) | UV(U, V)

				// 0: 右上 (10, 0, 10)
				{ glm::vec3(10.0f, 0.0f,  10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(10.0f, 0.0f) },

				// 1: 左上 (-10, 0, 10)
				{ glm::vec3(-10.0f, 0.0f,  10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },

				// 2: 左下 (-10, 0, -10)
				{ glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 10.0f) },

				// 3: 右下 (10, 0, -10)
				{ glm::vec3(10.0f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(10.0f, 10.0f) }
			};

			// 将唯一的 4 个顶点添加到组件的 vector 中
			vertex_data.assign(unique_vertices, unique_vertices + 4);


			// 3. 定义索引数据 (6 个索引)
			// Quad 由两个三角形组成：
			// 三角形 1: 0, 2, 1 (右上, 左下, 左上)
			// 三角形 2: 0, 3, 2 (右上, 右下, 左下)
			unsigned int indices[] = {
				0, 2, 1,  // 第一个三角形 (确保缠绕顺序正确)
				0, 3, 2   // 第二个三角形
			};

			// 将索引数据添加到组件的 vector 中
			index_data.assign(indices, indices + 6);

			CalculateTangents(vertex_data, index_data);
			// 渲染注意事项：
			// 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		void LoadSkybox(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{
			// 1. 定义立方体的 8 个唯一角顶点 (范围从 -1 到 1)
			// 对于天空盒，法线和 UV 通常不需要，因为着色器会直接使用 Position 进行立方体贴图采样。
			glm::vec3 p[] = {
				glm::vec3(-1.0f,  1.0f, -1.0f), // 0: 左上后
				glm::vec3(-1.0f, -1.0f, -1.0f), // 1: 左下后
				glm::vec3(1.0f, -1.0f, -1.0f), // 2: 右下后
				glm::vec3(1.0f,  1.0f, -1.0f), // 3: 右上后
				glm::vec3(-1.0f,  1.0f,  1.0f), // 4: 左上前
				glm::vec3(-1.0f, -1.0f,  1.0f), // 5: 左下前
				glm::vec3(1.0f, -1.0f,  1.0f), // 6: 右下前
				glm::vec3(1.0f,  1.0f,  1.0f)  // 7: 右上前
			};

			// 填充顶点数据
			vertex_data.clear();
			for (int i = 0; i < 8; ++i) {
				Vertex v;
				v.Position = p[i];
				v.Normal = glm::vec3(0.0f); // 天空盒通常不计算光照
				v.TexCoords = glm::vec2(0.0f); // 天空盒使用 vec3 采样，此处可留空
				vertex_data.push_back(v);
			}

			// 2. 定义索引数据 (36个索引，构成12个三角形)
			// 注意：这里的缠绕顺序是按照“从立方体内部向外看”为正面进行定义的（通常为逆时针 CCW）
			unsigned int indices[] = {
				// 右面 (Right)
				2, 6, 7,  2, 7, 3,
				// 左面 (Left)
				5, 1, 0,  5, 0, 4,
				// 顶面 (Top)
				4, 0, 3,  4, 3, 7,
				// 底面 (Bottom)
				1, 5, 6,  1, 6, 2,
				// 后面 (Back)
				1, 2, 3,  1, 3, 0,
				// 前面 (Front)
				6, 5, 4,  6, 4, 7
			};

			index_data.assign(indices, indices + 36);
		}

		void LoadHudQuad(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{

			// 定义 HUD Quad 的 4 个唯一的角顶点
			// 顶点格式：Pos(3) | Normal(3) | TexCoord(2)
			// HUD Quad 位于 XY 平面 (Z=0)，法线指向 Z+ (0, 0, 1)

			// A. 顶点数据 (4 个唯一的顶点)
			Vertex unique_vertices[] = {
				// Pos(-1, -1, 0) | Normal(0, 0, 1) | UV(0, 0)
				{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) }, // 0: bottom-left

				// Pos( 1, -1, 0) | Normal(0, 0, 1) | UV(1, 0)
				{ glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) }, // 1: bottom-right

				// Pos( 1,  1, 0) | Normal(0, 0, 1) | UV(1, 1)
				{ glm::vec3(1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) }, // 2: top-right

				// Pos(-1,  1, 0) | Normal(0, 0, 1) | UV(0, 1)
				{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) }  // 3: top-left
			};

			// 将唯一的 4 个顶点添加到组件的 vector 中
			vertex_data.assign(unique_vertices, unique_vertices + 4);


			// 3. 定义索引数据 (6 个索引)
			// Quad 由两个三角形组成：
			// 三角形 1: 0, 1, 2 (bottom-left, bottom-right, top-right)
			// 三角形 2: 0, 2, 3 (bottom-left, top-right, top-left)
			unsigned int indices[] = {
				0, 1, 2,  // 第一个三角形
				0, 2, 3   // 第二个三角形
			};

			// 将索引数据添加到组件的 vector 中
			index_data.assign(indices, indices + 6);

			// 渲染注意事项：
			// 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		
		void LoadGbufferQuad(std::vector<Vertex>& vertex_data, std::vector<unsigned int>& index_data)
		{
			// 原始给定的数据：位置 (X, Y, Z) 和 UV (U, V)
			GLfloat quadVertices[] = {
				// Positions        // Texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f, // 0: 左上 (Top-Left)
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 1: 左下 (Bottom-Left)
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // 2: 右上 (Top-Right)
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 3: 右下 (Bottom-Right)
			};

			// 清空容器，防止追加到旧数据上
			vertex_data.clear();
			index_data.clear();

			// 预设全屏 Quad 的正交基（虽然屏幕空间着色器大概率用不到它们，但需填补数据结构）
			glm::vec3 quadNormal(0.0f, 0.0f, 1.0f);    // 面朝屏幕外 Z+
			glm::vec3 quadTangent(1.0f, 0.0f, 0.0f);   // U轴方向 X+
			glm::vec3 quadBitangent(0.0f, 1.0f, 0.0f); // V轴方向 Y+

			// 1. 组装顶点数据
			for (int i = 0; i < 4; ++i) {
				Vertex v;

				// 从数组提取 Position
				v.Position = glm::vec3(
					quadVertices[i * 5 + 0],
					quadVertices[i * 5 + 1],
					quadVertices[i * 5 + 2]
				);

				// 从数组提取 UV
				v.TexCoords = glm::vec2(
					quadVertices[i * 5 + 3],
					quadVertices[i * 5 + 4]
				);

				// 填充剩余的属性以满足 VAO 步长要求
				v.Normal = quadNormal;
				v.Tangent = quadTangent;
				v.Bitangent = quadBitangent;

				vertex_data.push_back(v);
			}

			// 2. 组装索引数据 (构成两个三角形)
			// 使用逆时针 (CCW) 缠绕顺序，确保开启背面剔除时不会被过滤掉
			unsigned int indices[] = {
				0, 1, 2,  // 三角形 1: 左上 -> 左下 -> 右上
				1, 3, 2   // 三角形 2: 左下 -> 右下 -> 右上
			};

			index_data.assign(indices, indices + 6);
		}
	}

	std::string ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,
		std::map<std::string, std::string>& textures, const std::string& directory)
	{
		std::string texName="";	// Note: assume that there is only 1 texture for each type
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			if (mat->GetTexture(type, i, &str) == AI_SUCCESS)
			{
				// 1. 获取 Assimp 原始路径并格式化
				std::string assimpPath = str.C_Str();
				std::replace(assimpPath.begin(), assimpPath.end(), '\\', '/');

				// 2. 提取文件夹前缀 (例如: "nanosuit")
				std::filesystem::path dirPath(directory);
				std::string folderPrefix = dirPath.filename().string();

				// 3. 提取贴图文件名不含后缀 (例如: "glass_dif")
				std::filesystem::path texFile(assimpPath);
				std::string fileNameOnly = texFile.stem().string();

				// 4. 生成你要求的 Key (例如: "nanosuit_glass_dif")
				texName = folderPrefix + "_" + fileNameOnly;

				// 5. 生成物理完整路径用于加载文件
				std::string texPath = (dirPath / texFile).generic_string();

				// 6. 存入 Map
				if (!textures.contains(texName))
				{
					textures.emplace(texName, texPath);
					//ENGINE_LOG("ModelLoader: Generated Key [{0}] for path [{1}]", texName, texPath);
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
		const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			ENGINE_ERROR("ASSIMP: {0}", importer.GetErrorString());
			return false;
		}

		// 提取模型所在目录 (例如: "Assets/Models/Hero.obj" -> "Assets/Models")
		std::string directory = modelPath.substr(0, modelPath.find_last_of("\\/"));

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
		else if (shapeName == "gbuffer_quad") {
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