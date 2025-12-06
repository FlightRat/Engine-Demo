#pragma once
#include<fstream>
#include<rapidjson/prettywriter.h>

namespace ENGINE_FileSystem {
	class JSONSerializer
	{
	private:
		// 写入：m_pWriter先把数据写入m_StringBuffer，再由m_Filestream把m_StringBuffer中的写入磁盘
		std::fstream m_Filestream;        // 负责将最终的字符串写入磁盘文件
		rapidjson::StringBuffer m_StringBuffer; // 内存中的缓冲区，m_pWriter 先把数据写到这里
		std::unique_ptr<rapidjson::PrettyWriter<rapidjson::StringBuffer>> m_pWriter; // RapidJSON 的写入器
		int m_NumObjectsStarted;	// 用于计数{
		int m_NumArraysStarted; 
	public:
		JSONSerializer(const std::string& sFilename);
		~JSONSerializer();

		bool Reset(const std::string& sFilename);

		bool StartDocument();
		bool EndDocument();		

		JSONSerializer& StartNewObject(const std::string& key = "");
		JSONSerializer& EndObject();

		JSONSerializer& StartNewArray(const std::string& key);
		JSONSerializer& EndArray();

		template <typename TValue>
		JSONSerializer& AddKeyValuePair(const std::string& key, const TValue& value);

		template <>
		JSONSerializer& AddKeyValuePair(const std::string& key, const bool& value);
	};
}

#include "JSONSerializer.inl"