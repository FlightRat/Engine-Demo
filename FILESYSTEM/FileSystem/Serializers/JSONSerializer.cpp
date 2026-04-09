#include "JSONSerializer.h"
#include "Logger/Logger.h"
#include<format>

constexpr int MAX_DECIMAL_PLACES = 10;

namespace ENGINE_FileSystem {
	// RAII（资源获取即初始化）原则：在构造时准备好所有资源（打开文件、分配内存），在析构时释放资源（关闭文件、释放内存）。
	JSONSerializer::JSONSerializer(const std::string& sFilename) :
		m_Filestream{}, m_StringBuffer{},
		m_pWriter{ std::make_unique < rapidjson::PrettyWriter < rapidjson::StringBuffer>>(m_StringBuffer) },
		m_NumObjectsStarted{ 0 },
		m_NumArraysStarted{ 0 }
	{
		// std::ios::out：表示要写入文件
		// std::ios::trunc：如果文件不存在，创建它；如果文件已经存在，清空里面的所有内容！
		m_Filestream.open(sFilename, std::ios::out | std::ios::trunc);	
		assert(m_Filestream.is_open() && "Failed to open file!");
		if (!m_Filestream.is_open())
		{
			// throw std::runtime_error(fmt::format("JSONSerializer failed to  open file [{}]", sFilename));
			throw std::runtime_error("JSONSerializer failed to open file [" + sFilename + "]");
		}
		m_pWriter->SetMaxDecimalPlaces(MAX_DECIMAL_PLACES);
	}
	JSONSerializer::~JSONSerializer()
	{
		if (m_Filestream.is_open())
			m_Filestream.close();
	}

	bool JSONSerializer::Reset(const std::string& sFilename)
	{
		assert(m_NumObjectsStarted == 0 && "Failed to reset, there is document remains unfinished!");
		assert(m_NumArraysStarted == 0 && "Failed to reset, theres is array still opened!");
		if (m_NumObjectsStarted != 0)
		{
			ENGINE_ERROR("Failed to reset, there is document remains unfinished!");
			return false;
		}
		if (m_NumArraysStarted != 0)
		{
			ENGINE_ERROR("Failed to reset, theres is array still opened!");
			return false;
		}
		if (m_Filestream.is_open())
			m_Filestream.close();

		m_Filestream.open(sFilename, std::ios::out | std::ios::trunc);
		assert(m_Filestream.is_open() && "Failed to open file!");
		if (!m_Filestream.is_open())
		{
			// throw std::runtime_error(fmt::format("JSONSerializer failed to  open file [{}]", sFilename));
			throw std::runtime_error("JSONSerializer failed to open file [" + sFilename + "]");
		}

		return true;
	}

	bool JSONSerializer::StartDocument()
	{	
		/* 当m_NumObjectsStarted为0时，表示没有正在写的JSON文件，用StartObject写入"{"，标志着JSON文件的开始*/
		assert(m_NumObjectsStarted == 0 && "Document has already been started.Please reset the serializer!");
		if (m_NumObjectsStarted != 0)
		{
			ENGINE_ERROR("Document has already been started.Please reset the serializer!");
			return false;
		}
		++m_NumObjectsStarted;
		return m_pWriter->StartObject();
	}
	bool JSONSerializer::EndDocument()
	{	// 当m_NumObjectsStarted为1时，表示JSON文件还剩最终的一个}，用EndObject写入“}”，并把buffer刷入文件流
		assert(m_NumObjectsStarted == 1 && "There should be only one object opened, maybe forget to call EndObject()?");
		assert(m_NumArraysStarted == 0 && "Too many arrays started, maybe forget to call EndArray()?");
		if (m_NumObjectsStarted != 1)
		{
			ENGINE_ERROR("Failed to end document correctly.There should be only one object opened, maybe forget to call EndObject()?");
			return false;
		}
		if (m_NumArraysStarted != 0)
		{
			ENGINE_ERROR("Failed to end document correctly.Too many arrays started, maybe forget to call EndArray()?");
			return false;
		}
		
		m_pWriter->EndObject();
		--m_NumObjectsStarted;
		m_Filestream << m_StringBuffer.GetString(); // 将内存 buffer 中的 JSON 字符串刷入文件
		m_Filestream.flush();
		return true;
	}


	JSONSerializer& JSONSerializer::StartNewObject(const std::string& key)
	{
		++m_NumObjectsStarted;
		if (!key.empty())
			m_pWriter->Key(key.c_str());
		m_pWriter->StartObject();
		return *this;
	}
	JSONSerializer& JSONSerializer::EndObject()
	{
		assert(m_NumObjectsStarted > 1 && "EndObject called too many times!");
		--m_NumObjectsStarted;
		m_pWriter->EndObject();
		return *this;
	}

	JSONSerializer& JSONSerializer::StartNewArray(const std::string& key)
	{
		++m_NumArraysStarted;
		m_pWriter->Key(key.c_str());
		m_pWriter->StartArray();
		return *this;
	}
	JSONSerializer& JSONSerializer::EndArray()
	{
		assert(m_NumArraysStarted > 0 && "EndArray() called too many times!");
		--m_NumArraysStarted;
		m_pWriter->EndArray();
		return *this;
	}
}

