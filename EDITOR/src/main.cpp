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

class Camera2D
{
private:
	int m_Width, m_Height;
	float m_Scale;
	glm::vec2 m_Positon;
	glm::mat4 m_CameraMatrix, m_OrthoProjection;
	bool m_bNeedsUpdate;
public:
	Camera2D():Camera2D(640,480){}
	Camera2D(int width, int height) :m_Width{ width }, m_Height{ height },
		m_Scale(1.f), m_Positon{ glm::vec2{0} },
		m_CameraMatrix{1.f},m_OrthoProjection{1.f},m_bNeedsUpdate{true}
	{
		// Init ortho projection
		m_OrthoProjection = glm::ortho(0.f, static_cast<float>(m_Width), 0.f, static_cast<float>(m_Height), -1.f, 1.f);
		//m_OrthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	}

	inline glm::mat4 GetCameraMatrix() { return m_CameraMatrix; }
	inline void SetScale(float scale) { m_Scale = scale; m_bNeedsUpdate = true; }

	void Update()
	{
		if (!m_bNeedsUpdate)
			return;

		// Translate
		glm::vec3 translate{ 0.f, 0.f, 0.f };
		m_CameraMatrix = glm::translate(m_OrthoProjection, translate);

		// Scale
		glm::vec3 scale{ m_Scale, m_Scale, 0.f };
		m_CameraMatrix *= glm::scale(glm::mat4(1.f), scale);

		m_bNeedsUpdate = false;
	}

};

bool LoadTexture(const std::string& filePath, int& width, int& height, bool blended)
{
	int channels = 0;
	unsigned char* image = SOIL_load_image(filePath.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	// check
	if(!image)
	{
		std::cout << "SOIL failed to load image [" << filePath << "] -- " << SOIL_last_result() << std::endl;
		return false;
	}
	
	GLint format = GL_RGBA;
	switch (channels)
	{
	case 3:format = GL_RGB; break;
	case 4:format = GL_RGBA; break;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (!blended)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexImage2D(
		GL_TEXTURE_2D,		// target texture
		0,					// level of mipmp
		format,				// number of color components
		width, height,
		0,					// border
		format,				// formet of the pixel data
		GL_UNSIGNED_BYTE,
		image				// data
	);

	// delete the image data
	SOIL_free_image_data(image);
	
	return true;
}

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
		std::cout << "Failed to loadGL --> GLAD" << std::endl;
		running = false;
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event{};

	// texture data
	GLuint tex0;
	glGenTextures(1, &tex0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	int width{ 0 }, height{ 0 };
	if (!LoadTexture("assests/textures/mafuyu.png", width, height, false))
	{
		ENGINE_ERROR("Failed to load the texture!");
		return -1;
	}
	//ENGINE_LOG("window siez with_{},height{}!", width, height);
	//ENGINE_WARN("window siez with_{},height{}!", width, height);

	// vertex data 
	//float vertices[] = {
	//	-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,		// top left 
	//	 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,		// top right
	//	 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,		// bottom right
	//	-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,		// bottom left
	//};
	float vertices[] = {
		160.f,	360.0f,	0.0f, 0.0f, 1.0f,		// top left 
		480.f,  360.0f,	0.0f, 1.0f, 1.0f,		// top right
		480.0f, 120.0f,	0.0f, 1.0f, 0.0f,		// bottom right
		160.0f, 120.0f,	0.0f, 0.0f, 0.0f,		// bottom left
	};
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindVertexArray(0);	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.


	// Create camera
	Camera2D camera{};

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
		glBindTexture(GL_TEXTURE_2D, tex0);
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