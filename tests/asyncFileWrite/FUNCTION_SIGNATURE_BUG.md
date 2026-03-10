BUG FOUND: vendor/fundamental/include/filesystem/filesystem.h declares 'String path' but vendor/fundamental/src/filesystem/directory.c implements 'const char *path'
The function signature mismatch means the linker cannot find fun_filesystem_create_directory

FIX: Change filesystem.h line 46 to:
  ErrorResult fun_filesystem_create_directory(const char *path);
