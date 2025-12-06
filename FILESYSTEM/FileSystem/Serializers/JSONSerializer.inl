#pragma once
#include "JSONSerializer.h"

namespace ENGINE_FileSystem {
	template<typename TValue>
	inline JSONSerializer& JSONSerializer::AddKeyValuePair(const std::string& key, const TValue& value)
	{
		m_pWriter->Key(key.c_str()); // 1. 写入键名 (Key)

		// 2. 根据 TValue 的类型，决定写入什么格式的值 (Value)
		// "if constexpr" --在编译时进行条件判断，避免不必要的运行时开销，提高程序的效率
		if constexpr (std::is_same_v<TValue, std::string>) {
			m_pWriter->String(value.c_str());
		}
		else if constexpr (std::is_integral_v<TValue>)
		{
			m_pWriter->Int64(value);
		}
		else if constexpr (std::is_unsigned_v<TValue>)
		{
			m_pWriter->Uint64(value);
		}
		else if constexpr (std::is_floating_point_v<TValue>)
		{
			m_pWriter->Double(value);
		}
		else
		{
			static_assert(false, "Type is not supported!");
		}

		return *this;
	}

	template<>
	inline JSONSerializer& JSONSerializer::AddKeyValuePair(const std::string& key, const bool& value)
	{
		m_pWriter->Key(key.c_str());
		m_pWriter->Bool(value);
		return *this;
	}

}