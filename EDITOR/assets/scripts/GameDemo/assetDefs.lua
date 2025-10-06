AssetDefs = 
{
	textures=
	{
		{name="mafuyu", path="assets/textures/mafuyu.png", pixel_art=false},
		{name="wood", path="assets/textures/wood.png", pixel_art=false},
		{name="container", path="assets/textures/container.png", pixel_art=false}
	},
	music=
	{
		{name="2:23am",path="assets/music/2-23am.wav"}
	},
	soundFx=
	{
		{name="hit",path="assets/soundFx/hit.wav"}
	}
}

function LoadAssets()
	-- Texture
	for k, v in pairs(AssetDefs.textures) do
		if not AssetManager.add_texture(v.name, v.path, v.pixel_art) then
			print("Failed to load texture ["..v.name.."] at path ["..v.path.."]!")
		else
			print("Loaded texture["..v.name.."]")
		end
	end
	
	-- Music
	for k, v in pairs(AssetDefs.music) do
		if not AssetManager.add_music(v.name, v.path) then
			print("Failed to load music ["..v.name.."] at path ["..v.path.."]!")
		else
			print("Loaded music["..v.name.."]")
		end
	end

	-- SoundFx
	for k, v in pairs(AssetDefs.soundFx) do
		if not AssetManager.add_soundFx(v.name, v.path) then
			print("Failed to load soundFx ["..v.name.."] at path ["..v.path.."]!")
		else
			print("Loaded soundFx["..v.name.."]")
		end
	end

	-- TODO:Shader

end