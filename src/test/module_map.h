#ifndef TEST_MODULE_MAP_H
#define TEST_MODULE_MAP_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"
#include "vendor/fundamental/include/array/array.h"

/**
 * Module-to-source mapping
 * Maps module names to required fundamental source files
 */

/**
 * Maximum sources per module
 */
#define MAX_MODULE_SOURCES 32

/**
 * Module mapping entry
 */
typedef struct {
	String module_name;
	const char *sources[MAX_MODULE_SOURCES];
	int source_count;
} ModuleMapping;

/**
 * Array type for module mappings
 */
DEFINE_ARRAY_TYPE(ModuleMapping)

/**
 * Module mapping lookup result
 */
typedef struct {
	int found;
	const char **sources;
	int source_count;
} ModuleMapLookupResult;

/**
 * Initialize module mappings
 * @return ModuleMappingArray with all mappings
 */
ModuleMappingArrayResult test_module_map_init(void);

/**
 * Get sources for a module
 * @param mappings Module mappings
 * @param module_name Name of module to lookup
 * @return ModuleMapLookupResult with source files
 */
ModuleMapLookupResult test_module_map_get(ModuleMappingArray *mappings,
										  String module_name);

/**
 * Free module mappings
 * @param mappings Mappings to free
 */
void test_module_map_free(ModuleMappingArray *mappings);

#endif // TEST_MODULE_MAP_H
