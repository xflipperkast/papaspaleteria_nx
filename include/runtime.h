#pragma once
#include "runtime_base.h"
uint32_t paleteria_cursor_snapshot(void);
bool apk64_graphics_refresh_cursor(void);
bool apk64_android_has_modal_dialog(void);
#ifdef __SWITCH__


/* module interface */


#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <switch.h>


struct Apk64Linker;

typedef enum Apk64ModuleState {
    APK64_MODULE_EMPTY = 0,
    APK64_MODULE_LOADED,
    APK64_MODULE_LAYOUT_READY,
    APK64_MODULE_RESERVED,
    APK64_MODULE_DYNAMIC_PARSED,
    APK64_MODULE_RELOCATING,
    APK64_MODULE_RELOCATED,
    APK64_MODULE_MAPPED,
    APK64_MODULE_CONSTRUCTED,
    APK64_MODULE_FINALIZED,
    APK64_MODULE_FAILED
} Apk64ModuleState;

typedef struct Apk64RelocStats {
    size_t relative;
    size_t glob_dat;
    size_t jump_slot;
    size_t abs64;
    size_t relr;
    size_t tls_dtpmod;
    size_t tls_dtprel;
    size_t tls_tprel;
    size_t tlsdesc;
    size_t irelative;
    size_t ifunc;
    size_t weak_zero;
    size_t unresolved;
    size_t unsupported;
} Apk64RelocStats;

typedef struct Apk64TlsInfo {
    bool present;
    uint64_t vaddr;
    size_t file_size;
    size_t mem_size;
    size_t alignment;
    size_t module_id;
    ptrdiff_t static_offset;
    bool registered;
} Apk64TlsInfo;

typedef struct Apk64DynamicInfo {
    /*
     * Keep ELF virtual addresses as the canonical dynamic symbol/string-table
     * locations. Readers select the writable backing view before mapping and
     * the runtime alias after Horizon code-memory mapping.
     */
    uint64_t strtab_vaddr;
    size_t strsz;

    uint64_t symtab_vaddr;
    size_t syment;
    size_t symbol_count;

    Elf64_Rela *rela;
    size_t rela_count;

    /* Android packed RELA (APS2) is decoded at dynamic-parse time into owned
     * Elf64_Rela entries so the normal relocation engine remains canonical. */
    Elf64_Rela *android_rela;
    size_t android_rela_count;

    Elf64_Rela *jmprel;
    size_t jmprel_count;

    Elf64_Xword *relr;
    size_t relr_count;
    bool relr_is_android;

    uint64_t init_vaddr;
    uint64_t init_array_vaddr;
    size_t init_array_count;
    uint64_t fini_vaddr;
    uint64_t fini_array_vaddr;
    size_t fini_array_count;

    uint64_t hash_vaddr;
    uint64_t gnu_hash_vaddr;

    uint64_t versym_vaddr;
    uint64_t verneed_vaddr;
    size_t verneed_count;
    uint64_t verdef_vaddr;
    size_t verdef_count;

    uint32_t *needed_name_offsets;
    size_t needed_count;
    uint32_t soname_offset;

    /* Dynamic-linker policy bits. Android/Bionic uses DT_FLAGS_1/DF_1_GLOBAL
     * to build the process-wide global lookup group, and DT_SYMBOLIC or
     * DT_FLAGS/DF_SYMBOLIC to request local-first binding for definitions in
     * this DSO. Keep the raw flag words so the linker can reproduce that
     * policy without title-specific exceptions. */
    uint64_t flags;
    uint64_t flags_1;
    bool symbolic;
} Apk64DynamicInfo;

