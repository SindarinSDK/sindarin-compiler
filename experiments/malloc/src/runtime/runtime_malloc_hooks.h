#ifndef RUNTIME_MALLOC_HOOKS_H
#define RUNTIME_MALLOC_HOOKS_H

/*
 * Memory allocation hooks for Sindarin compiled programs.
 *
 * Platform-specific runtime hooking mechanisms:
 *
 * Linux:
 *   Uses plthook library to modify PLT/GOT entries at runtime.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * macOS:
 *   Uses Facebook's fishhook library for runtime symbol rebinding.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * Windows:
 *   Uses MinHook library for inline function hooking via trampolines.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * All platforms now use runtime hooking, which:
 *   - Catches allocations from both static and dynamic libraries
 *   - Requires no special linker flags (--wrap removed)
 *   - Provides consistent behavior across all platforms
 */

#include <stddef.h>

#ifdef SN_MALLOC_HOOKS

/*
 * No declarations needed - hooks are installed at runtime via constructors.
 * The hooking libraries (plthook, fishhook, MinHook) handle all redirection.
 */

#endif /* SN_MALLOC_HOOKS */

#endif /* RUNTIME_MALLOC_HOOKS_H */
