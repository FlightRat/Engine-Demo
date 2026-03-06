AssetDefs = 
{
	models = 
	{
		{name="nanosuit", path="assets/models/nanosuit/nanosuit.obj"},
		{name="backpack", path="assets/models/backpack/backpack.obj"},
	},
	textures=
	{
		{name="dragon_girl", path="assets/textures/dragon_girl.jpg", pixel_art=false},
		{name="mafuyu", path="assets/textures/mafuyu.png", pixel_art=false},
		{name="brick", path="assets/textures/brick.png", pixel_art=false},
		{name="wood", path="assets/textures/wood.png", pixel_art=false},
		{name="container", path="assets/textures/container.png", pixel_art=false},
		{name="container_specular", path="assets/textures/container_specular.png", pixel_art=false},
		{name="rust", path="assets/textures/rust.png", pixel_art=false}
	},
	music=
	{
		{name="2:23am",path="assets/music/2-23am.wav"}
	},
	soundFx=
	{
		{name="hit",path="assets/soundFx/hit.wav"},
		{name="jump",path="assets/soundFx/jumping.wav"},
		{name="land",path="assets/soundFx/landing.wav"},
		{name="shot",path="assets/soundFx/shooting.wav"},
	}
}

function LoadAssets()
	-- models
	for k, v in pairs(AssetDefs.models) do
		if not AssetManager.add_model(v.name, v.path) then
			ENGINE_Error("Failed to load model [%s] at path [%s]", v.name, v.path)
		else
			ENGINE_Log("Loaded model [%s]", v.name)
		end
	end

	-- Texture
	for k, v in pairs(AssetDefs.textures) do
		if not AssetManager.add_texture(v.name, v.path, v.pixel_art) then
			ENGINE_Error("Failed to load texture [%s] at path [%s]", v.name, v.path)
		else
			ENGINE_Log("Loaded texture [%s]", v.name)
		end
	end
	
	-- Music
	for k, v in pairs(AssetDefs.music) do
		if not AssetManager.add_music(v.name, v.path) then
			ENGINE_Error("Failed to load music [%s] at path [%s]", v.name, v.path)
		else
			ENGINE_Log("Loaded music [%s]", v.name) --equal to£º"Logger.log(string.format("Loaded music [%s]", v.name))"
		end
	end

	-- SoundFx
	for k, v in pairs(AssetDefs.soundFx) do
		if not AssetManager.add_soundFx(v.name, v.path) then
			ENGINE_Error("Failed to load soundFx [%s] at path [%s]", v.name, v.path)
		else
			ENGINE_Log("Loaded soundFx [%s]", v.name)
		end
	end

	-- TODO:Shader

end