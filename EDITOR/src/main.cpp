#define SDL_MAIN_HANDLED 1;
#include <Windowing//Window/Window.h>
#include<SDL.h>
#include<glad/glad.h>
#include<iostream>

int main()
{
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

	SDL_Event event{};

	// vertex data 
	float vertices[] =
	{
		0.0f,0.5f,0.0f,
		-0.5f,-0.5f,0.0f,
		0.5f,-0.5f,0.0f
	};
	
	// vertex source
	const char* vertexSource =
		"#version 450 core\n"
		"layout (location = 0) in vec3 aPosition;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(aPosition, 1.0);\n"
		"}\0";
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);		// shader
	glShaderSource(vertexShader, 1, &vertexSource, NULL);		// add the vertex shader source
	glCompileShader(vertexShader);								// compile the vertex shader
	int status;													// check
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status) 
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "Failed to compile vertex shader!\n" << infoLog << std::endl;
		return -1;
	}

	// fragment source
	const char* fragmentSource =
		"#version 450 core\n"
		"out vec4 color;\n"
		"void main()\n"
		"{\n"
		"	color = vec4(1.0f,1.0f,1.0f,1.0f);\n"
		"}\0";
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);		
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);															
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "Failed to compile fragment shader!\n" << infoLog << std::endl;
		return -1;
	}

	// Create the shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (!status)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "Failed to link shader program!\n" << infoLog << std::endl;
		return -1;
	}

	glUseProgram(shaderProgram);
	// can delete the shader after link
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// VAO VBO
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(
		GL_ARRAY_BUFFER,		// The target buffer type
		sizeof(vertices),		// Size of the data
		vertices,				// A pointer to the data
		GL_STATIC_DRAW			// The expected usage pattern of the data store
	);
	glVertexAttribPointer(
		0,						// Attribute 0
		3,						// Size of a attribute0/vertex component
		GL_FLOAT,				// type
		GL_FALSE,				// should do normalize?
		3 * sizeof(float),		// stride
		(void*)0				// offset
	);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

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

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		SDL_GL_SwapWindow(window.GetWindow().get());
	}

	std::cout << "Closing!" << std::endl;
	return 0;
}