typedef struct Apk64Module {
    char path[0x200];

    void *file_image;
    size_t file_size;

    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;

    uint64_t min_vaddr;
    uint64_t max_vaddr;
    size_t image_size;

    void *backing;
    void *runtime_base;
    uintptr_t load_bias;
    struct Apk64VmRange *reservation;
    bool mapped;
    bool post_map_relocations_applied;

    struct Apk64Linker *linker;

    Apk64DynamicInfo dynamic;
    Apk64TlsInfo tls;
    Apk64RelocStats reloc_stats;
    Apk64ModuleState state;
} Apk64Module;

void apk64_module_init(Apk64Module *module);
void apk64_module_unload(Apk64Module *module);

void *apk64_module_runtime_address(const Apk64Module *module, uint64_t elf_vaddr);
void *apk64_module_backing_address(const Apk64Module *module, uint64_t elf_vaddr);
bool apk64_module_backing_contains(const Apk64Module *module, uint64_t elf_vaddr, size_t size);
bool apk64_module_runtime_contains(const Apk64Module *module, uintptr_t runtime_address, size_t size);


/* dynamic interface */


#include <stdbool.h>



Apk64Result apk64_dynamic_parse(Apk64Module *module);
Apk64Result apk64_dynamic_run_constructors(Apk64Module *module);
Apk64Result apk64_dynamic_run_destructors(Apk64Module *module);

/* Enable verbose AArch64 constructor tracing around real-APK bring-up.
 * Kept off for the retained Stage 1-12 fixtures so diagnostics do not alter
 * their normal output or timing. */
void apk64_dynamic_set_constructor_trace(bool enabled);

Apk64Result apk64_dynamic_get_needed(const Apk64Module *module, size_t index, const char **out_name);
Apk64Result apk64_dynamic_get_soname(const Apk64Module *module, const char **out_name);


/* symbols interface */


#include <elf.h>
#include <stdint.h>



typedef struct Apk64SymbolVersion {
    const char *name;
    const char *provider; /* DT_VERNEED vn_file for imported versions. */
    uint32_t hash;
    bool hidden;
} Apk64SymbolVersion;

typedef struct Apk64ResolvedSymbol {
    uintptr_t address;
    const char *name;
    Apk64SymbolVersion version;
    const Apk64Module *provider;
    unsigned type;
    bool weak_zero;
} Apk64ResolvedSymbol;

Apk64Result apk64_symbol_get(const Apk64Module *module, size_t symbol_index,
                             const Elf64_Sym **out_symbol, const char **out_name);
Apk64Result apk64_symbol_resolve(Apk64Module *module, size_t symbol_index,
                                 Apk64ResolvedSymbol *out);
/* Re-resolve a relocation symbol after mapping without recording a second
 * compatibility bind or relocation failure statistic. Used by the deferred
 * IFUNC pass, which revisits already-accounted relocations. */
Apk64Result apk64_symbol_resolve_post_map(Apk64Module *module, size_t symbol_index,
                                          Apk64ResolvedSymbol *out);
Apk64Result apk64_symbol_probe(const Apk64Module *module, size_t symbol_index,
                               const char **out_name, bool *out_weak_zero);

Apk64Result apk64_symbol_version(const Apk64Module *module, size_t symbol_index,
                                   Apk64SymbolVersion *out_version);
Apk64Result apk64_symbol_find_defined_versioned_ex(const Apk64Module *module, const char *name,
                                                   const Apk64SymbolVersion *required_version,
                                                   uintptr_t *out_address, unsigned *out_type);
Apk64Result apk64_symbol_find_defined_versioned(const Apk64Module *module, const char *name,
                                                const Apk64SymbolVersion *required_version,
                                                uintptr_t *out_address);
Apk64Result apk64_symbol_find_tls_defined_versioned(const Apk64Module *module, const char *name,
                                                    const Apk64SymbolVersion *required_version,
                                                    size_t *out_offset);
Apk64Result apk64_symbol_resolve_tls(const Apk64Module *requester, size_t symbol_index,
                                     const Apk64Module **out_provider, size_t *out_offset,
                                     Apk64SymbolVersion *out_version);
