/* MinGW WinLibs headers are missing FILE_WRITE_FLAGS and FILE_FLUSH_MODE
 * which are required by ioringapi.h. Define them here before the include. */
#ifndef FILE_WRITE_FLAGS
typedef UINT32 FILE_WRITE_FLAGS;
#define FILE_WRITE_FLAGS_NONE ((FILE_WRITE_FLAGS)0)
#endif

#ifndef FILE_FLUSH_MODE
typedef UINT32 FILE_FLUSH_MODE;
#define FILE_FLUSH_MODE_DEFAULT ((FILE_FLUSH_MODE)0)
#endif
