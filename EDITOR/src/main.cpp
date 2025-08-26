#define SDL_MAIN_HANDLED 1;
#include <Windowing//Window/Window.h>
#include<SDL.h>
#include<glad/glad.h>
#include<iostream>
#include<SOIL/SOIL.h>
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Logger/Logger.h>
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Essentials/Vertex.h>
#include<Rendering/Core/Camera2D.h>

int main()
{
	ENGINE_INIT_LOGS(true, true);

	bool running{ true };

	// Init SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::string error = SDL_GetError();
		std::cout << "Failed to initialize SDL:" << error << std::endl;
		running = false;
		return -1;
	}

	// Init OpenGL
	if (SDL_GL_LoadLibrary(NULL) != 0)
	{
		std::string error = SDL_GetError();
		std::cout << "Failed to initialize OpenGL:" << error << std::endl;
		running = false;
		return -1;
	}

	// Set the OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	
	// Set per channel bits
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	// Create the window
	WINDOWING::Window window("Test", 640, 480, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, true, SDL_WINDOW_OPENGL);

	if (!window.GetWindow())
	{
		std::cout << "Failed to create the window!" << std::endl;
		return -1;
	}

	// Create OpenGL context
	window.SetGLContext(SDL_GL_CreateContext(window.GetWindow().get()));
	if (!window.GetGLContext())
	{
		std::string error = SDL_GetError();
		std::cout << "Failed to create OpenGL context:" << error << "\n";
		running = false;
		return -1;
	}

	SDL_GL_MakeCurrent(window.GetWindow().get(), window.GetGLContext());
	SDL_GL_SetSwapInterval(1);

	//Initialze Glad
	if (gladLoadGLLoader(SDL_GL_GetProcAddress) == 0)
	{
		ENGINE_ERROR("Failed to loadGL --> GLAD");
		running = false;
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event{};

	// texture data
	auto texture = RENDERING::TextureLoader::Create(RENDERING::Texture::TextureType::PIXEL, "./assests/textures/mafuyu.png");
	if (!texture)
	{
		ENGINE_ERROR("Failed to create the texture!");
		return -1;
	}
	//ENGINE_LOG("window siez with_{},height_{}!", texture->GetWidth(), texture->GetHeight());
	//ENGINE_WARN("window siez with_{},height_{}!", texture->GetWidth(), texture->GetHeight());


	// vertex data 
	//float vertices[] = {
	//	160.f,	360.0f,	0.0f, 0.0f, 1.0f,		// top left 
	//	480.f,  360.0f,	0.0f, 1.0f, 1.0f,		// top right
	//	480.0f, 120.0f,	0.0f, 1.0f, 0.0f,		// bottom right
	//	160.0f, 120.0f,	0.0f, 0.0f, 0.0f,		// bottom left
	//};
	std::vector<RENDERING::Vertex> vertices{};
	RENDERING::Vertex vTL, vTR, vBR, vBL;
	vTL.position = glm::vec2{ 160.0f, 360.0f };
	vTL.uvs = glm::vec2{ 0.0f, 1.0f };
	vTR.position = glm::vec2{ 480.0f, 360.0f };
	vTR.uvs = glm::vec2{ 1.0f, 1.0f };
	vBR.position = glm::vec2{ 480.0f, 120.0f };
	vBR.uvs = glm::vec2{ 1.0f, 0.0f };
	vBL.position = glm::vec2{ 160.0f, 120.0f };
	vBL.uvs = glm::vec2{ 0.0f, 0.0f };
	vertices.push_back(vTL);
	vertices.push_back(vTR);
	vertices.push_back(vBR);
	vertices.push_back(vBL);
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 2,  // first Triangle
		2, 3, 0   // second Triangle
	};

	// VAO VBO EBO
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(RENDERING::Vertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RENDERING::Vertex), (void*)offsetof(RENDERING::Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RENDERING::Vertex), (void*)offsetof(RENDERING::Vertex, uvs));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RENDERING::Vertex), (void*)offsetof(RENDERING::Vertex, color));
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindVertexArray(0);	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.


	// Create camera
	RENDERING::Camera2D camera{};

	auto shader = RENDERING::ShaderLoader::Create("assests/shaders/basicShader.vert", "assests/shaders/basicShader.frag");
	if (!shader)
	{
		std::cout << "Failed to create the shader" << std::endl;
		return -1;
	}

	// Window loop
	while (running)
	{
		//process Events
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					running = false;
				break;
			default:
				break;
			}
		}
		
		glViewport(0, 0, window.GetWidth(), window.GetHeight());

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Active shader
		shader->Enable();

		glActiveTexture(GL_TEXTURE0);		// Bind texture, texture0 is actived defaultly
		glBindTexture(GL_TEXTURE_2D, texture->GetID());
		shader->SetUniformInt("texture0", 0);
		
		camera.Update();
		auto projection = camera.GetCameraMatrix();
		shader->SetUniformMat4("Projection", projection);

		glBindVertexArray(VAO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		// glBindVertexArray(0); // no need to unbind it every time 

		SDL_GL_SwapWindow(window.GetWindow().get());
		shader->Disable();
	}

	std::cout << "Closing!" << std::endl;
	return 0;
}