Apk64Result apk64_symbol_find_defined(const Apk64Module *module, const char *name,
                                      uintptr_t *out_address);


/* linker interface */


#include <stdbool.h>
#include <stddef.h>




typedef struct Apk64LinkerNode Apk64LinkerNode;

typedef struct Apk64Linker {
    Apk64LinkerNode **nodes;
    size_t node_count;
    size_t node_capacity;
    size_t compat_bind_count;
    char library_dir[0x200];
} Apk64Linker;

void apk64_linker_init(Apk64Linker *linker, const char *library_dir);
void apk64_linker_unload(Apk64Linker *linker);

/* Load one root module plus its complete DT_NEEDED graph. */
Apk64Result apk64_linker_load(Apk64Linker *linker, const char *path, Apk64Module **out_root);

/* Stage 14 Java Activity bootstrap may know the complete native root set before
 * Java executes the corresponding System.loadLibrary() calls. Discover the
 * union graph and reserve guest addresses only. Relocation, mapping,
 * .init/.init_array and JNI_OnLoad remain tied to the actual load operation so
 * unrelated RTLD_LOCAL roots cannot leak into one another's symbol scope. */
Apk64Result apk64_linker_prepare_roots(Apk64Linker *linker,
                                       const char *const *paths,
                                       size_t path_count,
                                       Apk64Module **out_primary_root);

/* Run one already-mapped module's dependency constructors exactly once. Safe to
 * call repeatedly; dependencies are constructed before the requested module.
 * A constructor failure is latched: later calls return it without re-running. */
Apk64Result apk64_linker_construct_module(Apk64Linker *linker, const Apk64Module *module);

/* True once the module's DT_INIT/DT_INIT_ARRAY have completed, i.e. once the
 * module is "loaded" in the Android-observable dlopen() sense. A module that
 * prepare_roots() mapped with deferred constructors reports false. */
bool apk64_linker_module_constructed(const Apk64Linker *linker, const Apk64Module *module);

/* Eager convenience path retained for NativeActivity and host tests: prepare the
 * complete graph, then construct every requested root in dependency order. */
Apk64Result apk64_linker_load_roots(Apk64Linker *linker,
                                    const char *const *paths,
                                    size_t path_count,
                                    Apk64Module **out_primary_root);

/* Android System.loadLibrary() may request another root after the initial Java
 * bootstrap graph is already live. Add one lib*.so root to the existing graph
 * without relocating/reconstructing modules that are already loaded. */
Apk64Result apk64_linker_load_library(Apk64Linker *linker,
                                      const char *soname,
                                      Apk64Module **out_module,
                                      bool *out_newly_loaded);

/* Resolve one undefined guest symbol against the loaded dependency scope. */
Apk64Result apk64_linker_resolve_import_versioned(const Apk64Module *requester,
                                                  const char *name,
                                                  const Apk64SymbolVersion *version,
                                                  uintptr_t *out_address,
                                                  const Apk64Module **out_provider);
Apk64Result apk64_linker_resolve_import(const Apk64Module *requester,
                                        const char *name,
                                        uintptr_t *out_address,
                                        const Apk64Module **out_provider);

/* Side-effect-free import lookup used by relocation preflight. Unlike the
 * normal resolver it does not increment compatibility bind counters or emit
 * per-bind success logs. */
Apk64Result apk64_linker_resolve_tls_import_versioned(const Apk64Module *requester,
                                                      const char *name,
                                                      const Apk64SymbolVersion *version,
                                                      const Apk64Module **out_provider,
                                                      size_t *out_offset);

Apk64Result apk64_linker_probe_import_versioned(const Apk64Module *requester,
                                                const char *name,
                                                const Apk64SymbolVersion *version,
                                                uintptr_t *out_address,
                                                const Apk64Module **out_provider);
Apk64Result apk64_linker_probe_import(const Apk64Module *requester,
                                      const char *name,
                                      uintptr_t *out_address,
                                      const Apk64Module **out_provider);

