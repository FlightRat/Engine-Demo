#include "FileDialog.h"
#include "tinyfiledialogs.h"

namespace ENGINE_FileSystem {
	// -----------------------------------------------------------------------------
	// [编码架构说明] 
	// 确保 Windows 环境下统一使用 UTF-8，摒弃 GBK 造成的乱码问题。
	// tinyfd_winUtf8 是 tinyfiledialogs.h 中声明的全局变量：
	// 设置为 0 (默认): 使用多字节本地编码 (MBCS/GBK)
	// 设置为 1: 使用纯 UTF-8 编码进行系统 API 交互
	// -----------------------------------------------------------------------------
	inline void EnsureUTF8Context()
	{
#ifdef _WIN32
		tinyfd_winUtf8 = 1;
#endif
	}

	std::string FileDialog::OpenFileDialog(const std::string& sTitle, const std::string& sPath, const std::vector<const char*> filters, const std::string& sFilterDesc)
	{
		EnsureUTF8Context();
		const char* file = tinyfd_openFileDialog(sTitle.c_str(), sPath.c_str(), filters.size(), filters.data(), sFilterDesc.c_str(), 1);
		if (!file)
			return std::string();
		return std::string{ file };
	}

	std::string FileDialog::SaveFileDialog(const std::string& sTitle, const std::string& sPath, const std::vector<const char*> filters, const std::string& sFilterDesc)
	{
		EnsureUTF8Context();
		const char* file = tinyfd_saveFileDialog(sTitle.c_str(), sPath.c_str(), filters.size(), filters.data(), sFilterDesc.c_str());
		if (!file)
			return std::string();
		return std::string{ file };
	}

	std::string FileDialog::SelectFolderDialog(const std::string& sTitle, const std::string& sPath)
	{
		EnsureUTF8Context();
		const char* folder = tinyfd_selectFolderDialog(sTitle.c_str(), sPath.c_str());
		if (!folder)
			return std::string();
		return std::string{ folder };
	}
}


