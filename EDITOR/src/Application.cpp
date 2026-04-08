#include "Application.h"
#include<SDL.h>
#include<SDL_opengl.h>
#include<glad/glad.h>
#include<random>
#include<iostream>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<Logger/Logger.h>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Essentials/Lights.h>
#include<Rendering/Core/Camera3D.h>
#include<entt.hpp>
#include<Core/ECS/Entity.h>
#include<Core/ECS/MainRegistry.h>
#include<Core/ECS/Components/TransformComponent.h>
#include<Core/ECS/Components/PhysicsComponent.h>
#include<Core/ECS/Components/Identification.h>
#include<Core/ECS/Components/LightComponent.h>
#include<Core/Resources/AssetManager.h>
#include<Core/Systems/ScriptingSystem.h>
#include<Core/Systems/RenderSystem.h>
#include<Core/Systems/PhysicsSystem.h>
#include<Core/Systems/LightSystem.h>
#include<Core/Inputs/InputManager.h>
#include<Core/Buffers/BufferManager.h>
#include<Core/CoreUtilities/CoreEngineData.h>
#include<Windowing/Inputs/Keyboard.h>
#include<Sounds/MusicPlayer/MusicPlayer.h>
#include<Sounds/SoundFxPlayer/SoundFxPlayer.h>
#include<Physics/ContactListener.h>
#include<Physics/ContactListener.h>
#include<imgui.h>
#include<imgui_internal.h>
#include<backends/imgui_impl_sdl2.h>
#include<backends/imgui_impl_opengl3.h>
#include"editor/displays/IDisplay.h"
#include"editor/displays/GameDisplay.h"
#include"editor/displays/SceneDisplay.h"
#include"editor/displays/LogDisplay.h"
#include"editor/displays/AssetDisplay.h"
#include"editor/displays/MenuDisplay.h"
#include"editor/displays/SceneHierarchyDisplay.h"
#include"editor/utilities/editor_textures.h"
#include"editor/utilities/ComponentDrawer.h"
#include"editor/scene/SceneManager.h"
#include"editor/scene/SceneObject.h"

namespace ENGINE_EDITOR {
	Application::Application() :m_pWindow{ nullptr }, m_Event{}, m_bIsRunning{ true }
	{

	}

	bool Application::Initialize()
	{
		ENGINE_INIT_LOGS(true, true);

		// Init SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to initialize SDL: {0}", error);
			return false;
		}

		// Init OpenGL
		if (SDL_GL_LoadLibrary(NULL) != 0)
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to initialize OpenGL: {0}", error);
			return false;
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
		SDL_DisplayMode displayMode;
		SDL_GetCurrentDisplayMode(0, &displayMode);
		m_pWindow = std::make_unique<ENGINE_WINDOWING::Window>(
			"Test",
			displayMode.w,
			displayMode.h,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, true, 
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MAXIMIZED);
		if (!m_pWindow->GetWindow())
		{
			ENGINE_ERROR("Failed to create the window!");
			return false;
		}

