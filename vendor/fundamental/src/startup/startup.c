// Fundamental Library - Runtime Initialization
// Provides minimal startup support for zero-stdlib builds
// This module ensures GCC's constructor initialization works without CRT

// Provide __main to satisfy GCC when using -ffreestanding/-nostdlib
// Called automatically by GCC-generated code at program startup
void __main(void) {
  // No initialization needed for fundamental library
  // This stub prevents linker errors when building without CRT
}
