#include "test/test.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/memory/memory.h"

ModuleMappingArrayResult test_module_map_init(void)
{
	ModuleMappingArrayResult result = fun_array_ModuleMapping_create(16);
	if (fun_error_is_error(result.error)) {
		return result;
	}

	// String module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "string";
		mapping.sources[0] =
			"../../vendor/fundamental/src/string/stringConversion.c";
		mapping.sources[1] =
			"../../vendor/fundamental/src/string/stringOperations.c";
		mapping.sources[2] =
			"../../vendor/fundamental/src/string/stringTemplate.c";
		mapping.sources[3] =
			"../../vendor/fundamental/src/string/stringValidation.c";
		mapping.source_count = 4;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Memory module mapping (arch-specific, handled separately in scaffolder)
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "memory";
		mapping.sources[0] = "../../vendor/fundamental/arch/memory";
		mapping.source_count = 1;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Console module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "console";
		mapping.sources[0] = "../../vendor/fundamental/src/console/console.c";
		mapping.sources[1] = "../../vendor/fundamental/arch/console";
		mapping.source_count = 2;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Async module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "async";
		mapping.sources[0] = "../../vendor/fundamental/src/async/async.c";
		mapping.sources[1] = "../../vendor/fundamental/src/async/process.c";
		mapping.sources[2] = "../../vendor/fundamental/arch/async";
		mapping.source_count = 3;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Filesystem module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "filesystem";
		mapping.sources[0] =
			"../../vendor/fundamental/src/filesystem/directory.c";
		mapping.sources[1] =
			"../../vendor/fundamental/src/filesystem/file_exists.c";
		mapping.sources[2] = "../../vendor/fundamental/src/filesystem/path.c";
		mapping.sources[3] = "../../vendor/fundamental/arch/filesystem";
		mapping.source_count = 4;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// File module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "file";
		mapping.sources[0] = "../../vendor/fundamental/arch/file";
		mapping.source_count = 1;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Stream module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "stream";
		mapping.sources[0] = "../../vendor/fundamental/src/stream/stream.c";
		mapping.sources[1] = "../../vendor/fundamental/arch/stream";
		mapping.source_count = 2;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Platform module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "platform";
		mapping.sources[0] = "../../vendor/fundamental/src/platform/platform.c";
		mapping.sources[1] = "../../vendor/fundamental/arch/platform";
		mapping.source_count = 2;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	// Error module mapping
	{
		ModuleMapping mapping;
		mapping.module_name = (String) "error";
		mapping.sources[0] = "../../vendor/fundamental/src/error/error.c";
		mapping.source_count = 1;
		fun_array_ModuleMapping_push(&result.value, mapping);
	}

	return result;
}

ModuleMapLookupResult test_module_map_get(ModuleMappingArray *mappings,
										  String module_name)
{
	ModuleMapLookupResult result;
	result.found = 0;
	result.sources = NULL;
	result.source_count = 0;

	size_t count = fun_array_ModuleMapping_size(mappings);
	for (size_t i = 0; i < count; i++) {
		ModuleMapping mapping = fun_array_ModuleMapping_get(mappings, i);
		if (fun_string_compare(mapping.module_name, module_name) == 0) {
			result.found = 1;
			result.sources = mapping.sources;
			result.source_count = mapping.source_count;
			return result;
		}
	}

	return result;
}

void test_module_map_free(ModuleMappingArray *mappings)
{
	fun_array_ModuleMapping_destroy(mappings);
}
