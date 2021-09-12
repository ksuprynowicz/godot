#include "godot_elixir.h"

#include "core/string/string_builder.h"
#include "main/main.h"
#include "modules/gdscript/gdscript.h"
#include "scene/main/node.h"

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>

static OS_LinuxBSD os;

UNIFEX_TERM init(UnifexEnv *env, MyState *state, char **in_strings, unsigned int list_length) {
	int err = OK;
	if (state) {
		return init_result_fail(env, state, "Godot is already initialized.");
	}
	state = unifex_alloc_state(env);

	setlocale(LC_CTYPE, "");

	state->ret = getcwd(state->cwd, PATH_MAX);

	err = Main::setup(in_strings[0], list_length - 1, &in_strings[1]);
	if (err != OK) {
		return init_result_fail(env, state, "Godot can't be setup.");
	}
	err = Main::start();
	if (err != OK) {
		return init_result_fail(env, state, "Godot can't start.");
	}
	os.run();
	return init_result_ok(env, state, err);
}

UNIFEX_TERM call(UnifexEnv *env, MyState *state, char *method, char **in_strings, unsigned int list_length) {
	if (!state) {
		return init_result_fail(env, state, "Godot is not initialized.");
	}
	if (!os.get_main_loop()) {
		return init_result_fail(env, state, "Godot does not have a main loop.");
	}
	Array args;
	args.resize(list_length);
	for (int32_t arg_i = 0; arg_i < list_length; arg_i++) {
		args[arg_i] = in_strings[arg_i];
	}
	Variant res = os.get_main_loop()->callv(method, args);
	switch (res.get_type()) {
		case Variant::NIL: {
			return init_result_fail(env, state, "Call is invalid.");
		} break;
		case Variant::BOOL: {
			int res_int = res;
			return call_result_ok_bool(env, state, res_int);
		} break;
		case Variant::INT: {
			int res_int = res;
			return call_result_ok_int(env, state, res_int);
		} break;
		case Variant::FLOAT: {
			const CharString res_float_string = rtos(res).utf8();
			return call_result_ok_string(env, state, res_float_string.get_data());
		} break;
		case Variant::STRING: {
			const CharString res_char_string = String(res).utf8();
			const char *res_string = res_char_string.get_data();
			return call_result_ok_string(env, state, res_string);
		} break;
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::ARRAY: {
			const Array array = res;
			StringBuilder builder;
			for (int32_t array_i = 0; array_i < array.size(); array_i++) {
				builder.append(array[array_i]);
			}
			const CharString res_string = String(builder.as_string()).utf8();
			return call_result_ok_string(env, state, res_string);
		} break;
		default: {
			const CharString res_char_string = (String("Unsupported result: ") + String(res)).utf8();
			const char *res_string = res_char_string.get_data();
			return init_result_fail(env, state, res_string);
		} break;
	}
	return init_result_fail(env, state, "Call is invalid.");
}

void handle_destroy_state(UnifexEnv *env, MyState *state) {
	UNIFEX_UNUSED(env);
	UNIFEX_UNUSED(state);
	if (os.get_main_loop()) {
		os.get_main_loop()->finalize();
	}
	Main::cleanup();
	if (state->ret) { // Previous getcwd was successful
		if (chdir(state->cwd) != 0) {
			ERR_PRINT("Couldn't return to previous working directory.");
		}
	}
	free(state->cwd);
	state->cwd = nullptr;
}