/* dlsym(handle, name) searches the handle's local dependency group, not just
 * the root object. Version is NULL for ordinary dlsym and non-NULL for future
 * dlvsym-style lookups. */
Apk64Result apk64_linker_find_in_handle(const Apk64Linker *linker,
                                        const Apk64Module *module,
                                        const char *name,
                                        const Apk64SymbolVersion *version,
                                        uintptr_t *out_address,
                                        const Apk64Module **out_provider);
Apk64Result apk64_linker_find_global(const Apk64Linker *linker,
                                     const char *name,
                                     const Apk64SymbolVersion *version,
                                     uintptr_t *out_address,
                                     const Apk64Module **out_provider);

/* Stable dlopen handle lifetime. Java/System.loadLibrary roots are pinned;
 * native dlopen references are counted and dlclose runs eligible destructors. */
Apk64Result apk64_linker_acquire_module(Apk64Linker *linker, const Apk64Module *module);
Apk64Result apk64_linker_release_module(Apk64Linker *linker, const Apk64Module *module);
Apk64Result apk64_linker_pin_module(Apk64Linker *linker, const Apk64Module *module);
bool apk64_linker_module_handle_live(const Apk64Linker *linker, const Apk64Module *module);

/* Resolve only against Bionic's process-wide guest global group. Used when a
 * relocation references a default-visibility symbol defined by the requester:
 * such a definition is preemptible unless the DSO requested symbolic binding
 * or the symbol is STV_PROTECTED. */
Apk64Result apk64_linker_probe_global_versioned(const Apk64Module *requester,
                                                const char *name,
                                                const Apk64SymbolVersion *version,
                                                uintptr_t *out_address,
                                                const Apk64Module **out_provider);
Apk64Result apk64_linker_probe_global(const Apk64Module *requester,
                                      const char *name,
                                      uintptr_t *out_address,
                                      const Apk64Module **out_provider);

/* dlopen(..., RTLD_GLOBAL) promotes an already-loaded local root so its symbols
 * are visible to later loads. DF_1_GLOBAL modules are promoted automatically. */
Apk64Result apk64_linker_promote_global(Apk64Linker *linker,
                                        const Apk64Module *module,
                                        const char *reason);

size_t apk64_linker_module_count(const Apk64Linker *linker);
size_t apk64_linker_compat_bind_count(const Apk64Linker *linker);
const Apk64Module *apk64_linker_module_at(const Apk64Linker *linker, size_t index);


/* loader interface */



/* Generic single-module loader retained for Stage 1/2 regression tests. */
Apk64Result apk64_loader_load(const char *path, Apk64Module *module);

/* Stage 1 regression path: succeeds only when the guest requires no relocations. */
Apk64Result apk64_loader_load_stage1(const char *path, Apk64Module *module);

Apk64Result apk64_loader_find_export(const Apk64Module *module, const char *name, void **out_address);

/* Stage 3 linker building blocks. These do not run constructors implicitly. */
Apk64Result apk64_loader_prepare(const char *path, Apk64Module *module);
/* Header-only classification for speculative Java native-library candidates. */
Apk64Result apk64_loader_probe_shared_object(const char *path, bool *out_shared);
Apk64Result apk64_loader_relocate(Apk64Module *module);
Apk64Result apk64_loader_patch_system_registers(Apk64Module *module);
Apk64Result apk64_loader_map(Apk64Module *module);
Apk64Result apk64_loader_resolve_ifuncs(Apk64Module *module);
Apk64Result apk64_loader_construct(Apk64Module *module);


/* relocations interface */



Apk64Result apk64_apply_relr(Apk64Module *module);
Apk64Result apk64_preflight_relocations(Apk64Module *module, size_t *out_unique_missing);
Apk64Result apk64_apply_relocations(Apk64Module *module);
Apk64Result apk64_apply_post_map_relocations(Apk64Module *module);


#endif