		// Create OpenGL context
		m_pWindow->SetGLContext(SDL_GL_CreateContext(m_pWindow->GetWindow().get()));
		if (!m_pWindow->GetGLContext())
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to create OpenGL context: {0}", error);
			return false;
		}

		SDL_GL_MakeCurrent(m_pWindow->GetWindow().get(), m_pWindow->GetGLContext());
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_GL_SetSwapInterval(1);

		//Initialze Glad
		if (gladLoadGLLoader(SDL_GL_GetProcAddress) == 0)
		{
			ENGINE_ERROR("Failed to loadGL --> GLAD");
			return false;
		}

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_STENCIL_TEST);					// 开启模板测试
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);		// 当目标像素的模板值不等于1时，通过测试
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	// 但凡没通过模板或深度测试，保留原模板值；当同时通过模板和神单独测试时，用ref替代原有模板值

		if (!InitImGui())
		{
			ENGINE_ERROR("Failed to initialize ImGui!");
			return false;
		}

		// main registry
		auto& mainRegistry = MAIN_REGISTRY();
		if (!mainRegistry.Initialize())
		{
			ENGINE_ERROR("Failed to initialize the Main Registry!");
			return false;
		}
		
		// Render System
		auto renderSystem = std::make_shared<ENGINE_CORE::Systems::RenderSystem>();
		if (!renderSystem)
		{
			ENGINE_ERROR("Failed to create the render system!");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>(renderSystem))
		{
			ENGINE_ERROR("Failed to add the render system to the registry context!");
			return false;
		}

		// Light System  do this before LoadBuffers!!!
		auto lightSystem = std::make_shared<ENGINE_CORE::Systems::LightSystem>(4, 4);
		if (!lightSystem)
		{
			ENGINE_ERROR("Failed to create the light system!");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>(lightSystem))
		{
			ENGINE_ERROR("Failed to add the light system to the registry context!");
			return false;
		}

		if (!CreateDisplays())
		{
			ENGINE_ERROR("Failed to create displays!");
			return false;
		}
		if (!LoadEditorMeshes())
		{
			ENGINE_ERROR("Failed to load the default meshes!");
			return false;
		}
		if (!LoadEditorShaders())
		{
			ENGINE_ERROR("Failed to load the shaders!");
			return false;
		}
		if (!LoadEditorTextures())
		{
			ENGINE_ERROR("Failed to load the editor textures!");
			return false;
		}
		if (!LoadEditorBuffers())
		{
			ENGINE_ERROR("Failed to load buffers!");
			return false;
		}
		PrepareIBL();

		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::TransformComponent>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::PhysicsComponent>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::MeshFilter>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::MeshRender>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::Identification>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::LightComponent>();

		SCENE_MANAGER().AddScene("scene1");
		SCENE_MANAGER().AddScene("scene2");
		//SCENE_MANAGER().SetCurrentScene("scene1");

		return true;
	}

	bool Application::LoadEditorShaders()
	{
		//auto& assetManager = m_pRegistry->GetContext<std::shared_ptr<ENGINE_CORE::RESOURCES::AssetManager>>();
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		// forward BlinnPhong
		if (!assetManager.AddShader("forward_BlinnPhong", "assets/shaders/forward_BlinnPhong.vert", "assets/shaders/forward_BlinnPhong.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// forward color
		if (!assetManager.AddShader("forward_Color", "assets/shaders/forward_Color.vert", "assets/shaders/forward_Color.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// skybox shader TODO: hide shyboxShader from UI
		if (!assetManager.AddShader("skybox", "assets/shaders/skybox.vert", "assets/shaders/skybox.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// physicsDebug
		if (!assetManager.AddShader("physics_Debug", "assets/shaders/physics_Debug.vert", "assets/shaders/physics_Debug.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// shadowmap
		if(!assetManager.AddShader("shadow_Map", "assets/shaders/shadow_Map.vert", "assets/shaders/shadow_Map.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// shadowCubemap
		if (!assetManager.AddShader("shadow_Cubemap", "assets/shaders/shadow_Cubemap.vert", "assets/shaders/shadow_Cubemap.frag", "assets/shaders/shadow_Cubemap.geom"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// defer Gbuffer
		if (!assetManager.AddShader("defer_Gbuffer", "assets/shaders/defer_Gbuffer.vert", "assets/shaders/defer_Gbuffer.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// defer Lighting
		if (!assetManager.AddShader("defer_Lighting", "assets/shaders/defer_Lighting.vert", "assets/shaders/defer_Lighting.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// defer ssao
		if (!assetManager.AddShader("defer_SSAO", "assets/shaders/defer_ssao.vert", "assets/shaders/defer_ssao.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// defer ssao blur
		if (!assetManager.AddShader("defer_SSAOBlur", "assets/shaders/defer_ssaoBlur.vert", "assets/shaders/defer_ssaoBlur.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// IBL proj shader
		if (!assetManager.AddShader("ibl_proj", "assets/shaders/ibl_cubemap.vert", "assets/shaders/ibl_proj.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// IBL conv shader
		if (!assetManager.AddShader("ibl_conv", "assets/shaders/ibl_cubemap.vert", "assets/shaders/ibl_conv.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// IBL prefilter shader
		if (!assetManager.AddShader("ibl_prefilter", "assets/shaders/ibl_cubemap.vert", "assets/shaders/ibl_prefilter.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// IBL brdf shader
		if (!assetManager.AddShader("ibl_brdf", "assets/shaders/ibl_brdf.vert", "assets/shaders/ibl_brdf.frag", ""))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		assetManager.GetShader("forward_BlinnPhong")->SetIsEditorShader(true);
		assetManager.GetShader("forward_Color")->SetIsEditorShader(true);
		assetManager.GetShader("defer_Gbuffer")->SetIsEditorShader(true);
		assetManager.GetShader("defer_Lighting")->SetIsEditorShader(true);
		assetManager.GetShader("defer_SSAO")->SetIsEditorShader(true);
		assetManager.GetShader("defer_SSAOBlur")->SetIsEditorShader(true);
		assetManager.GetShader("shadow_Map")->SetIsEditorShader(true);
		assetManager.GetShader("shadow_Cubemap")->SetIsEditorShader(true);
		assetManager.GetShader("skybox")->SetIsEditorShader(true);
		assetManager.GetShader("physics_Debug")->SetIsEditorShader(true);
		assetManager.GetShader("ibl_proj")->SetIsEditorShader(true);
		assetManager.GetShader("ibl_conv")->SetIsEditorShader(true);
		assetManager.GetShader("ibl_prefilter")->SetIsEditorShader(true);
		assetManager.GetShader("ibl_brdf")->SetIsEditorShader(true);
		return true;
	}

	bool Application::LoadEditorTextures()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		if (!assetManager.AddTextureFromMemory("play_button", play_button, sizeof(play_button) / sizeof(play_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [play_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("stop_button", stop_button, sizeof(stop_button) / sizeof(stop_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [stop_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("translate_button", translate_button, sizeof(translate_button) / sizeof(translate_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [translate_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("rotate_button", rotate_button, sizeof(rotate_button) / sizeof(rotate_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [rotate_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("scale_button", scale_button, sizeof(scale_button) / sizeof(scale_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [scale_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("none_button", none_button, sizeof(none_button) / sizeof(none_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [stop_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("music_icon", music_icon, sizeof(music_icon) / sizeof(music_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [music_icon] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("scene_icon", scene_icon, sizeof(scene_icon) / sizeof(scene_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [scene_icon] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("model_icon", model_icon, sizeof(model_icon) / sizeof(model_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [model_icon] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("shader_icon", shader_icon, sizeof(shader_icon) / sizeof(shader_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [shader_icon] from memory!");
			return false;
		}
		if (!assetManager.AddSkyboxTexture("skybox", "assets/textures/skybox", false))
		{
			ENGINE_ERROR("Failed to load texture [skybox] from memory!");
			return false;
		}
		if (!assetManager.AddHDRTexture("HDR", "assets/textures/hdr/newport_loft.hdr"))
		{
			ENGINE_ERROR("Failed to load texture [HDR] from memory!");
			return false;
		}
		if (!assetManager.AddNoiseTexture("ssaoNoise"))
		{
			ENGINE_ERROR("Failed to load texture [ssaoNoise] from memory!");
			return false;
		}
		assetManager.GetTexture("play_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("stop_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("translate_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("rotate_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("scale_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("none_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("music_icon")->SetIsEditorTexture(true);
		assetManager.GetTexture("scene_icon")->SetIsEditorTexture(true);
		assetManager.GetTexture("model_icon")->SetIsEditorTexture(true);
		assetManager.GetTexture("shader_icon")->SetIsEditorTexture(true);
		assetManager.GetTexture("skybox")->SetIsEditorTexture(true);
		assetManager.GetTexture("HDR")->SetIsEditorTexture(true);
		assetManager.GetTexture("ssaoNoise")->SetIsEditorTexture(true);

	}

	bool Application::LoadEditorMeshes()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		if (!assetManager.AddModelFromMemory("cube", "cube"))
		{
			ENGINE_ERROR("Failed to load default mesh [cube]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("sphere", "sphere"))
		{
			ENGINE_ERROR("Failed to load default mesh [sphere]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("capsule", "capsule"))
		{
			ENGINE_ERROR("Failed to load default mesh [capsule]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("plane", "plane"))
		{
			ENGINE_ERROR("Failed to load default mesh [plane]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("skybox", "skybox"))
		{
			ENGINE_ERROR("Failed to load default mesh [skybox]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("hud_quad", "hud_quad"))
		{
			ENGINE_ERROR("Failed to load default mesh [hud_quad]!");
			return false;
		}
		if (!assetManager.AddModelFromMemory("quad", "quad"))
		{
			ENGINE_ERROR("Failed to load default mesh [quad]!");
			return false;
		}

		assetManager.GetModel("cube")->SetIsEditorModel(true);
		assetManager.GetModel("sphere")->SetIsEditorModel(true);
		assetManager.GetModel("capsule")->SetIsEditorModel(true);
		assetManager.GetModel("plane")->SetIsEditorModel(true);
		assetManager.GetModel("skybox")->SetIsEditorModel(true);
		assetManager.GetModel("hud_quad")->SetIsEditorModel(true);
		assetManager.GetModel("quad")->SetIsEditorModel(true);
		return true;
	}

	bool Application::LoadEditorBuffers()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& bufferManager = mainRegistry.GetBufferManager();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();

		// frame buffer
		bufferManager.AddFrameBuffer("GAME_FB", ENGINE_RENDERING::BufferType::FRAMEBUFFER, 600, 600, true);
		bufferManager.AddFrameBuffer("SCENE_FB", ENGINE_RENDERING::BufferType::FRAMEBUFFER, 600, 600, true);
		// gbuffer
		bufferManager.AddFrameBuffer("GAME_GB", ENGINE_RENDERING::BufferType::GBUFFER, 600, 600, true);
		bufferManager.AddFrameBuffer("SCENE_GB", ENGINE_RENDERING::BufferType::GBUFFER, 600, 600, true);
		// ssao
		bufferManager.AddFrameBuffer("GAME_SSAO",ENGINE_RENDERING::BufferType::SSAO, 600,600,false);
		bufferManager.AddFrameBuffer("GAME_SSAO_Blur", ENGINE_RENDERING::BufferType::SSAO, 600, 600, false);
		bufferManager.AddFrameBuffer("SCENE_SSAO", ENGINE_RENDERING::BufferType::SSAO, 600, 600, false);
		bufferManager.AddFrameBuffer("SCENE_SSAO_Blur", ENGINE_RENDERING::BufferType::SSAO, 600, 600, false);
		bufferManager.AddFrameBuffer("IBL", ENGINE_RENDERING::BufferType::IBL, 512, 512, true);
		
		// shadowmap for direction light
		for (int i = 0; i < lightSystem->GetMaxDirLights(); i++)
		{
			bufferManager.AddFrameBuffer(
				"shadow_map_" + std::to_string(i), 
				ENGINE_RENDERING::BufferType::SHADOWMAP, 
				2048, 2048, false);
		}
		// shadowcubemap for point light
		for (int i = 0; i < lightSystem->GetMaxPointLights(); i++)
		{
			bufferManager.AddFrameBuffer(
				"shadow_cubemap_" + std::to_string(i), 
				ENGINE_RENDERING::BufferType::SHADOWCUBEMAP, 
				2048, 2048, false);
		}

		// uniform buffer
		bufferManager.AddUniformBuffer("matrix", 2 * sizeof(glm::mat4), 0);
		bufferManager.AddUniformBuffer("DirLights", lightSystem->GetMaxDirLights() * sizeof(ENGINE_RENDERING::DirLight), 1);
		bufferManager.AddUniformBuffer("PointLights", lightSystem->GetMaxPointLights() * sizeof(ENGINE_RENDERING::PointLight), 2);
		bufferManager.AddUniformBuffer("SSAO_samples", 64 * sizeof(glm::vec4), 3);

		auto& assetManager = mainRegistry.GetAssetManager();
		// forward rendering shader
		auto forward_BlinnPhong = assetManager.GetShader("forward_BlinnPhong");
		if (forward_BlinnPhong->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		auto forward_Color = assetManager.GetShader("forward_Color");
		if (forward_Color->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		// defer rendering shader
		auto defer_gbuffer = assetManager.GetShader("defer_Gbuffer");
		if (defer_gbuffer->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		auto defer_lighting = assetManager.GetShader("defer_Lighting");
		if (defer_lighting->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		//
		auto physics_Debug = assetManager.GetShader("physics_Debug");
		if (physics_Debug->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		// defer SSAO
		auto defer_SSAO = assetManager.GetShader("defer_SSAO");
		if (defer_SSAO->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return false;
		}
		// bind uniform block index
		forward_BlinnPhong->BindUniformBlock("Matrices", 0);
		forward_Color->BindUniformBlock("Matrices", 0);
		defer_gbuffer->BindUniformBlock("Matrices", 0);
		defer_SSAO->BindUniformBlock("Matrices", 0);
		physics_Debug->BindUniformBlock("Matrices", 0);
		forward_BlinnPhong->BindUniformBlock("DirLights", 1);
		defer_lighting->BindUniformBlock("DirLights", 1);
		forward_BlinnPhong->BindUniformBlock("PointLights", 2);
		defer_lighting->BindUniformBlock("PointLights", 2);
		defer_SSAO->BindUniformBlock("SSAO_samples", 3);

		// uniform block -- ssao samoles (it never update, so set it here)
		std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
		std::default_random_engine generator;
		std::vector<glm::vec4> ssaoKernel; // 声明为 vec4 以满足 std140 对齐
		for (GLuint i = 0; i < 64; ++i)
		{
			glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
			sample = glm::normalize(sample);
			sample *= randomFloats(generator);
			GLfloat scale = GLfloat(i) / 64.0;
			scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
			sample *= scale;
			// 放入 vec4，第四个常量 w 设为 0.0f
			ssaoKernel.push_back(glm::vec4(sample, 0.0f));
		}
		const auto& ssaoSamplesUBO = bufferManager.GetUniformBuffer("SSAO_samples");
		ssaoSamplesUBO->UpdateUniformBuffer(
			ssaoKernel.data(),
			64 * sizeof(glm::vec4), // 使用 vec4 的总大小 (1024 字节)
			0
		);

		return true;
	}

	void Application::PrepareIBL()
	{
		GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
		GLboolean blendEnabled = glIsEnabled(GL_BLEND);
		GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);

		auto& mainRegistry = MAIN_REGISTRY();
		auto& bufferManager = mainRegistry.GetBufferManager();
		auto& assetManager = mainRegistry.GetAssetManager();
		const auto& hdr_texture = assetManager.GetTexture("HDR");
		const auto& ibl_fb = bufferManager.GetFrameBuffer("IBL");
		const auto& ibl_proj_shader = assetManager.GetShader("ibl_proj");
		const auto& ibl_conv_shader = assetManager.GetShader("ibl_conv");
		const auto& ibl_prefilter_shader = assetManager.GetShader("ibl_prefilter");
		const auto& ibl_brdf_shader = assetManager.GetShader("ibl_brdf");
		const auto& cube = assetManager.GetModel("cube");
		const auto& quad = assetManager.GetModel("quad");

		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] = {
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// project equirectangularMap to cubemap
		ibl_fb->Bind();
		ibl_proj_shader->Enable();
		ibl_proj_shader->SetUniformInt("equirectangularMap", 0);
		ibl_proj_shader->SetUniformMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdr_texture->GetID());
		glViewport(0, 0, 512, 512);
		for (unsigned int i = 0; i < 6; i++)
		{
			ibl_proj_shader->SetUniformMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl_fb->GetTextureID(0), 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cube->Draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_fb->GetTextureID(0));
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		// conv cubemap into irradianceMap
		ibl_fb->Bind();
		ibl_conv_shader->Enable();
		ibl_conv_shader->SetUniformInt("environmentMap", 0);
		ibl_conv_shader->SetUniformMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_fb->GetTextureID(0));
		glViewport(0, 0, 32, 32);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
		for (unsigned int i = 0; i < 6; i++)
		{
			ibl_conv_shader->SetUniformMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl_fb->GetTextureID(1), 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cube->Draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// prefilter cubemap
		ibl_fb->Bind();
		ibl_prefilter_shader->Enable();
		ibl_prefilter_shader->SetUniformInt("environmentMap", 0);
		ibl_prefilter_shader->SetUniformMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_fb->GetTextureID(0));
		unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; mip++)
		{
			unsigned int mipWidth = 128 * std::pow(0.5, mip);
			unsigned int mipHeight = 128 * std::pow(0.5, mip);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
			glViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			ibl_prefilter_shader->SetUniformFloat("roughness", roughness);
			for (unsigned int i = 0; i < 6; i++)
			{
				ibl_prefilter_shader->SetUniformMat4("view", captureViews[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl_fb->GetTextureID(2), mip);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				cube->Draw();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// lut
		ibl_fb->Bind();
		ibl_brdf_shader->Enable();
		glViewport(0, 0, 512, 512);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ibl_fb->GetTextureID(3), 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad->Draw();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		if (blendEnabled)     glEnable(GL_BLEND);      else glDisable(GL_BLEND);
		if (cullFaceEnabled)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
	}

	void Application::ProcessEvents()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		//bool load = false;
		//if (pCurrentScene && pCurrentScene->CheckPlay())
		//{
		//	load = true;
		//	//auto& runtimeRegistry = pCurrentScene->GetRuntimeRegistry();
		//	//auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		//	//auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		//}

		auto& inputManager = ENGINE_CORE::INPUTS::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		auto& engine = ENGINE_CORE::CoreEngineData::GetInstance();

		//process Events
		while (SDL_PollEvent(&m_Event))
		{
			ImGui_ImplSDL2_ProcessEvent(&m_Event);
			switch (m_Event.type)
			{
			case SDL_QUIT:
			{
				m_bIsRunning = false;
				break;
			}
			case SDL_KEYDOWN:
			{
				//if (m_Event.key.keysym.sym == SDLK_ESCAPE)
				//	m_bIsRunning = false;
				if (m_Event.key.keysym.sym == SDLK_1 && pCurrentScene)
				{
					engine.ToggleRenderCollisions();
					auto& runtimeRegistry = pCurrentScene->GetRegistry();
					//auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
					auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
					physicsWorld->setIsDebugRenderingEnabled(engine.RenderCollidersEnabled());
					auto view = runtimeRegistry.GetRegistry().view<ENGINE_CORE::ECS::PhysicsComponent>();
					for (auto [entity, physics] : view.each())
					{
						physics.SetDebug(engine.RenderCollidersEnabled());
					}
					physicsWorld->update(1.0f / 60.0f);
				}
				keyboard.OnKeyPressed(m_Event.key.keysym.sym);
				break;
			}
			case SDL_KEYUP:
			{
				keyboard.OnKeyReleased(m_Event.key.keysym.sym);
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				mouse.OnBtnPressed(m_Event.button.button);
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				mouse.OnBtnReleased(m_Event.button.button);
				break;
			}
			case SDL_MOUSEMOTION:
			{
				mouse.SetMouseMoving(true);
				mouse.SetMouseOffset(m_Event.motion.xrel, m_Event.motion.yrel);
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				mouse.SetMouseWheelX(m_Event.wheel.x);
				mouse.SetMouseWheelY(m_Event.wheel.y);

				break;
			}
			case SDL_WINDOWEVENT: // MODIFICATION: 监听窗口事件
			{
				if (m_Event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					int newWidth = m_Event.window.data1;
					int newHeight = m_Event.window.data2;
					m_pWindow->SetWidth(newWidth);
					m_pWindow->SetHeight(newHeight);
					//ENGINE_CORE::CoreEngineData::GetInstance().SetWindowWidth(newWidth);
					//ENGINE_CORE::CoreEngineData::GetInstance().SetWindowHeight(newHeight);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void Application::Update()
	{
		ENGINE_CORE::CoreEngineData::GetInstance().UpdateDeltaTime();

		auto& mainRegistry = MAIN_REGISTRY();
		auto& displayHolder = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>();
		for (const auto& pDisplay : displayHolder->displays)
			pDisplay->Update();

		auto& inputManager = ENGINE_CORE::INPUTS::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		keyboard.Update();
		mouse.Update();
	}

	void Application::Render()
	{
		BeginImGui();
		RenderImGui();
		EndImGui();
		SDL_GL_SwapWindow(m_pWindow->GetWindow().get());
	}

	void Application::CleanUp()
	{
		SDL_Quit();
	}

	bool Application::CreateDisplays()
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto pDisplayHolder = std::make_shared<ENGINE_EDITOR::DisplayHolder>();
		if (!pDisplayHolder)
		{
			ENGINE_ERROR("Failed to create the DisplayHolder");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>(pDisplayHolder))
		{
			ENGINE_ERROR("Failed to add the DisplayHolder to the registry context!");
			return false;
		}
	
		// game display
		auto pGameDisplay = std::make_unique<ENGINE_EDITOR::GameDisplay>();
		if (!pGameDisplay)
		{
			ENGINE_ERROR("Failed to create the GameDisplay");
			return false;
		}

		// scene display
		auto pSceneDisplay = std::make_unique< ENGINE_EDITOR::SceneDisplay>();
		if (!pSceneDisplay)
		{
			ENGINE_ERROR("Failed to create the SceneDisplay");
			return false;
		}

		// log display
		auto pLogDisplay = std::make_unique<ENGINE_EDITOR::LogDisplay>();
		if (!pLogDisplay)
		{
			ENGINE_ERROR("Failed to create the LogDisplay");
			return false;
		}

		// asset display
		auto pAssetDisplay = std::make_unique<ENGINE_EDITOR::AssetDisplay>();
		if (!pAssetDisplay)
		{
			ENGINE_ERROR("Failed to create the AssetDisplay");
			return false;
		}

		// MenuDisplay
		auto pMenuDisplay = std::make_unique<ENGINE_EDITOR::MenuDisplay>();
		if (!pMenuDisplay)
		{
			ENGINE_ERROR("Failed to create the MenuDisplay");
			return false;
		}

		// scene hierarchy display
		auto pSceneHierarchyDisplay = std::make_unique<ENGINE_EDITOR::SceneHierarchyDisplay>();
		if (!pSceneHierarchyDisplay)
		{
			ENGINE_ERROR("Failed to create the scene hierarchy display!");
			return false;
		}

		pDisplayHolder->displays.push_back(std::move(pGameDisplay));
		pDisplayHolder->displays.push_back(std::move(pSceneDisplay));
		pDisplayHolder->displays.push_back(std::move(pLogDisplay));
		pDisplayHolder->displays.push_back(std::move(pAssetDisplay));
		pDisplayHolder->displays.push_back(std::move(pMenuDisplay));
		pDisplayHolder->displays.push_back(std::move(pSceneHierarchyDisplay));
		
		return true;
	}

	bool Application::InitImGui()
	{
		const char* glslVersion = "#version 450";
		IMGUI_CHECKVERSION();
		if (!ImGui::CreateContext())
		{
			ENGINE_ERROR("Failed to create ImGui Context");
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		if (!ImGui_ImplSDL2_InitForOpenGL(m_pWindow->GetWindow().get(), m_pWindow->GetGLContext()))
		{
			ENGINE_ERROR("Failed to initialize ImGui SDL2 for OpenGL!");
			return false;
		}
		if (!ImGui_ImplOpenGL3_Init(glslVersion))
		{
			ENGINE_ERROR("Failed to initialize ImGui OpenGL3!");
			return false;
		}

		return true;
	}

	void Application::BeginImGui()
	{
		// start a new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	void Application::EndImGui()
	{
		ImGui::Render(); // generate data for imgui to render
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_GLContext backupContext = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(m_pWindow->GetWindow().get(), backupContext);
		}
	}

	void Application::RenderImGui()
	{
		const auto dockSpaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		if (static auto firstTime = true; firstTime) [[unlikely]]
		{
			firstTime = false;
			ImGui::DockBuilderRemoveNode(dockSpaceId);
			ImGui::DockBuilderAddNode(dockSpaceId);
			auto centerNodeId = dockSpaceId;
			const auto leftNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Left, 0.2f, nullptr, &centerNodeId);
			const auto downNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Down, 0.2f, nullptr, &centerNodeId);
			const auto rightNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Right, 0.25f, nullptr, &centerNodeId);
			//ImGui::DockBuilderDockWindow("Dear ImGui Demo", leftNodeId);
			ImGui::DockBuilderDockWindow("Scene Hierarchy", leftNodeId);
			ImGui::DockBuilderDockWindow("GO Details", rightNodeId);
			ImGui::DockBuilderDockWindow("Game", centerNodeId);
			ImGui::DockBuilderDockWindow("Scene", centerNodeId);
			ImGui::DockBuilderDockWindow("Asset", downNodeId);
			ImGui::DockBuilderDockWindow("Logs", downNodeId);
			ImGui::DockBuilderFinish(dockSpaceId);
		}

		auto& mainRegistry = MAIN_REGISTRY();
		auto& pDisplayHolder = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>();
		for (const auto& pDisplay : pDisplayHolder->displays)
		{
			pDisplay->Draw();
		}
		//ImGui::ShowDemoWindow();
	}

	Application& Application::GetInstance()
	{
		static Application app{};
		return app;
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		if (!Initialize())
		{
			ENGINE_ERROR("Initialization failed!");
			return;
		}

		while (m_bIsRunning)
		{
			ProcessEvents();
			Update();
			Render();
		}
		CleanUp();
	}
}