#define SDL_MAIN_HANDLED 1;
#include <Windowing//Window/Window.h>
#include<SDL.h>
#include<glad/glad.h>
#include<iostream>
#include<SOIL/SOIL.h>

bool LoadTexture(const std::string& filePath, int& width, int& height, bool blended)
{
	int channels = 0;
	unsigned char* image = SOIL_load_image(filePath.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	// check
	if(!image)
	{
		std::cout << "SOIL failed to load image [" << filePath << "] -- " << SOIL_last_result();
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

	// texture
	GLuint tex0;
	glGenTextures(1, &tex0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	int width{ 0 }, height{ 0 };
	if (!LoadTexture("assests/textures/mafuyu.png", width, height, false))
	{
		std::cout << "Failed to load the texture\n";
		return -1;
	}

	// vertex data 
	float vertices[] = {
		-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,		// top left 
		 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,		// top right
		 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,		// bottom right
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,		// bottom left
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 2,  // first Triangle
		2, 3, 0   // second Triangle
	};

	// vertex source
	const char* vertexSource =
		"#version 450 core\n"
		"layout (location = 0) in vec3 aPosition;\n"
		"layout (location = 1) in vec2 aTexCoord;"
		"out vec2 TexCoord;"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(aPosition, 1.0);\n"
		"	TexCoord = aTexCoord;\n"
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
		"in vec2 TexCoord;\n"
		"uniform sampler2D texture0;\n"
		"void main()\n"
		"{\n"
		"	//color = vec4(1.0f,1.0f,1.0f,1.0f);\n"
		"	vec2 flip_coord = vec2(TexCoord.x, 1.0 - TexCoord.y);\n"
		"	color = texture(texture0, flip_coord);\n"
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

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
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
		glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);
		glBindVertexArray(VAO);

		glActiveTexture(GL_TEXTURE0);		// texture0 is actived defaultly
		glBindTexture(GL_TEXTURE_2D, tex0);

		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		// glBindVertexArray(0); // no need to unbind it every time 

		SDL_GL_SwapWindow(window.GetWindow().get());
	}

	std::cout << "Closing!" << std::endl;
	return 0;
}