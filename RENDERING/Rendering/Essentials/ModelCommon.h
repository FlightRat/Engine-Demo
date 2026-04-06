#pragma once
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace ENGINE_RENDERING {
	class ModelRegistry {
	public:
		static bool IsSupported(std::string_view extension)
		{
			const auto ext = NormalizeExtension(extension);
			const auto& set = GetSupportedSet();
			return set.contains(ext);
		}

		static bool IsSupportedPath(const std::string& filePath)
		{
			std::filesystem::path p(filePath);
			return IsSupported(p.extension().string());
		}

		static const std::vector<std::string>& GetSupportedExtensions()
		{
			static const std::vector<std::string> exts = {
				".obj",
				".fbx",
				".gltf",
				".glb"
			};
			return exts;
		}

		static std::string BuildFilterString()
		{
			std::string result;
			const auto& exts = GetSupportedExtensions();
			for (size_t i = 0; i < exts.size(); i++)
			{
				if (i > 0) result += ";";
				result += "*";
				result += exts[i];
			}
			return result;
		}

	private:
		static std::string NormalizeExtension(std::string_view extension)
		{
			std::string ext(extension);

			// 如果没带点，自动补上
			if (!ext.empty() && ext[0] != '.')
			{
				ext.insert(ext.begin(), '.');
			}

			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			return ext;
		}

		static const std::unordered_set<std::string>& GetSupportedSet()
		{
			static const std::unordered_set<std::string> set = {
				".obj",
				".fbx",
				".gltf",
				".glb"
			};
			return set;
		}
	};
}