#pragma once
#include<string>
namespace SOUNDS {
	struct SoundParams
	{
		std::string name{ "" }, description{ "" }, filename{ "" };
		double duration{ 0.0 };
	};
}