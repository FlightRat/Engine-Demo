#pragma once
#include <string>
#include <vector>

namespace ENGINE_FileSystem {
	class FileDialog
	{
	public:
		std::string OpenFileDialog(const std::string& sTitle = "Open", const std::string& sPath = "",
								   const std::vector<const char*> filters = {}, const std::string& sFilterDesc = "");

		std::string SaveFileDialog(const std::string& sTitle = "Save", const std::string& sPath = "",
								   const std::vector<const char*> filters = {}, const std::string& sFilterDesc = "");

		std::string SelectFolderDialog(const std::string& sTitle = "Select Folder", const std::string& sPath="");

	};
}