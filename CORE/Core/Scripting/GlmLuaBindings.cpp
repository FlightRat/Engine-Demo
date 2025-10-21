#include "GlmLuaBindings.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

namespace ENGINE_CORE::Scripting
{
	// vec2 
	void CreateVec2Bind(sol::state& lua)
	{
		// overload multiply
		auto vec2_multiply_overloads = sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) {return v1 * v2; },
			[](const glm::vec2& v1, float value) {return v1 * value; },
			[](float value, const glm::vec2& v2) {return value * v2; }
		);

		// overload divide
		auto vec2_divide_overloads = sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) {return v1 / v2; },
			[](const glm::vec2& v1, float value) {return v1 / value; },
			[](float value, const glm::vec2& v2) {return v2 / value; }
		);

		// overload addition
		auto vec2_addition_overload = sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) {return v1 + v2; },
			[](const glm::vec2& v1, float value) {return v1 + value; },
			[](float value, const glm::vec2& v2) {return v2 + value; }
		);

		// overload substraction
		auto vec2_substraction_overload = sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) {return v1 - v2; },
			[](const glm::vec2& v1, float value) {return v1 - value; },
			[](float value, const glm::vec2& v2) {return v2 - value; }
		);

		// create vec2 usertype
		lua.new_usertype<glm::vec2>(
			"vec2",
			sol::call_constructor,
			sol::constructors<glm::vec2(float), glm::vec2(float, float)>(),
			"x", &glm::vec2::x,
			"y", &glm::vec2::y,
			sol::meta_function::multiplication, vec2_multiply_overloads,
			sol::meta_function::division, vec2_divide_overloads,
			sol::meta_function::addition, vec2_addition_overload,
			sol::meta_function::subtraction, vec2_substraction_overload,
			"length", [](const glm::vec2& v) {return glm::length(v); },
			"lengthSq", [](const glm::vec2& v) {return glm::length2(v); }
		);
	}

	// vec3 
	void CreateVec3Bind(sol::state& lua)
	{
		// overload multiply
		auto vec3_multiply_overloads = sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) {return v1 * v2; },
			[](const glm::vec3& v1, float value) {return v1 * value; },
			[](float value, const glm::vec3& v2) {return value * v2; }
		);

		// overload divide
		auto vec3_divide_overloads = sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) {return v1 / v2; },
			[](const glm::vec3& v1, float value) {return v1 / value; },
			[](float value, const glm::vec3& v2) {return v2 / value; }
		);

		// overload addition
		auto vec3_addition_overload = sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) {return v1 + v2; },
			[](const glm::vec3& v1, float value) {return v1 + value; },
			[](float value, const glm::vec3& v2) {return v2 + value; }
		);

		// overload substraction
		auto vec3_substraction_overload = sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) {return v1 - v2; },
			[](const glm::vec3& v1, float value) {return v1 - value; },
			[](float value, const glm::vec3& v2) {return v2 - value; }
		);

		// create vec3 usertype
		lua.new_usertype<glm::vec3>(
			"vec3",
			sol::call_constructor,
			sol::constructors<glm::vec3(float), glm::vec3(float, float, float)>(),
			"x", &glm::vec3::x,
			"y", &glm::vec3::y,
			"z", &glm::vec3::z,
			sol::meta_function::multiplication, vec3_multiply_overloads,
			sol::meta_function::division, vec3_divide_overloads,
			sol::meta_function::addition, vec3_addition_overload,
			sol::meta_function::subtraction, vec3_substraction_overload,
			"length", [](const glm::vec3& v) {return glm::length(v); },
			"lengthSq", [](const glm::vec3& v) {return glm::length2(v); }
		);
	}

	// vec4
	void CreateVec4Bind(sol::state& lua)
	{
		// overload multiply
		auto vec4_multiply_overloads = sol::overload(
			[](const glm::vec4& v1, const glm::vec4& v2) {return v1 * v2; },
			[](const glm::vec4& v1, float value) {return v1 * value; },
			[](float value, const glm::vec4& v2) {return value * v2; }
		);

		// overload divide
		auto vec4_divide_overloads = sol::overload(
			[](const glm::vec4& v1, const glm::vec4& v2) {return v1 / v2; },
			[](const glm::vec4& v1, float value) {return v1 / value; },
			[](float value, const glm::vec4& v2) {return v2 / value; }
		);

		// overload addition
		auto vec4_addition_overload = sol::overload(
			[](const glm::vec4& v1, const glm::vec4& v2) {return v1 + v2; },
			[](const glm::vec4& v1, float value) {return v1 + value; },
			[](float value, const glm::vec4& v2) {return v2 + value; }
		);

		// overload substraction
		auto vec4_substraction_overload = sol::overload(
			[](const glm::vec4& v1, const glm::vec4& v2) {return v1 - v2; },
			[](const glm::vec4& v1, float value) {return v1 - value; },
			[](float value, const glm::vec4& v2) {return v2 - value; }
		);

		// create vec4 usertype
		lua.new_usertype<glm::vec4>(
			"vec4",
			sol::call_constructor,
			sol::constructors<glm::vec4(float), glm::vec4(float, float, float, float)>(),
			"x", &glm::vec4::x,
			"y", &glm::vec4::y,
			"z", &glm::vec4::z,
			"w", &glm::vec4::w,
			sol::meta_function::multiplication, vec4_multiply_overloads,
			sol::meta_function::division, vec4_divide_overloads,
			sol::meta_function::addition, vec4_addition_overload,
			sol::meta_function::subtraction, vec4_substraction_overload,
			"length", [](const glm::vec4& v) {return glm::length(v); },
			"lengthSq", [](const glm::vec4& v) {return glm::length2(v); }
		);
	}

	void MathFreeFunctions(sol::state& lua)
	{
		lua.set_function("distance", sol::overload(
			[](glm::vec2& a, glm::vec2& b) {return glm::distance(a, b); },
			[](glm::vec3& a, glm::vec3& b) {return glm::distance(a, b); },
			[](glm::vec4& a, glm::vec4& b) {return glm::distance(a, b); }
		));

		lua.set_function("lerp", [](float a, float b, float t) {return std::lerp(a, b, t); });
		lua.set_function("clamp", sol::overload(
			[](float value, float min, float max) {return std::clamp(value, min, max); },
			[](double value, double min, double max) {return std::clamp(value, min, max); },
			[](int value, int min, int max) {return std::clamp(value, min, max); }
		));
	}

	void GLMBindings::CreateLuaGlmBind(sol::state& lua)
	{
		CreateVec2Bind(lua);
		CreateVec3Bind(lua);
		CreateVec4Bind(lua);
		MathFreeFunctions(lua);
	}
}


