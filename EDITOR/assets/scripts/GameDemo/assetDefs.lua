AssetDefs = 
{
	textures=
	{
		{name="mafuyu", path="assets/textures/mafuyu.png", pixel_art=false},
		{name="wood", path="assets/textures/wood.png", pixel_art=false},
		{name="container", path="assets/textures/container.png", pixel_art=false}
	}
}

function LoadAssets()
	for k, v in pairs(AssetDefs.textures) do
		if not AssetManager.add_texture(v.name, v.path, v.pixel_art) then
			print("Failed to load texture ["..v.name.."] at path ["..v.path.."]!")
		else
			print("Loaded texture["..v.name.."]")
		end
	end
	--TODO: ADD MORE ASSETS
end