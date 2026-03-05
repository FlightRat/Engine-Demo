#include "MeshLoader.h"

namespace ENGINE_RENDERING {
	namespace Primitives {
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
				v.pos_ = positions[i];
				v.normal_ = normals[i];
				v.uv_ = uv[i];
				vertex_data.push_back(v);
			}

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
				v.pos_ = positions[i];
				v.normal_ = normals[i];
				v.uv_ = uv[i];
				vertex_data.push_back(v);
			}

			// 拷贝索引数据
			index_data = indices;
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

			// 渲染注意事项：
			// 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
	}


	bool MeshLoader::LoadMesh(const std::string& meshPath, std::vector <Vertex>& vertices, std::vector <unsigned int>& indices)
	{
		// TODO: load mesh with Assimp here
		return false;
	}

	bool MeshLoader::LoadMeshFromMemory(const std::string& shapeName, std::vector <Vertex>& vertices, std::vector <unsigned int>& indices)
	{
		vertices.clear();
		indices.clear();

		if (shapeName == "cube") {
			Primitives::LoadCube(vertices, indices);
			return true;
		}
		else if (shapeName == "sphere") {
			Primitives::LoadSphere(vertices, indices);
			return true;
		}
		else if (shapeName == "capsule") {
			Primitives::LoadCapsule(vertices, indices);
			return true;
		}
		else if (shapeName == "plane") {
			Primitives::LoadPlane(vertices, indices);
			return true;
		}
		else if (shapeName == "hud_quad") {
			Primitives::LoadHudQuad(vertices, indices);
			return true;
		}

		// 没找到对应的形状
		return false;
	}

	std::shared_ptr<Mesh> MeshLoader::Create(const std::string& meshPath)
	{
		std::vector <Vertex> vertices;
		std::vector <unsigned int> indices;
		if (LoadMesh(meshPath, vertices, indices))
		{
			return std::make_shared<Mesh>(vertices, indices);
		}
		return nullptr;
	}

	std::shared_ptr<Mesh> MeshLoader::CreateFromMemory(const std::string& shapeName)
	{
		std::vector <Vertex> vertices;
		std::vector <unsigned int> indices;
		if (LoadMeshFromMemory(shapeName, vertices, indices))
		{
			return std::make_shared<Mesh>(vertices, indices);
		}
		return nullptr;
	}
}