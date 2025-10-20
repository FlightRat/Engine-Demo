#pragma once
#include<string>
namespace ENGINE_SOUNDS {
	struct SoundParams
	{
		std::string name{ "" }, description{ "" }, filename{ "" };
		double duration{ 0.0 };
	};
}