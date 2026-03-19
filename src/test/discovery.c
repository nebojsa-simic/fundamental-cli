#include "test/test.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/async/async.h"
#include <stddef.h>
#include <stdint.h>

TestDiscoveryResult test_discover(String tests_dir)
{
	TestDiscoveryResult result;
	result.status = TEST_DISCOVERY_SUCCESS;
	result.error_message = (String) "";

	boolResult dir_exists = fun_path_exists(tests_dir);
	if (fun_error_is_error(dir_exists.error)) {
		result.status = TEST_DISCOVERY_ERROR;
		result.error_message = (String) "exists_err";
		TestModuleArrayResult ar = fun_array_TestModule_create(0);
		if (fun_error_is_ok(ar.error))
			result.modules = ar.value;
		return result;
	}
	if (!dir_exists.value) {
		result.status = TEST_DISCOVERY_NO_TESTS;
		result.error_message = (String) "no_dir";
		TestModuleArrayResult ar = fun_array_TestModule_create(0);
		if (fun_error_is_ok(ar.error))
			result.modules = ar.value;
		return result;
	}

	TestModuleArrayResult array_result = fun_array_TestModule_create(16);
	if (fun_error_is_error(array_result.error)) {
		result.status = TEST_DISCOVERY_ERROR;
		result.error_message = (String) "arr_err";
		return result;
	}
	result.modules = array_result.value;

	MemoryResult list_mem = fun_memory_allocate(4096);
	if (fun_error_is_error(list_mem.error)) {
		result.status = TEST_DISCOVERY_ERROR;
		result.error_message = (String) "mem_err";
		return result;
	}

	ErrorResult list_result =
		fun_filesystem_list_directory(tests_dir, list_mem.value);
	if (fun_error_is_error(list_result)) {
		fun_memory_free(&list_mem.value);
		result.status = TEST_DISCOVERY_ERROR;
		result.error_message = (String) "list_err";
		return result;
	}

	const char *listing = (const char *)list_mem.value;
	if (listing[0] == '\0') {
		fun_memory_free(&list_mem.value);
		result.status = TEST_DISCOVERY_NO_TESTS;
		result.error_message = (String) "empty";
		return result;
	}

	const char *ptr = listing;
	while (*ptr != '\0') {
		const char *newline = ptr;
		while (*newline != '\n' && *newline != '\0') {
			newline++;
		}
		StringLength entry_len = (StringLength)(newline - ptr);

		if (entry_len == 0 || (entry_len == 1 && ptr[0] == '.') ||
			(entry_len == 2 && ptr[0] == '.' && ptr[1] == '.')) {
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}

		char entry_name[256];
		if (entry_len >= sizeof(entry_name)) {
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		for (StringLength i = 0; i < entry_len; i++) {
			entry_name[i] = ptr[i];
		}
		entry_name[entry_len] = '\0';

		char test_path[520];
		StringLength tests_len = fun_string_length(tests_dir);
		if (tests_len + 1 + entry_len >= sizeof(test_path)) {
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		fun_string_copy(tests_dir, test_path, sizeof(test_path));
		test_path[tests_len] = '/';
		fun_string_copy(entry_name, test_path + tests_len + 1,
						sizeof(test_path) - tests_len - 1);
		test_path[tests_len + 1 + entry_len] = '\0';

		if (!test_has_test_file(test_path)) {
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}

		TestModule module;
		MemoryResult name_mem = fun_memory_allocate(entry_len + 1);
		if (fun_error_is_error(name_mem.error)) {
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		fun_string_copy(entry_name, name_mem.value, entry_len + 1);
		module.name = name_mem.value;

		StringLength path_len = fun_string_length(test_path);
		MemoryResult path_mem = fun_memory_allocate(path_len + 1);
		if (fun_error_is_error(path_mem.error)) {
			fun_memory_free(&name_mem.value);
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		fun_string_copy(test_path, path_mem.value, path_len + 1);
		module.path = path_mem.value;

		MemoryResult file_mem = fun_memory_allocate(path_len + 8);
		if (fun_error_is_error(file_mem.error)) {
			fun_memory_free(&name_mem.value);
			fun_memory_free(&path_mem.value);
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		fun_string_copy(test_path, file_mem.value, path_len + 8);
		char *file_ptr = (char *)file_mem.value;
		file_ptr[path_len] = '/';
		String test_c = (String) "test.c";
		for (int i = 0; i < 6; i++) {
			file_ptr[path_len + 1 + i] = test_c[i];
		}
		file_ptr[path_len + 1 + 6] = '\0';
		module.test_file = file_mem.value;

		ErrorResult push_result =
			fun_array_TestModule_push(&result.modules, module);
		if (fun_error_is_error(push_result)) {
			fun_memory_free(&name_mem.value);
			fun_memory_free(&path_mem.value);
			fun_memory_free(&file_mem.value);
			if (*newline == '\n') {
				ptr = newline + 1;
				continue;
			}
			break;
		}
		if (*newline == '\n') {
			ptr = newline + 1;
		} else {
			break;
		}
	}

	fun_memory_free(&list_mem.value);
	if (fun_array_TestModule_size(&result.modules) == 0) {
		result.status = TEST_DISCOVERY_NO_TESTS;
		result.error_message = (String) "no_mods";
	}
	return result;
}

int test_has_test_file(String dir_path)
{
	char test_c_path[520];
	StringLength len = fun_string_length(dir_path);
	if (len + 7 >= sizeof(test_c_path)) {
		return 0;
	}
	fun_string_copy(dir_path, test_c_path, sizeof(test_c_path));
	test_c_path[len] = '/';
	fun_string_copy((String) "test.c", test_c_path + len + 1,
					sizeof(test_c_path) - len - 1);
	test_c_path[len + 1 + 6] = '\0';
	boolResult exists = fun_file_exists(test_c_path);
	return fun_error_is_ok(exists.error) && exists.value;
}

void test_discovery_free(TestDiscoveryResult *result)
{
	size_t count = fun_array_TestModule_size(&result->modules);
	for (size_t i = 0; i < count; i++) {
		TestModule module = fun_array_TestModule_get(&result->modules, i);
		fun_memory_free((Memory *)&module.name);
		fun_memory_free((Memory *)&module.path);
		fun_memory_free((Memory *)&module.test_file);
	}
	fun_array_TestModule_destroy(&result->modules);
}
