#include "FileDialog.h"
#include "tinyfiledialogs.h"

namespace ENGINE_FileSystem {
	std::string FileDialog::OpenFileDialog(const std::string& sTitle, const std::string& sPath, const std::vector<const char*> filters, const std::string& sFilterDesc)
	{
		const char* file = tinyfd_openFileDialog(sTitle.c_str(), sPath.c_str(), filters.size(), filters.data(), sFilterDesc.c_str(), 1);
		if (!file)
			return std::string();
		return std::string{ file };
	}

	std::string FileDialog::SaveFileDialog(const std::string& sTitle, const std::string& sPath, const std::vector<const char*> filters, const std::string& sFilterDesc)
	{
		const char* file = tinyfd_saveFileDialog(sTitle.c_str(), sPath.c_str(), filters.size(), filters.data(), sFilterDesc.c_str());
		if (!file)
			return std::string();
		return std::string{ file };
	}

	std::string FileDialog::SelectFolderDialog(const std::string& sTitle, const std::string& sPath)
	{
		const char* folder = tinyfd_selectFolderDialog(sTitle.c_str(), sPath.c_str());
		if (!folder)
			return std::string();
		return std::string{ folder };
	}
}


