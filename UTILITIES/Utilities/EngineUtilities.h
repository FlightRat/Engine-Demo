#pragma once
#include <vector>
#include <algorithm>
#include <ranges>

namespace ENGINE_UTIL
{
	enum class AssetType
	{
		TEXTURE=0,
		MODEL,
		MUSIC,
		SOUNDFX,
		SCENE,
		SHADER,
		NO_TYPE
	};

	template<typename Map>
	std::vector<typename Map::key_type> GetKeys(const Map& map)
	{
		auto keyView = std::views::keys(map);
		std::vector<typename Map::key_type> keys{ keyView.begin(), keyView.end() };
		return keys;
	}

	template<typename Map, typename Func>
	std::vector<typename Map::key_type> GetKeys(const Map& map, Func func)
	{
		auto keyView = map | std::views::filter(func) | std::views::keys;
		std::vector<typename Map::key_type> keys{ keyView.begin(), keyView.end() };
		return keys;
	}

	template<typename Map>
	bool ChangeKey(Map& map, const typename Map::key_type& oldKey, const typename Map::key_type& newKey)
	{
		if (!map.contains(oldKey) || map.contains(newKey))
			return false;

		auto node = map.extract(oldKey);
		node.key() = newKey;
		auto [itr, bSuccess, nType] = map.insert(std::move(node));
		return bSuccess;
	}
}