#include "State.h"
#include <Logger/Logger.h>

void ENGINE_CORE::State::CreateLuaStateBind(sol::state& lua)
{
	lua.new_usertype<State>(
		"state",
		sol::call_constructor,
		sol::factories(
			[](std::string name) {
				return State{ .name = name };
			},
			[](sol::protected_function on_render, sol::protected_function on_update,
				sol::protected_function on_enter, sol::protected_function on_exit,
				sol::protected_function handle_inputs, sol::object variables) {
					return State{ .on_render = on_render,.on_update = on_update,.on_enter = on_enter,.on_exit = on_exit,
								  .handle_inputs = handle_inputs,.variables = variables };
			}
		),
		"set_on_render", [](State& state, sol::protected_function on_render) {
			if (!on_render.valid()) 
			{
				ENGINE_ERROR("Failed to set the on_render function!");
				return;
			}
			state.on_render = on_render;
		},
		"set_on_update", [](State& state, sol::protected_function on_update) {
			if (!on_update.valid())
			{
				ENGINE_ERROR("Failed to set the on_update function!");
				return;
			}
			state.on_update = on_update;
		},
		"set_on_enter", [](State& state, sol::protected_function on_enter) {
			if (!on_enter.valid())
			{
				ENGINE_ERROR("Failed to set the on_enter function!");
				return;
			}
			state.on_enter = on_enter;
		},
		"set_on_exit", [](State& state, sol::protected_function on_exit) {
			if (!on_exit.valid())
			{
				ENGINE_ERROR("Failed to set the on_exit function!");
				return;
			}
			state.on_exit = on_exit;
		},
		"set_handle_inputs", [](State& state, sol::protected_function handle_inputs) {
			if (!handle_inputs.valid())
			{
				ENGINE_ERROR("Failed to set the handle_inputs function!");
				return;
			}
			state.handle_inputs = handle_inputs;
		},
		"set_variable_table", [](State& state, const sol::table& table){
			if (!table.valid())
			{
				ENGINE_ERROR("Failed to set the variable function");
				return;
			}
			state.variables = table;
		},
		"variables", &State::variables,
		"name", &State::name,
		"bKillState", &State::bKillState
	);
}
