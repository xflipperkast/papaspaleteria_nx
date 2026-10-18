#pragma once
#include <stdlib.h>
#include <stdio.h>


/* aaudio interface */


#include <stddef.h>
#include <stdint.h>

/* Android NDK AAudio (libaaudio.so, API 26+) output streams. Streams are mixed
 * into the shared APK64 audio output; data callbacks run on the audio worker
 * thread, which carries guest TLS. Input streams are reported unavailable. */

typedef struct AAudioStreamStruct AAudioStream;
typedef struct AAudioStreamBuilderStruct AAudioStreamBuilder;
typedef int32_t aaudio_result_t;
typedef int32_t aaudio_stream_state_t;
typedef int32_t aaudio_data_callback_result_t;
typedef aaudio_data_callback_result_t (*AAudioStream_dataCallback)(AAudioStream *stream, void *user_data,
                                                                     void *audio_data, int32_t num_frames);
typedef void (*AAudioStream_errorCallback)(AAudioStream *stream, void *user_data, aaudio_result_t error);

const char *AAudio_convertResultToText(aaudio_result_t result);
const char *AAudio_convertStreamStateToText(aaudio_stream_state_t state);
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **builder);

void AAudioStreamBuilder_setDeviceId(AAudioStreamBuilder *b, int32_t device_id);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b, int32_t sample_rate);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b, int32_t channel_count);
void AAudioStreamBuilder_setSamplesPerFrame(AAudioStreamBuilder *b, int32_t samples_per_frame);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b, int32_t format);
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *b, int32_t sharing_mode);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b, int32_t direction);
void AAudioStreamBuilder_setBufferCapacityInFrames(AAudioStreamBuilder *b, int32_t frames);
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder *b, int32_t mode);
void AAudioStreamBuilder_setUsage(AAudioStreamBuilder *b, int32_t usage);
void AAudioStreamBuilder_setContentType(AAudioStreamBuilder *b, int32_t content_type);
void AAudioStreamBuilder_setInputPreset(AAudioStreamBuilder *b, int32_t preset);
void AAudioStreamBuilder_setAllowedCapturePolicy(AAudioStreamBuilder *b, int32_t policy);
void AAudioStreamBuilder_setSessionId(AAudioStreamBuilder *b, int32_t session_id);
void AAudioStreamBuilder_setPrivacySensitive(AAudioStreamBuilder *b, _Bool sensitive);
void AAudioStreamBuilder_setChannelMask(AAudioStreamBuilder *b, uint32_t mask);
void AAudioStreamBuilder_setPackageName(AAudioStreamBuilder *b, const char *name);
void AAudioStreamBuilder_setAttributionTag(AAudioStreamBuilder *b, const char *tag);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, AAudioStream_dataCallback cb, void *user_data);
void AAudioStreamBuilder_setFramesPerDataCallback(AAudioStreamBuilder *b, int32_t frames);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, AAudioStream_errorCallback cb, void *user_data);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **stream);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b);

aaudio_result_t AAudioStream_release(AAudioStream *s);
aaudio_result_t AAudioStream_close(AAudioStream *s);
aaudio_result_t AAudioStream_requestStart(AAudioStream *s);
aaudio_result_t AAudioStream_requestPause(AAudioStream *s);
aaudio_result_t AAudioStream_requestFlush(AAudioStream *s);
aaudio_result_t AAudioStream_requestStop(AAudioStream *s);
aaudio_stream_state_t AAudioStream_getState(AAudioStream *s);
aaudio_result_t AAudioStream_waitForStateChange(AAudioStream *s, aaudio_stream_state_t input_state,
                                                aaudio_stream_state_t *next_state, int64_t timeout_nanos);
int32_t AAudioStream_read(AAudioStream *s, void *buffer, int32_t num_frames, int64_t timeout_nanos);
int32_t AAudioStream_write(AAudioStream *s, const void *buffer, int32_t num_frames, int64_t timeout_nanos);
aaudio_result_t AAudioStream_setBufferSizeInFrames(AAudioStream *s, int32_t frames);
int32_t AAudioStream_getBufferSizeInFrames(AAudioStream *s);
int32_t AAudioStream_getFramesPerBurst(AAudioStream *s);
int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream *s);
int32_t AAudioStream_getFramesPerDataCallback(AAudioStream *s);
int32_t AAudioStream_getXRunCount(AAudioStream *s);
int32_t AAudioStream_getSampleRate(AAudioStream *s);
int32_t AAudioStream_getChannelCount(AAudioStream *s);
int32_t AAudioStream_getSamplesPerFrame(AAudioStream *s);
int32_t AAudioStream_getDeviceId(AAudioStream *s);
int32_t AAudioStream_getFormat(AAudioStream *s);
int32_t AAudioStream_getSharingMode(AAudioStream *s);
int32_t AAudioStream_getPerformanceMode(AAudioStream *s);
int32_t AAudioStream_getDirection(AAudioStream *s);
int64_t AAudioStream_getFramesWritten(AAudioStream *s);
int64_t AAudioStream_getFramesRead(AAudioStream *s);
int32_t AAudioStream_getSessionId(AAudioStream *s);
aaudio_result_t AAudioStream_getTimestamp(AAudioStream *s, int32_t clockid, int64_t *frame_position,
                                          int64_t *time_nanoseconds);
int32_t AAudioStream_getUsage(AAudioStream *s);
int32_t AAudioStream_getContentType(AAudioStream *s);
int32_t AAudioStream_getInputPreset(AAudioStream *s);
int32_t AAudioStream_getAllowedCapturePolicy(AAudioStream *s);
_Bool AAudioStream_isPrivacySensitive(AAudioStream *s);
uint32_t AAudioStream_getChannelMask(AAudioStream *s);
_Bool AAudioStream_isMMapUsed(AAudioStream *s);

/* Runtime side. */
/* AudioTrack streaming stop drains frames already queued, then stops consuming
 * new writes. Native AAudio requestStop retains its discard contract. */
aaudio_result_t apk64_aaudio_stop_after_queued_frames(AAudioStream *s);
size_t apk64_aaudio_mix_s16(int16_t *io_interleaved, size_t frames);
void apk64_aaudio_reset(void);


/* result interface */


typedef enum Apk64Result {
    APK64_OK = 0,
    APK64_ERR_ARGUMENT = -1,
    APK64_ERR_IO = -2,
    APK64_ERR_NO_MEMORY = -3,
    APK64_ERR_ELF_MAGIC = -4,
    APK64_ERR_ELF_CLASS = -5,
    APK64_ERR_ELF_ENDIAN = -6,
    APK64_ERR_ELF_MACHINE = -7,
    APK64_ERR_ELF_TYPE = -8,
    APK64_ERR_ELF_BOUNDS = -9,
    APK64_ERR_ELF_NO_LOAD = -10,
    APK64_ERR_VM_RESERVE = -11,
    APK64_ERR_VM_MAP = -12,
    APK64_ERR_VM_PERMISSIONS = -13,
    APK64_ERR_SYMBOL_TABLE = -14,
    APK64_ERR_SYMBOL_NOT_FOUND = -15,
    APK64_ERR_RELOCATIONS_REQUIRED = -16,
    APK64_ERR_TEST_FAILED = -17,
    APK64_ERR_DYNAMIC_MISSING = -18,
    APK64_ERR_DYNAMIC_INVALID = -19,
    APK64_ERR_RELOCATION_FORMAT = -20,
    APK64_ERR_RELOCATION_UNSUPPORTED = -21,
    APK64_ERR_RELOCATION_TARGET = -22,
    APK64_ERR_SYMBOL_INDEX = -23,
    APK64_ERR_SYMBOL_UNRESOLVED = -24,
    APK64_ERR_RELR_INVALID = -25,
    APK64_ERR_CONSTRUCTOR = -26,
    APK64_ERR_DEPENDENCY_NOT_FOUND = -27,
    APK64_ERR_DEPENDENCY_INVALID = -28,
    APK64_ERR_APK_INVALID = -29,
    APK64_ERR_APK_UNSUPPORTED = -30,
    APK64_ERR_MANIFEST_INVALID = -31,
    APK64_ERR_ABI_NOT_FOUND = -32,
    APK64_ERR_CANCELLED = -33
} Apk64Result;

const char *apk64_result_string(Apk64Result result);


/* jni interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* JVMS 4.3.3: up to 255 parameter slots, with J/D using two slots and
 * instance methods reserving one slot for the receiver. Dalvik invoke/range
 * carries the same maximum in its eight-bit argument-word count. */
#define APK64_JNI_MAX_PARAMETER_SLOTS 255u

/* Stage 6 JNI core.
 *
 * APK64 intentionally exposes the real C JNI pointer-to-function-table ABI to
 * guest AArch64 code. The concrete object model stays private to the host.
 */

enum {
    APK64_JNI_OK        = 0,
    APK64_JNI_ERR       = -1,
    APK64_JNI_EDETACHED = -2,
    APK64_JNI_EVERSION  = -3,
    APK64_JNI_ENOMEM    = -4,
    APK64_JNI_EEXIST    = -5,
    APK64_JNI_EINVAL    = -6,
};

enum {
    APK64_JNI_VERSION_1_1 = 0x00010001,
    APK64_JNI_VERSION_1_2 = 0x00010002,
    APK64_JNI_VERSION_1_4 = 0x00010004,
    APK64_JNI_VERSION_1_6 = 0x00010006,
};

typedef union Apk64JniValue {
    uint8_t z;
    int8_t b;
    uint16_t c;
    int16_t s;
    int32_t i;
    int64_t j;
    float f;
    double d;
    void *l;
} Apk64JniValue;

typedef Apk64JniValue (*Apk64JniMethodCallback)(void *env_handle,
                                                 void *object_token,
                                                 const Apk64JniValue *args,
                                                 size_t arg_count,
                                                 void *user_data);

/* Optional lazy class resolver used by Java-Activity package bootstraps. A
 * resolver answers whether a slash-form JNI class name is defined by the
 * package and may return its direct superclass. It describes metadata only;
 * APK64 still does not execute DEX bytecode here. */
typedef Apk64Result (*Apk64JniClassResolver)(const char *class_name,
                                              char *out_super_name,
                                              size_t out_super_name_cap,
                                              bool *out_found,
                                              void *user_data);

/* Optional lazy Java-method resolver. This complements the class resolver for
 * package DEX classes whose methods are requested from native code through
 * Get(Method|StaticMethod)ID. A successful resolver returns the normal JNI
 * callback and opaque callback data to register for that exact descriptor. */
typedef Apk64Result (*Apk64JniMethodResolver)(const char *class_name,
                                               const char *method_name,
                                               const char *signature,
                                               bool is_static,
                                               Apk64JniMethodCallback *out_callback,
                                               void **out_callback_user_data,
                                               bool *out_found,
                                               void *user_data);

/* Reset and initialize one process-global fake VM. The main thread is attached
 * by apk64_jni_init so JNI_OnLoad can immediately call GetEnv. */
Apk64Result apk64_jni_init(void);
void apk64_jni_reset(void);

/* This is the exact pointer value that should be passed as JavaVM* to a guest
 * JNI_OnLoad or another JNI-aware entry point. */
void *apk64_jni_vm(void);

/* Install/clear a lazy package-backed class resolver. The callback is invoked
 * after registry misses and to reconcile classes explicitly registered with a
 * provisional framework superclass. */
void apk64_jni_set_class_resolver(Apk64JniClassResolver resolver, void *user_data);

/* Extended class resolver: as Apk64JniClassResolver, and also returns the
 * class's directly implemented interfaces (slash form, '\n'-separated). APK64
 * links them when the class is registered or reconciled, so instance-of,
 * check-cast and interface assignability follow the class_def (JLS 4.10.2).
 * When installed it replaces the plain resolver. */
typedef Apk64Result (*Apk64JniClassResolverEx)(const char *class_name,
                                                char *out_super_name,
                                                size_t out_super_name_cap,
                                                char *out_interfaces,
                                                size_t out_interfaces_cap,
                                                bool *out_found,
                                                void *user_data);
void apk64_jni_set_class_resolver_ex(Apk64JniClassResolverEx resolver, void *user_data);
void apk64_jni_set_method_resolver(Apk64JniMethodResolver resolver, void *user_data);

/* Optional declared-member materializer for package classes. Reflection over
 * a class (Class.getDeclaredMethods/Fields/Constructors, getMethods, ...)
 * invokes it once per class so the model lists every member the class
 * declares, not only members already resolved through JNI or bytecode. */
typedef Apk64Result (*Apk64JniClassMemberMaterializer)(void *env, const char *class_name,
                                                       void *user_data);
void apk64_jni_set_class_member_materializer(Apk64JniClassMemberMaterializer materializer,
                                             void *user_data);

/* Optional package-class initializer used by JNI member-ID lookups. JNI
 * GetMethodID/GetStaticMethodID and GetFieldID/GetStaticFieldID trigger class
 * initialization before returning an ID. Framework classes can return APK64_OK
 * without doing work; package implementations should execute the class' normal
 * <clinit> exactly once. */
typedef Apk64Result (*Apk64JniClassInitializer)(void *env, const char *class_name,
                                                void *user_data);
void apk64_jni_set_class_initializer(Apk64JniClassInitializer initializer,
                                     void *user_data);
/* Explicitly initialize a resolved Java class through the installed runtime
 * class-initializer hook. Class.forName(..., true, ...) uses this; ordinary
 * ClassLoader.loadClass remains resolution-only. */
Apk64Result apk64_jni_initialize_class_host(void *env_handle,
                                            const char *class_name);
/* True when class_name (lazily resolved) was defined only from the API-30
 * platform declaration table, i.e. it has no framework model. */
bool apk64_jni_class_is_platform_inert_host(const char *class_name);
/* Declares on a framework-modelled class (no DEX class_def) the interface
 * methods its model implements, so reflection lists them as the platform
 * class declares them. Dispatch is unchanged. */
Apk64Result apk64_jni_declare_modelled_interface_methods_host(const char *class_name);
/* True when JNI resolution from class_name reaches the member (the class,
 * its superclasses and, for methods, their interfaces). */
bool apk64_jni_class_resolves_member_host(const char *class_name, const char *name,
                                          const char *signature, bool is_static, bool is_method);
/* True when class_name itself (not a superclass) declares the member. */
bool apk64_jni_class_declares_member_host(const char *class_name, const char *name,
                                          const char *signature, bool is_static, bool is_method);
size_t apk64_jni_native_binding_count(void);
Apk64Result apk64_jni_find_native_binding(const char *class_name, const char *name,
                                           const char *signature, void **out_fn,
                                           bool *out_found);

/* Build a standard JNI Java_* export name. Only Java identifiers/descriptors are
 * mangled; the literal Java_ prefix and JNI separators remain literal.
 * long_name=false builds Java_<class>_<method>; true appends the mangled
 * parameter descriptor after the literal __ overload separator. */
bool apk64_jni_build_export_name(const char *class_name, const char *method_name,
                                  const char *signature, bool long_name,
                                  char *out, size_t out_cap);

/* Host-side dispatch used by the bounded DEX interpreter and runtime bridges.
 * It resolves an already-modeled method first, then consults the optional lazy
 * Java-method resolver so package/core DEX methods can be called back from
 * native code. out_found=false is a normal miss. */
Apk64Result apk64_jni_invoke_method_host(void *env_handle,
                                         void *receiver_ref,
                                         const char *class_name,
                                         const char *method_name,
                                         const char *signature,
                                         bool is_static,
                                         const Apk64JniValue *args,
                                         Apk64JniValue *out_result,
                                         bool *out_found);

/* Query only the already-modeled JNI/framework method registry. This never
 * invokes the lazy Java-method resolver and is used by the DEX bridge to
 * prove that a package helper's transitive platform calls are safe before
 * exposing that helper to guest JNI. */
Apk64Result apk64_jni_has_method_host(const char *class_name,
                                      const char *method_name,
                                      const char *signature,
                                      bool is_static,
                                      bool *out_found);
/* Force the package-backed declared-member materializer for one class. Core
 * Java facilities such as AtomicReferenceFieldUpdater need declared volatile
 * fields even when no bytecode iget/iput has touched them yet. */
Apk64Result apk64_jni_materialize_class_members_host(void *env_handle,
                                                      const char *class_name);

/* Generic class/model registration used by Stage 6 and later Android framework
 * layers. Names and descriptors use normal JNI slash/descriptors. */
Apk64Result apk64_jni_register_class(const char *name, const char *super_name);
/* Register an implemented/extended Java interface relationship without
 * corrupting the concrete superclass chain. This is used by framework models
 * whose JNI/Dex dispatch may target the interface owner (for example Map on a
 * HashMap receiver). Both classes must already be registered. */
Apk64Result apk64_jni_register_interface(const char *class_name, const char *interface_name);
/* Diagnostics: visits every registered class (method_name/signature NULL) and
 * every registered method. superclass may be NULL. For class entries is_static
 * reports whether the class is an interface. Coverage audits only. */
typedef void (*Apk64JniRegistryVisitor)(const char *class_name, const char *superclass,
                                        const char *method_name, const char *signature,
                                        bool is_static, void *user_data);
void apk64_jni_visit_registry(Apk64JniRegistryVisitor visitor, void *user_data);
/* Diagnostics: visits every registered field. Coverage audits only. */
typedef void (*Apk64JniFieldVisitor)(const char *class_name, const char *field_name,
                                     const char *signature, bool is_static, void *user_data);
void apk64_jni_visit_registry_fields(Apk64JniFieldVisitor visitor, void *user_data);
/* Link the interfaces each host-modeled platform class declares in the API-30
 * table (for example Messenger implements Parcelable). Returns links added. */
size_t apk64_jni_link_platform_api_interfaces(void);
/* Declare an abstract method of a modeled Java interface (the class is marked
 * as an interface). JNI calls through the method ID dispatch on the receiver's
 * runtime class: package DEX implementation, framework model, or proxy
 * handler; a receiver without an implementation raises AbstractMethodError. */
Apk64Result apk64_jni_register_interface_method(const char *interface_name, const char *name,
                                                const char *signature);
/* java.lang.reflect.Proxy.newProxyInstance core: defines (or reuses) the proxy
 * class for the ordered Class[] interface list, allocates an instance whose
 * Proxy.h field holds handler_ref, and returns it as a local reference.
 * Java-level failures (null handler/element, non-interface, repeated
 * interface) return APK64_OK with the matching exception pending and
 * *out_proxy_ref == NULL. Every non-static interface method, and Object's
 * hashCode/equals/toString, invoked on the proxy is routed to
 * InvocationHandler.invoke(proxy, Method, Object[]) with boxed arguments. */
Apk64Result apk64_jni_proxy_new_instance_host(void *env_handle, void *interfaces_ref,
                                              void *handler_ref, void **out_proxy_ref);
/* Proxy.getInvocationHandler: local reference, or NULL for a non-proxy. */
void *apk64_jni_proxy_get_handler_host(void *env_handle, void *proxy_ref);
bool apk64_jni_is_proxy_class_host(void *class_ref);
/* Describe a java.lang.reflect.Method created by the JNI layer (proxy dispatch
 * or ToReflectedMethod). Returns false for other Method objects. */
bool apk64_jni_reflected_method_info_host(void *method_ref, char *out_owner, size_t owner_cap,
                                          char *out_name, size_t name_cap,
                                          char *out_signature, size_t signature_cap,
                                          bool *out_is_static, bool *out_owner_is_interface);
/* Declared access flags (JVMS/DEX encoding) of a reflected Method and whether
 * its declaring type is an interface (Method.getModifiers/isDefault). */
bool apk64_jni_reflected_method_modifiers_host(void *method_ref, uint32_t *out_access_flags,
                                               bool *out_declared_in_interface);
/* Enumerate the registered method model as reflected Method/Constructor objects.
 * Constructors are selected by <init>; normal method enumeration excludes them. */
void *apk64_jni_class_get_reflected_methods_host(void *env_handle, void *class_ref,
                                                  bool declared_only, bool constructors_only,
                                                  bool public_only);
/* One reflected Method/Constructor ("<init>") by name and parameter descriptor
 * "(...)", or NULL when absent (Class.get[Declared]Method/Constructor). */
void *apk64_jni_class_get_reflected_method_host(void *env_handle, void *class_ref,
                                                 const char *name, const char *params,
                                                 bool declared_only, bool public_only);
/* False for a constructor of an interface or abstract class. */
bool apk64_jni_reflected_constructor_instantiable_host(void *method_ref);
/* Local Class reference for the direct superclass, or NULL for Object/primitive. */
void *apk64_jni_class_get_superclass_host(void *env_handle, void *class_ref);
/* Java/JNI Field reflection over the same registered field model used by
 * GetFieldID/GetStaticFieldID. */
void *apk64_jni_class_get_reflected_field_host(void *env_handle, void *class_ref,
                                                const char *name, bool declared_only);
void *apk64_jni_class_get_reflected_fields_host(void *env_handle, void *class_ref,
                                                 bool declared_only);
bool apk64_jni_reflected_field_info_host(void *field_ref, char *out_owner, size_t owner_cap,
                                         char *out_name, size_t name_cap,
                                         char *out_signature, size_t signature_cap,
                                         bool *out_is_static);
/* sun.misc.Unsafe storage access. A field "offset" is an opaque handle to a
 * reflected field (ART offsets are opaque to callers too); array offsets follow
 * ART's layout (base + index * scale) so caller arithmetic is meaningful.
 * kind is the JNI type letter of the slot ('Z','B','C','S','I','J','F','D' or
 * 'L' for references, which hold object tokens). */
typedef struct Apk64UnsafeSlot {
    void *address;
    char kind;
} Apk64UnsafeSlot;
bool apk64_jni_unsafe_field_offset_host(void *field_ref, bool want_static, int64_t *out_offset);
bool apk64_jni_unsafe_array_layout_host(void *class_ref, int32_t *out_base, int32_t *out_scale);
bool apk64_jni_unsafe_slot_host(void *object_ref, int64_t offset, Apk64UnsafeSlot *out_slot);
/* Guest ART metadata views translate opaque field offsets and clone/rebind
 * reflection members; none of these helpers expose a host address. */
void *apk64_jni_unsafe_offset_field_host(void *env_handle, int64_t offset);
void *apk64_jni_clone_reflected_member_host(void *env_handle, void *member_ref);
bool apk64_jni_rebind_reflected_member_host(void *target_ref, void *member_ref);
void *apk64_jni_reflected_method_by_signature_host(void *env_handle,const char *owner,
    const char *name,const char *signature,bool is_static);
/* Return the java.lang.Class object corresponding to one JNI descriptor. This
 * includes primitive Class objects without making primitive names valid
 * FindClass inputs. */
void *apk64_jni_class_ref_for_descriptor_host(void *env_handle, const char *signature);
/* Register a temporary superclass relationship that package DEX metadata may
 * later replace in place. Intended for manifest-declared app classes that must
 * exist before their DEX hierarchy is available. */
Apk64Result apk64_jni_register_class_provisional(const char *name, const char *super_name);
Apk64Result apk64_jni_register_method(const char *class_name,
                                      const char *name,
                                      const char *signature,
                                      bool is_static,
                                      Apk64JniMethodCallback callback,
                                      void *user_data);
Apk64Result apk64_jni_register_field(const char *class_name,
                                     const char *name,
                                     const char *signature,
                                     bool is_static,
                                     Apk64JniValue initial_value);

size_t apk64_jni_class_count(void);
size_t apk64_jni_method_count(void);
size_t apk64_jni_field_count(void);
/* Host-side helpers for generic framework layers. These helpers create normal
 * JNI references/objects; guest code still sees only the standard JNI ABI. */
void *apk64_jni_current_env(void);
Apk64Result apk64_jni_attach_current_thread_host(void **out_env);
void apk64_jni_detach_current_thread_host(void);
void *apk64_jni_find_class_ref(void *env_handle, const char *name);
/* Guest FindClass derives its loader from the current Java/native call. Host
 * dispatch enters application-loader context around JNI_OnLoad and package
 * native methods; freshly attached native threads otherwise see only the
 * system/bootstrap namespace, matching Android JNI behavior. */
Apk64Result apk64_jni_push_application_class_loader_host(void *env_handle);
void apk64_jni_pop_application_class_loader_host(void *env_handle);
/* Automatic JNI local-reference frames for transitions into guest native
 * methods. The returned reference from PopLocalFrame is promoted into the
 * previous frame, while all other locals from the native call are invalidated. */
Apk64Result apk64_jni_push_local_frame_host(void *env_handle, int32_t capacity);
void *apk64_jni_pop_local_frame_host(void *env_handle, void *result_ref);
void *apk64_jni_get_object_class_host(void *env_handle, void *object_ref);
void *apk64_jni_new_local_object(void *env_handle, const char *class_name);
void *apk64_jni_new_local_string_utf(void *env_handle, const char *text);
void *apk64_jni_new_local_string_utf16(void *env_handle, const uint16_t *chars, int32_t length);
/* Exact Java String length in UTF-16 code units, or -1 for a null/non-String
 * reference. This mirrors JNI GetStringLength semantics for framework code. */
int32_t apk64_jni_string_length_host(void *string_ref);
/* malloc-owned copy of a String's UTF-16 code units (caller frees); NULL for a
 * null/non-String reference or OOM. */
uint16_t *apk64_jni_string_utf16_dup_host(void *string_ref, int32_t *out_length);
/* Read one Java UTF-16 code unit exactly as String.charAt() does. */
bool apk64_jni_string_char_at_host(void *string_ref, int32_t index, uint16_t *out_char);
/* Java Class.isAssignableFrom semantics: true when source can be assigned to target. */
bool apk64_jni_class_is_assignable_from_host(void *target_class_ref, void *source_class_ref);
/* True iff the supplied java/lang/Class reference represents an interface. */
bool apk64_jni_class_is_interface_host(void *class_ref);
void *apk64_jni_class_get_interfaces_host(void *env_handle, void *class_ref);
/* True when a java.lang.Class object is defined by APK64's application
 * ClassLoader rather than the bootstrap/framework namespace. */
bool apk64_jni_class_uses_application_loader_host(void *class_ref);
/* File-backed dynamic classes retain their explicit defining loader. The
 * current registry has one class per binary name; callers reject conflicting
 * definitions rather than aliasing classes across loaders. */
Apk64Result apk64_jni_class_set_defining_loader_host(void *class_ref, void *loader_ref);
bool apk64_jni_class_initiated_by_host(void *class_ref, void *loader_ref);
Apk64Result apk64_jni_class_record_initiator_host(void *class_ref, void *loader_ref);
void *apk64_jni_class_defining_loader_host(void *env_handle, void *class_ref, void *application_loader);
void *apk64_jni_find_registered_class_host(void *env_handle, const char *name);
void *apk64_jni_find_boot_class_ref_host(void *env_handle, const char *name);
/* Initialize an already allocated java/lang/String object token with UTF-16
 * contents. This models Java String constructors, which must preserve the
 * identity created by DEX new-instance rather than returning a replacement. */
Apk64Result apk64_jni_string_token_assign_utf16(void *object_token,
                                                 const uint16_t *chars,
                                                 int32_t length);
void *apk64_jni_new_local_ref_host(void *env_handle, void *ref_value);
void *apk64_jni_new_local_from_token(void *env_handle, void *object_token);
void *apk64_jni_new_global_ref_host(void *env_handle, void *ref_value);
void apk64_jni_delete_local_ref_host(void *env_handle, void *ref_value);
void apk64_jni_delete_global_ref_host(void *env_handle, void *ref_value);
bool apk64_jni_string_utf_copy(void *string_ref, char *out, size_t out_size);
/* Return a malloc-owned modified UTF-8 copy of a modeled Java String.
 * Caller must free(). Returns NULL for null/non-String references or OOM. */
char *apk64_jni_string_utf_dup(void *string_ref);
Apk64Result apk64_jni_set_object_host_data(void *object_ref, void *host_data);
Apk64Result apk64_jni_set_token_host_data(void *object_token, void *host_data);
void *apk64_jni_get_object_host_data(void *object_ref);
void *apk64_jni_get_token_host_data(void *object_token);
bool apk64_jni_same_object_host(void *a_ref, void *b_ref);
/* Host-side monitor operations used by the DEX interpreter and java.lang.Object
 * synchronization methods. Return JNI_OK/JNI_ERR style status and leave a
 * modeled Java exception pending on ownership violations. */
int32_t apk64_jni_monitor_enter_host(void *env_handle, void *object_ref);
/* DEX watchdog helper: identical monitor semantics, but bounds only the host wait
 * so the interpreter's existing wall-clock watchdog cannot be bypassed by a
 * contended monitor. timeout_ms==UINT32_MAX means wait indefinitely. */
int32_t apk64_jni_monitor_enter_timed_host(void *env_handle, void *object_ref, uint32_t timeout_ms);
int32_t apk64_jni_monitor_exit_host(void *env_handle, void *object_ref);
int32_t apk64_jni_monitor_notify_host(void *env_handle, void *object_ref, bool notify_all);
/* Thread.holdsLock(obj): true when the calling thread owns obj's monitor. */
bool apk64_jni_monitor_held_by_current_host(void *object_ref);
/* Object.wait(timeout_ms, nanos); timeout 0 waits for a notification. */
int32_t apk64_jni_monitor_wait_host(void *env_handle, void *object_ref, int64_t timeout_ms, int32_t nanos);
/* Object.clone(): a new local reference, or NULL with CloneNotSupportedException
 * pending (non-Cloneable instance or a framework model object). */
void *apk64_jni_clone_object_host(void *env_handle, void *object_ref);
/* For untyped 64-bit values (DEX registers): returns true when both values are
 * JNI references issued by this runtime, and then reports in out_same whether
 * they name the same object. Never dereferences a value that is not a ref. */
bool apk64_jni_ref_values_identity(uint64_t a, uint64_t b, bool *out_same);

/* A new DEX lookup tier became visible: provisional classes the package
 * resolver could not define are resolved again on their next lookup. */
void apk64_jni_class_resolver_invalidate(void) __attribute__((weak));
/* Return the opaque object token backing a JNI reference. This is intended for
 * framework-owned state tables whose callbacks are keyed by object token. */
void *apk64_jni_object_token_host(void *object_ref);
/* Copy the concrete runtime class name for a JNI reference. */
bool apk64_jni_object_class_name_host(void *object_ref, char *out, size_t out_size);
/* Copy the represented class name from a java/lang/Class reference. */
bool apk64_jni_class_ref_name_host(void *class_ref, char *out, size_t out_size);
/* Copy the message carried by a modeled Throwable/Exception object. */
bool apk64_jni_exception_message_host(void *throwable_ref, char *out, size_t out_size);
/* Host-side exception helpers used by the DEX interpreter. Pending JNI
 * exceptions must remain observable across nested interpreted calls so DEX
 * try/catch handlers can route them exactly like ART/Dalvik. */
bool apk64_jni_exception_pending_host(void *env_handle);
void *apk64_jni_exception_occurred_host(void *env_handle);
void apk64_jni_exception_clear_host(void *env_handle);
Apk64Result apk64_jni_throw_host(void *env_handle, void *throwable_ref);
bool apk64_jni_is_instance_of_class_host(void *object_ref, const char *class_name);
Apk64Result apk64_jni_set_instance_field_value(void *object_ref, const char *name,
                                                const char *signature, Apk64JniValue value);
/* Owner-qualified instance-field access used by the DEX interpreter. DEX field_id
 * entries name the qualifying type of the reference, so resolution starts at
 * that class and walks its superclasses (JVMS 5.4.3.2); a field declared on a
 * subclass of the qualifying type is never matched, preserving hiding.
 * Object/array reads are returned as ordinary local JNI references. */
Apk64Result apk64_jni_get_instance_field_value(void *env_handle, void *object_ref,
                                                const char *class_name, const char *name,
                                                const char *signature, Apk64JniValue *out_value,
                                                bool *out_found);
/* Process-exit request (Android would kill the app process, e.g. a thread
 * died with an uncaught failure). The first reason wins; blocking framework
 * waits and the DEX interpreter observe it and unwind. Cleared by init. */
void apk64_jni_request_process_exit(const char *reason);
bool apk64_jni_process_exit_requested(char *out_reason, size_t out_reason_cap);
void apk64_jni_clear_process_exit(void);

/* True when class_name (lazily resolved) or a superclass declares the field.
 * Used by the diagnostic DEX preflight; it never materializes fields. */
bool apk64_jni_field_exists_host(const char *class_name, const char *name,
                                 const char *signature, bool is_static);
Apk64Result apk64_jni_set_instance_field_value_for_class(void *object_ref,
                                                          const char *class_name,
                                                          const char *name,
                                                          const char *signature,
                                                          Apk64JniValue value,
                                                          bool *out_found);
Apk64Result apk64_jni_set_static_field_value(const char *class_name, const char *name,
                                              const char *signature, Apk64JniValue value);
/* Host-side static-field read used by the bounded DEX interpreter. Object and
 * array values are returned as ordinary local JNI references so DEX registers
 * keep the same reference representation as other Java/JNI paths. A missing
 * class/field is a normal out_found=false result. */
Apk64Result apk64_jni_get_static_field_value(void *env_handle, const char *class_name,
                                              const char *name, const char *signature,
                                              Apk64JniValue *out_value, bool *out_found);
/* A resolved field handle is valid only for the JNI metadata generation in
 * which it was obtained. DEX caches must re-resolve on generation changes.
 * A handle is an opaque field ID, not a JNI object/reference. */
typedef struct Apk64JniResolvedField {
    void *field;
    /* Opaque class whose name appeared in the DEX field reference, not
     * necessarily the class that declares the inherited field. */
    void *requested_class;
    uint64_t generation;
    char type_code;
    bool is_static;
} Apk64JniResolvedField;
uint64_t apk64_jni_field_resolution_generation(void);
/* DEEP profiler: report aggregate JNI cache and reference diagnostics. */
void apk64_jni_cache_diag_report(void);
bool apk64_jni_resolve_field_host(const char *class_name, const char *name,
                                  const char *signature, bool is_static,
                                  Apk64JniResolvedField *out_field);
/* Returns false for stale/mismatched handles so the interpreter falls back to
 * normal resolution. If true, out_result describes a completed access. */
bool apk64_jni_access_resolved_field(void *env_handle, void *object_ref,
                                    const Apk64JniResolvedField *resolved,
                                    bool is_write, Apk64JniValue *inout_value,
                                    Apk64Result *out_result);
int32_t apk64_jni_throw_new_by_name(void *env_handle, const char *class_name,
                                    const char *message);

/* Stage 8 host-side array helpers used by Java AssetManager/InputStream models. */
void *apk64_jni_new_local_object_array(void *env_handle, int32_t length);
/* Create a reference array with the exact DEX/JNI array descriptor, e.g.
 * [Ljava/lang/String; or [[I.  This is used by the DEX VM so new-array and
 * filled-new-array preserve the runtime array class instead of degrading to
 * Object[]. */
void *apk64_jni_new_local_reference_array(void *env_handle,
                                          const char *array_descriptor,
                                          int32_t length);
/* Allocate a new array with the same runtime array class/kind as source, but
 * with a caller-selected length. Used by java.util.Arrays.copyOf/copyOfRange so
 * reference-array covariance and primitive kinds are preserved. */
void *apk64_jni_new_local_array_like_host(void *env_handle,
                                           void *source_array_ref,
                                           int32_t length);
Apk64Result apk64_jni_set_object_array_element_host(void *env_handle, void *array_ref,
                                                     int32_t index, void *value_ref);
/* Read one element from a modeled Object[] array. A null array element is a
 * successful read and is returned as *out_value_ref == NULL. */
Apk64Result apk64_jni_get_object_array_element_host(void *env_handle, void *array_ref,
                                                     int32_t index, void **out_value_ref);
void *apk64_jni_new_local_boolean_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_byte_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_char_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_short_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_int_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_float_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_long_array(void *env_handle, int32_t length);
void *apk64_jni_new_local_double_array(void *env_handle, int32_t length);
/* Single-element primitive array access for DEX aget/aput. type is the Java
 * element descriptor (Z B C S I J F D); values use the canonical DEX register
 * encoding (narrow integers sign/zero-extended per Java type, float as its
 * 32-bit pattern, long/double as 64 bits). Callers check null/bounds first;
 * a kind mismatch or bad index returns APK64_ERR_ARGUMENT. */
Apk64Result apk64_jni_primitive_array_get_host(void *array_ref, char type, int32_t index,
                                               uint64_t *out_bits);
Apk64Result apk64_jni_primitive_array_set_host(void *array_ref, char type, int32_t index,
                                               uint64_t bits);
Apk64Result apk64_jni_boolean_array_write(void *array_ref, int32_t start, int32_t count,
                                          const uint8_t *in);
Apk64Result apk64_jni_char_array_write(void *array_ref, int32_t start, int32_t count,
                                       const uint16_t *in);
Apk64Result apk64_jni_short_array_write(void *array_ref, int32_t start, int32_t count,
                                        const int16_t *in);
Apk64Result apk64_jni_int_array_write(void *array_ref, int32_t start, int32_t count,
                                      const int32_t *in);
Apk64Result apk64_jni_float_array_write(void *array_ref, int32_t start, int32_t count,
                                        const float *in);
/* Return the length of any array type currently modeled by APK64 (object, byte,
 * or int). Returns -1 when the reference is null or is not an array. This is
 * shared by JNI GetArrayLength and the DEX array-length opcode so both paths
 * observe one object model. */
int32_t apk64_jni_array_length_host(void *array_ref);
/* Stable storage of a primitive array's elements (APK64 arrays never move),
 * or NULL for a non-primitive-array reference. */
void *apk64_jni_primitive_array_data_host(void *array_ref);
Apk64Result apk64_jni_arraycopy_host(void *src_ref, int32_t src_pos, void *dst_ref, int32_t dst_pos, int32_t length);
int32_t apk64_jni_byte_array_length(void *array_ref);
Apk64Result apk64_jni_byte_array_read(void *array_ref, int32_t start, int32_t count, int8_t *out);
Apk64Result apk64_jni_char_array_read(void *array_ref, int32_t start, int32_t count, uint16_t *out);
Apk64Result apk64_jni_byte_array_write(void *array_ref, int32_t start, int32_t count, const int8_t *in);


/* air_adapter interface */
#ifndef APK64_AIR_ADAPTER_H
#define APK64_AIR_ADAPTER_H


/* Platform capabilities for a resolved, initialized package AIR method.
 * Unknown methods or unavailable host capabilities execute their original DEX. */
Apk64Result apk64_air_try_call(void *env, const char *class_name,
                              const char *method_name, const char *signature,
                              bool is_static, size_t arg_count,
                              Apk64JniValue *out_result, bool *out_handled);

#endif


/* apk interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define APK64_APK_PATH_CAP 768u
#define APK64_APK_PACKAGE_CAP 256u
#define APK64_APK_ACTIVITY_CAP 256u
#define APK64_APK_LIBNAME_CAP 128u
#define APK64_APK_SPLIT_CAP 128u
#define APK64_APK_ENTRY_CAP 512u
/* Java bootstrap libraries: up to one per packaged arm64 library, stored as
 * packed NUL-separated System.loadLibrary names. */
#define APK64_APK_JAVA_BOOTSTRAP_MAX 64u
#define APK64_APK_JAVA_BOOTSTRAP_NAMES_CAP 1024u
#define APK64_APK_PROVIDER_MAX 32u

typedef enum Apk64EngineKind {
    APK64_ENGINE_UNKNOWN = 0,
    APK64_ENGINE_UNITY = 1,
} Apk64EngineKind;

typedef enum Apk64UnityBackend {
    APK64_UNITY_BACKEND_NONE = 0,
    APK64_UNITY_BACKEND_IL2CPP = 1,
    APK64_UNITY_BACKEND_MONO = 2,
    APK64_UNITY_BACKEND_MIXED = 3,
} Apk64UnityBackend;

typedef struct Apk64ManifestProvider {
    char class_name[APK64_APK_ACTIVITY_CAP];
    char process_name[APK64_APK_PACKAGE_CAP];
    char authorities[256];
    int32_t init_order;
    bool enabled;
    bool exported;
    bool exported_declared;
} Apk64ManifestProvider;

typedef struct Apk64ApkInfo {
    char apk_path[APK64_APK_PATH_CAP];
    char package_name[APK64_APK_PACKAGE_CAP];
    char version_name[128];
    char application_label[128];
    /* Manifest <application android:name>, raw declaration form. Empty means
     * android.app.Application. Component expansion is performed by the launcher. */
    char application_name[APK64_APK_ACTIVITY_CAP];
    uint32_t version_code;
    uint32_t min_sdk;
    uint32_t target_sdk;

    /* Resource IDs declared by <application android:icon/roundIcon>.  These
     * remain Android resource identifiers (0xpptteeee); launcher decoding
     * resolves them through resources.arsc instead of guessing filenames. */
    uint32_t application_icon_resource;
    uint32_t application_round_icon_resource;
    /* <application android:label> when it is a @string reference rather than a
     * literal. Launcher metadata resolves it through resources.arsc. */
    uint32_t application_label_resource;
    uint32_t application_theme_resource;
    uint32_t activity_theme_resource; /* launcher target declaration; 0 inherits application */

    int32_t screen_orientation;
    bool screen_orientation_declared;

    char activity_name[APK64_APK_ACTIVITY_CAP];
    char native_lib_name[APK64_APK_LIBNAME_CAP];
    char native_entry_path[APK64_APK_ENTRY_CAP];
    char split_name[APK64_APK_SPLIT_CAP];

    /* Manifest ContentProviders are process components. Android installs them
     * after the Application object is attached but before Application.onCreate()
     * and before any Activity startup. Keep the small declaration subset needed
     * to reproduce that process-main-thread ordering generically. */
    size_t provider_count;
    Apk64ManifestProvider providers[APK64_APK_PROVIDER_MAX];

    size_t zip_entry_count;
    size_t native_lib_count;
    size_t arm64_lib_count;
    bool native_activity;
    bool native_library_present;
    bool manifest_binary_xml;
    bool split_apk;

    /* Reusable engine metadata discovered from the selected ARM64 payload.
     * Detection is based only on canonical middleware library names; it never
     * changes application behavior or selects title-specific compatibility. */
    Apk64EngineKind engine_kind;
    Apk64UnityBackend unity_backend;
    bool unity_player_present;
    bool unity_main_present;
    bool unity_il2cpp_present;
    bool unity_mono_present;
    bool sdl_present;
    bool godot_present;
    bool gamemaker_present;
    bool game_activity;

    /* Ordinary Java Activity packages prepare native-library candidates from
     * DEX System/Runtime loadLibrary strings and application android.app.lib_name
     * metadata. Java startup still controls loading and JNI_OnLoad. These names
     * use System.loadLibrary form (no lib/.so wrapper). */
    bool java_activity_bootstrap;
    size_t java_dex_count;
    size_t java_bootstrap_lib_count;
    char java_bootstrap_libs[APK64_APK_JAVA_BOOTSTRAP_NAMES_CAP]; /* use apk64_apk_java_bootstrap_lib() */

    /* Logical-package metadata filled by the package layer. For a standalone
     * APK base_apk_path == apk_path. For XAPK, apk_path stays the original
     * container while base_apk_path names the materialized base APK. */
    bool source_xapk;
    size_t package_apk_count;
    size_t package_selected_apk_count;
    size_t package_obb_count;
    char base_apk_path[APK64_APK_PATH_CAP];
} Apk64ApkInfo;

/* Returns the index-th Java bootstrap library name, or NULL when out of range. */
static inline const char *apk64_apk_java_bootstrap_lib(const Apk64ApkInfo *info, size_t index) {
    if (!info || index >= info->java_bootstrap_lib_count) return NULL;
    const char *p = info->java_bootstrap_libs;
    const char *const end = p + sizeof(info->java_bootstrap_libs);
    for (; index > 0 && p < end; --index) {
        while (p < end && *p) ++p;
        ++p;
    }
    return p < end && *p ? p : NULL;
}


/* Inspect APK metadata without imposing the runtime's current launchability
 * policy. This succeeds for valid Java-Activity APKs and split APKs. */
Apk64Result apk64_apk_inspect(const char *apk_path, Apk64ApkInfo *out_info);

/* Active deduplicated manifest permission names in declaration order. Caller
 * frees each string and the array. Filters maxSdkVersion / sdk-23 tags. */
Apk64Result apk64_apk_requested_permissions(const char *apk_path, uint32_t platform_api,
                                            char ***out_names, size_t *out_count);

typedef struct Apk64ZipEntryInfo {
    char name[APK64_APK_ENTRY_CAP];
    uint16_t flags;
    uint16_t method;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint32_t dos_time;
} Apk64ZipEntryInfo;

typedef bool (*Apk64ZipEntryVisitor)(const Apk64ZipEntryInfo *entry, void *userdata);

/* Retained archive for Java ZipFile: reads remain tied to the opened file even
 * after its pathname is renamed/replaced. Caller serializes handle operations. */
typedef struct ZipDirectory Apk64ZipArchive;
Apk64Result apk64_zip_archive_open(const char *path, Apk64ZipArchive **out);
void apk64_zip_archive_close(Apk64ZipArchive *zip);
Apk64Result apk64_zip_archive_entry(Apk64ZipArchive *zip, const char *name, Apk64ZipEntryInfo *out);
Apk64Result apk64_zip_archive_count(Apk64ZipArchive *zip, size_t *out);
/* Independent byte cursor into the retained central directory; starts at zero.
 * Count bounds iteration. Lookup/read operations do not disturb this cursor. */
Apk64Result apk64_zip_archive_next(Apk64ZipArchive *zip, size_t *cursor, Apk64ZipEntryInfo *out);
Apk64Result apk64_zip_archive_read(Apk64ZipArchive *zip, const char *name, uint8_t **out, size_t *size);

/* Generic ZIP traversal/extraction used by APK and XAPK package handling.
 * Returning false from a visitor stops iteration successfully. */
Apk64Result apk64_zip_visit(const char *zip_path, Apk64ZipEntryVisitor visitor, void *userdata);
Apk64Result apk64_zip_has_entry(const char *zip_path, const char *entry_name, bool *out_present);
Apk64Result apk64_zip_extract_entry(const char *zip_path,
                                    const char *entry_name,
                                    const char *output_path);

/* Read one exact ZIP entry into heap memory. Caller owns *out_data and frees it
 * with free(). This is used by package metadata analyzers such as the Stage
 * 14.2 DEX bootstrap scanner without materializing large temporary files. */
Apk64Result apk64_zip_read_entry_alloc(const char *zip_path,
                                       const char *entry_name,
                                       uint8_t **out_data,
                                       size_t *out_size);

typedef struct Apk64ApkSigningCertificate {
    uint8_t *data;
    size_t size;
} Apk64ApkSigningCertificate;

#define APK64_APK_SIGNER_MAX 8u

typedef struct Apk64ApkSigningInfo {
    /* Active content signers extracted from the APK Signing Block. Certificate
     * bytes are the exact DER blobs embedded in the signer record. */
    Apk64ApkSigningCertificate signers[APK64_APK_SIGNER_MAX];
    size_t signer_count;
    /* Android signing scheme version understood by the API-30 compatibility
     * surface: 2 or 3. Newer signing-block IDs are ignored like Android 11. */
    int32_t scheme_version;
} Apk64ApkSigningInfo;

/* Read active APK signing certificates from v2/v3 signing blocks. This extracts
 * package identity metadata; it is not a substitute for APK install-time
 * cryptographic verification. The caller must release successful output with
 * apk64_apk_signing_info_free(). */
Apk64Result apk64_apk_read_signing_info(const char *apk_path,
                                        Apk64ApkSigningInfo *out_info);
void apk64_apk_signing_info_free(Apk64ApkSigningInfo *info);

/* Parse an Android APK without executing it. Stage 12.1 currently supports
 * classic (non-Zip64) ZIP central directories, STORE/DEFLATE entries, and
 * Android binary XML manifests. */
Apk64Result apk64_apk_probe(const char *apk_path, Apk64ApkInfo *out_info);

/* Extract every arm64-v8a shared-library entry into destination_dir. Only the base
 * filename is used at the destination, preventing APK path traversal. */
Apk64Result apk64_apk_extract_arm64(const char *apk_path,
                                    const char *destination_dir,
                                    size_t *out_count);

/* Extract one exact APK ZIP entry to a file path. */
Apk64Result apk64_apk_extract_entry(const char *apk_path,
                                    const char *entry_name,
                                    const char *output_path);

/* Load the best PNG launcher-icon candidate embedded in an APK. The returned
 * pixels use R in the low byte followed by G, B and A, matching libnx RGBA8
 * framebuffer packing. The caller owns *out_pixels and must free() it.
 * The highest-density variant is preferred, adaptive icons are cropped to their
 * visible 72/108 viewport, and results are area-downsampled to at most 256x256
 * to keep launcher memory bounded. */

/* Decode a PNG buffer into launcher RGBA pixels (R in low byte). Shared by
 * APK resource icons and XAPK outer icon.png fast-path metadata. */
bool apk64_png_decode_rgba(const uint8_t *png, size_t png_size,
                           uint32_t **out_pixels, int *out_width, int *out_height);

Apk64Result apk64_apk_load_launcher_icon(const char *apk_path,
                                         uint32_t **out_pixels,
                                         int *out_width,
                                         int *out_height,
                                         char *out_entry_name,
                                         size_t out_entry_name_cap);

/* Resolve a @string resource (typically android:label) through the APK's
 * resources.arsc, preferring the default locale, then English. */
Apk64Result apk64_apk_resolve_string_resource(const char *apk_path, uint32_t resource_id,
                                              char *out, size_t out_cap);

/* Resolve a file-backed resource (res/raw, res/xml, drawables ...) to its APK
 * entry name ("res/raw/name.ext") for the device configuration. */
Apk64Result apk64_apk_resolve_file_resource(const char *apk_path, uint32_t resource_id,
                                            char *out, size_t out_cap);

/* Resolve a scalar @bool resource using the same resources.arsc configuration
 * selection used by the framework. References are followed recursively. */
Apk64Result apk64_apk_resolve_boolean_resource(const char *apk_path, uint32_t resource_id,
                                               bool *out_value);

/* Resolve a scalar @color resource to the Android Color int representation
 * (0xAARRGGBB), following resource references and device configuration. */
Apk64Result apk64_apk_resolve_color_resource(const char *apk_path, uint32_t resource_id,
                                             uint32_t *out_argb);

/* Resolve Resources.getIdentifier-style names through resources.arsc.  The
 * returned value is the APK's real 0xPPTTEEEE resource identifier; a missing
 * resource is reported as APK64_ERR_SYMBOL_NOT_FOUND so Java can return 0. */
Apk64Result apk64_apk_find_resource_id(const char *apk_path, const char *name,
                                       const char *def_type, const char *def_package,
                                       uint32_t *out_resource_id);

typedef enum Apk64MetaDataType {
    APK64_META_DATA_STRING = 1,
    APK64_META_DATA_INT,
    APK64_META_DATA_BOOLEAN,
    APK64_META_DATA_FLOAT,
} Apk64MetaDataType;

/* One real style attribute. text is owned; free after copying to Java. */
typedef struct Apk64StyleValue {
    uint8_t type;
    uint32_t data, resource_id;
    char *text;
    uint32_t asset_cookie, density, changing_configurations;
} Apk64StyleValue;

typedef struct Apk64XmlAttribute {
    char *name, *namespace_uri, *text;
    uint32_t name_resource, data;
    uint8_t type;
} Apk64XmlAttribute;
typedef struct Apk64XmlNode {
    char *name;
    int32_t parent;
    uint32_t line;
    Apk64XmlAttribute *attributes;
    size_t attribute_count;
} Apk64XmlNode;
typedef struct Apk64XmlDocument {
    Apk64XmlNode *nodes;
    size_t count;
} Apk64XmlDocument;
Apk64Result apk64_apk_parse_binary_xml(const uint8_t *data, size_t size, Apk64XmlDocument *out);
void apk64_apk_free_xml(Apk64XmlDocument *document);
Apk64Result apk64_apk_resolve_value(const char *path, uint32_t id, Apk64StyleValue *out);
Apk64Result apk64_apk_get_resource_value(const char *path, uint32_t id, bool resolve_refs,
                                        Apk64StyleValue *out);
Apk64Result apk64_apk_resolve_style_attribute(const char *apk_path, uint32_t style_id,
                                               uint32_t attribute_id, bool resolve_refs,
                                               Apk64StyleValue *out);

/* One <application><meta-data> entry, typed as Android's PackageParser stores
 * it in ApplicationInfo.metaData: android:resource -> int resource id;
 * android:value -> string/boolean/int/float (references resolved through
 * resources.arsc as TypedArray.peekValue does). */
typedef struct Apk64MetaData {
    char name[128];
    Apk64MetaDataType type;
    int32_t int_value;
    float float_value;
    char string_value[256];
} Apk64MetaData;

#define APK64_APK_META_DATA_MAX 64u

/* Read the application-level <meta-data> entries in manifest order. Entries
 * beyond cap and unsupported value types are skipped. */
Apk64Result apk64_apk_read_application_meta_data(const char *apk_path, Apk64MetaData *out,
                                                 size_t cap, size_t *out_count);

/* Read <provider><meta-data> for one manifest provider. provider_class_name may
 * use Android dot notation or APK64 slash notation; relative manifest names are
 * expanded against package_name before comparison. */
Apk64Result apk64_apk_read_provider_meta_data(const char *apk_path, const char *package_name,
                                              const char *provider_class_name, Apk64MetaData *out,
                                              size_t cap, size_t *out_count);
/* Same for any manifest component element (component_tag "service",
 * "activity", "activity-alias", "receiver" or "provider").
 * APK64_ERR_SYMBOL_NOT_FOUND when the manifest declares no such component. */
Apk64Result apk64_apk_read_component_meta_data(const char *apk_path, const char *package_name,
                                               const char *component_tag, const char *class_name,
                                               Apk64MetaData *out, size_t cap, size_t *out_count);

/* One <application> child component as PackageParser records it. class_name
 * and target_activity are as declared (possibly relative, ".Name"). */
typedef struct Apk64ManifestComponent {
    char class_name[APK64_APK_ACTIVITY_CAP];
    char target_activity[APK64_APK_ACTIVITY_CAP]; /* activity-alias only */
    bool enabled;                                 /* android:enabled, default true */
    bool exported;
    bool exported_declared;
} Apk64ManifestComponent;

/* The <application> children with tag component_tag ("activity",
 * "activity-alias", "service", "receiver" or "provider") in manifest order;
 * entries beyond cap are skipped. */
Apk64Result apk64_apk_list_components(const char *apk_path, const char *component_tag,
                                      Apk64ManifestComponent *out, size_t cap, size_t *out_count);
/* Declared Activity theme, including activity-alias target inheritance. Zero
 * means ApplicationInfo.theme is used by ActivityInfo.getThemeResource(). */
Apk64Result apk64_apk_activity_theme(const char *apk_path, const char *package_name,
                                    const char *activity_name, uint32_t *out_theme);


/* android_compat interface */


#include <stdbool.h>
#include <stdint.h>

/* APK64 currently exposes an Android 11 / API 30 runtime personality.  Keep
 * these defaults in one place so Java framework state, Bionic properties and
 * native API-level queries cannot silently drift apart. */
#define APK64_ANDROID_PLATFORM_API_DEFAULT 30
#define APK64_ANDROID_MIN_SDK_DEFAULT      21
#define APK64_ANDROID_TARGET_SDK_DEFAULT   30
#define APK64_ANDROID_API_FUTURE            10000

typedef enum Apk64AndroidAbi {
    APK64_ANDROID_ABI_UNKNOWN = 0,
    APK64_ANDROID_ABI_ARM64_V8A = 1,
} Apk64AndroidAbi;

typedef struct Apk64AndroidCompatProfile {
    /* Android runtime API level observed as Build.VERSION.SDK_INT and by
     * android_get_device_api_level()/ro.build.version.sdk. */
    int32_t platform_api;

    /* Manifest application requirements/compatibility target. */
    int32_t min_sdk;
    int32_t target_sdk;

    /* Guest ABI selected for this process. APK64 Phase 2 supports arm64-v8a. */
    Apk64AndroidAbi abi;
} Apk64AndroidCompatProfile;

/* Construct a profile while preserving APK64's Phase-0/1 defaults for fields
 * that were absent from the manifest/configuration. No install-policy decision
 * is made here: a minSdk above platform_api remains observable and can be
 * handled by a later compatibility-policy phase. */
Apk64AndroidCompatProfile apk64_android_compat_profile_make(int32_t platform_api,
                                                            int32_t min_sdk,
                                                            int32_t target_sdk,
                                                            Apk64AndroidAbi abi);

/* Process-wide Android personality. It is configured before guest execution
 * and may only be changed between launches after guest threads are quiescent. */
void apk64_android_compat_set_profile(const Apk64AndroidCompatProfile *profile);
void apk64_android_compat_reset_profile(void);
void apk64_android_compat_get_profile(Apk64AndroidCompatProfile *out_profile);
bool apk64_android_compat_profile_is_configured(void);

int32_t apk64_android_compat_platform_api(void);
int32_t apk64_android_compat_min_sdk(void);
int32_t apk64_android_compat_target_sdk(void);
Apk64AndroidAbi apk64_android_compat_abi(void);
const char *apk64_android_compat_abi_name(Apk64AndroidAbi abi);

bool apk64_android_compat_platform_at_least(int32_t api_level);
bool apk64_android_compat_target_at_least(int32_t api_level);
bool apk64_android_compat_target_before(int32_t api_level);
bool apk64_android_compat_min_sdk_supported(void);

/* Named Android compatibility gates belong here rather than as scattered
 * targetSdk comparisons in framework/Bionic implementations. Android changed
 * ContentProvider's default exported state for apps targeting API 17+; explicit
 * android:exported always wins at the manifest layer. */
bool apk64_android_compat_provider_default_exported(void);


/* android interface */


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>




typedef Apk64Result (*Apk64AndroidLoadLibraryCallback)(const char *library_name,
                                                        void *user_data);

/* Apply Activity.setRequestedOrientation() to the host presentation layer.
 * On success the callback returns the resulting Android logical dimensions and
 * Display.getRotation() value so framework Resources/DisplayMetrics stay in
 * lockstep with the real surface presented to the guest. */
typedef bool (*Apk64AndroidOrientationRequestCallback)(int32_t requested_orientation,
                                                       int32_t *out_width_pixels,
                                                       int32_t *out_height_pixels,
                                                       int32_t *out_display_rotation,
                                                       void *user_data);

typedef struct Apk64AndroidConfig {
    const char *package_name;
    const char *version_name;
    int64_t version_code;
    /* Inputs to the process-wide Android compatibility profile. Zero selects
     * APK64's centralized defaults from android_compat.h. */
    int32_t min_sdk;
    int32_t target_sdk;
    int32_t sdk_int; /* Android runtime/platform API (Build.VERSION.SDK_INT). */
    const char *release;
    const char *codename;

    const char *manufacturer;
    const char *brand;
    const char *model;
    const char *device;
    const char *product;
    const char *hardware;

    const char *language;
    const char *country;

    /* Current Android resource/input configuration. Zero leaves the framework
     * default in place; production launch code supplies the real APK64 runtime
     * state. These values use android/configuration.h numeric constants. */
    int32_t touchscreen;
    int32_t keyboard;
    int32_t navigation;
    int32_t keys_hidden;
    int32_t nav_hidden;
    int32_t ui_mode_type;
    int32_t ui_mode_night;
    int32_t layout_direction;
    int32_t screen_round;

    int32_t width_pixels;
    int32_t height_pixels;
    int32_t density_dpi;
    float density;
    float scaled_density;
    float xdpi;
    float ydpi;
    /* Display refresh rate reported by android.view.Display.getRefreshRate().
     * Values <= 0 use APK64's platform default (60 Hz on Switch). */
    float refresh_rate_hz;

    /* android.view.Surface.ROTATION_* value for the guest logical display. */
    int32_t display_rotation;

    const char *root_path;
    const char *native_library_dir;
    /* Host path used internally by APK64 for package/DEX access. */
    const char *source_path;
    /* Android-visible absolute APK path returned by Context/ApplicationInfo.
     * When NULL, source_path is exposed for legacy/test configurations. */
    const char *package_code_path;
    const char *application_label;

    /* Optional manifest Application class. Dot or slash notation is accepted.
     * NULL/empty selects android.app.Application. */
    const char *application_class_name;

    /* Optional manifest Java Activity class. Dot or slash notation is accepted.
     * NativeActivity launches leave this NULL and retain the historical base
     * android/app/Activity instance. */
    const char *activity_class_name;
    uint32_t application_theme_resource;
    uint32_t activity_theme_resource;

    /* Manifest ActivityInfo.screenOrientation. This seeds Activity's observable
     * requested-orientation state before any runtime setRequestedOrientation(). */
    int32_t activity_screen_orientation;
    bool activity_screen_orientation_declared;

    /* Optional bridge used by Activity.setRequestedOrientation(). */
    Apk64AndroidOrientationRequestCallback request_orientation;
    void *request_orientation_user_data;

    /* Stage 8 asset sources. Either or both may be supplied. Directory assets
     * take precedence over APK/ZIP assets on duplicate names. */
    const char *asset_directory;
    const char *asset_apk_path;

    /* Application-level manifest <meta-data>, exposed as ApplicationInfo.metaData
     * when PackageManager is asked for GET_META_DATA. Copied during init. */
    const Apk64MetaData *application_meta_data;
    size_t application_meta_data_count;

    /* Manifest ContentProvider declarations. APK64 copies this array during
     * framework init so PackageManager.getProviderInfo can expose real component
     * identity/initOrder and provider-scoped meta-data. */
    const Apk64ManifestProvider *manifest_providers;
    size_t manifest_provider_count;

    /* Java System.loadLibrary(String) bridge. The callback receives the Java
     * logical library name (for example "cocos2dcpp", not "libcocos2dcpp.so").
     * It is responsible for Android-style mapping/loading and JNI_OnLoad. */
    Apk64AndroidLoadLibraryCallback load_library;
    void *load_library_user_data;
} Apk64AndroidConfig;

/* Register and instantiate APK64's generic minimal Android framework model.
 * apk64_jni_init() must already have succeeded on the calling thread. */
Apk64Result apk64_android_init(const Apk64AndroidConfig *config);
void apk64_android_reset(void);

/* Publish an initialized, current-process manifest ContentProvider. Framework
 * query dispatch retains its own global reference until android_reset(). */
Apk64Result apk64_android_publish_content_provider(void *env, void *provider_ref);

union Apk64JniValue;
/* invoke-virtual from host code: the receiver's runtime class (including a
 * package DEX override) selects the implementation. False when the call did
 * not complete or left an exception pending. */
bool apk64_android_call_virtual(void *env, void *receiver, const char *name, const char *sig,
                                const union Apk64JniValue *args, size_t n, union Apk64JniValue *out);


typedef enum Apk64AndroidActivityState {
    APK64_ANDROID_ACTIVITY_INITIALIZED = 0,
    APK64_ANDROID_ACTIVITY_CREATED,
    APK64_ANDROID_ACTIVITY_STARTED,
    APK64_ANDROID_ACTIVITY_RESUMED,
    APK64_ANDROID_ACTIVITY_PAUSED,
    APK64_ANDROID_ACTIVITY_STOPPED,
    APK64_ANDROID_ACTIVITY_DESTROYED,
} Apk64AndroidActivityState;

/* Observable state of APK64's single foreground Activity model. The state is
 * advanced by the framework base Activity lifecycle methods, so a DEX subclass
 * that fails to call super is detectable just like Android's ActivityThread
 * SuperNotCalledException contract. */
Apk64AndroidActivityState apk64_android_activity_state(void);
const char *apk64_android_activity_state_name(Apk64AndroidActivityState state);
bool apk64_android_activity_has_window_focus(void);
void apk64_android_set_window_focus(bool has_focus);
/* WindowManager attachment, after the Activity's onResume has returned. */
void apk64_android_set_activity_window_attached(bool attached);
/* Display-frame animation callbacks followed by scheduled measure/layout. */
bool apk64_android_do_frame(void);
bool apk64_android_has_pending_activity(void);
bool apk64_android_activity_finish_requested(void);
/* Raises ActivityLifecycleCallbacks.onActivity<event> for the current
 * Activity (event = "PreCreated", "PostResumed", ...; API 29 Pre/Post
 * callbacks). Callbacks without an override are skipped. */
bool apk64_android_dispatch_activity_lifecycle_event(void *env, const char *event);

/* Stable global JNI references suitable for passing to guest code. */
void *apk64_android_activity(void);
void *apk64_android_application(void);
void *apk64_android_base_context(void);
void *apk64_android_asset_manager(void);
/* Current root retained by Activity.setContentView(View), or NULL before one is set. */
void *apk64_android_content_view(void);
/* Commit one Activity.startActivity(Intent) transition queued by the modeled
 * Java framework. The returned class is slash-form and apk64_android_activity()
 * then refers to the new Activity. Consume after startup callbacks/main-Looper
 * work return, matching Android's asynchronous Activity transition. */
bool apk64_android_take_started_activity(void *env_handle,
                                         char *out_class_name,
                                         size_t out_class_name_size);
/* Activity.isFinishing()/isDestroyed() state driven by the launcher's final
 * shutdown (finishing before onPause, destroyed after onDestroy). */
void apk64_android_mark_activity_finishing(void);
void apk64_android_mark_activity_destroyed(void);
/* Workers started through java.lang.Thread.start() that have not returned
 * from run(). These are host threads executing APK64 text. */
size_t apk64_android_live_java_thread_count(void);
/* Release host handles/stacks of exited Java workers without changing their
 * observable Java Thread state. Also used by native thread quota recovery. */
size_t apk64_android_reap_exited_java_threads(void);
/* Minimum backing stack used for java.lang.Thread workers. Android ART adds
 * a 1 MiB native-stack allowance so managed threads may safely enter JNI. */
size_t apk64_android_java_thread_stack_size(void);
/* Opt-in Google Play services presence (sdmc:/switch/papaspaleteria_nx/gms-present.flag
 * on Switch): report com.google.android.gms as installed but provide none of
 * its services. Exposed for host tests and embedders. */
void apk64_android_set_gms_present(bool present);
/* Wait up to timeout_ms for Thread.start() workers to return, join every
 * finished one, and return how many are still running. */
size_t apk64_android_wait_java_threads_idle(uint32_t timeout_ms);
/* Stable per-package anonymous Android identity backing Settings.Secure.ANDROID_ID.
 * Never derived from Nintendo account or console identifiers. */
const char *apk64_android_stable_id(void);

typedef struct Apk64AndroidGLSurfaceViewInfo {
    /* Local JNI references owned by the caller; delete them with
     * apk64_jni_delete_local_ref_host(). */
    void *view_ref;
    void *renderer_ref;
    int32_t egl_client_version;
    bool preserve_egl_context_on_pause;
    bool egl_config_explicit;
    int32_t egl_red_size;
    int32_t egl_green_size;
    int32_t egl_blue_size;
    int32_t egl_alpha_size;
    int32_t egl_depth_size;
    int32_t egl_stencil_size;
    bool resumed;
    int32_t render_mode;
    size_t queued_event_count;
} Apk64AndroidGLSurfaceViewInfo;

/* Find the first renderer-bearing GLSurfaceView reachable from the Activity
 * content root. This preserves Android's content-tree ownership instead of
 * scanning arbitrary Java objects. */
/* True once a SurfaceHolder.Callback has received surfaceCreated() for a
 * SurfaceView, i.e. the app renders into that surface itself (for example from
 * its own EGL/native thread) instead of through a GLSurfaceView renderer. */
bool apk64_android_has_attached_surface(void);
/* Read-only, bounded inventory; never invokes guest View callbacks. */
void apk64_android_log_surface_state(void *env_handle, const char *reason);

/* Diagnostics: log main-queue depth and, per worker Looper, the owning Java
 * thread, queue depth and the callback currently being dispatched. */
void apk64_android_log_looper_states(const char *reason);
void apk64_android_log_java_threads(const char *reason);

/* Publish the latest host input state to Android's InputManager/InputDevice
 * model, NDK motion queues and Java SensorManager listeners. Queue
 * InputDeviceListener callbacks for real connection changes. */
struct Apk64InputSnapshot;
void apk64_android_update_input_snapshot(const struct Apk64InputSnapshot *snapshot);
/* Android-facing controller semantics above the host-neutral Apk64InputSnapshot.
 * The host backend never emits Android keycodes/events directly. */
int32_t apk64_android_keycode_for_button(uint64_t button_bit);
void apk64_android_dispatch_controller_input(void *activity_ref,
                                              const struct Apk64InputSnapshot *snapshot);

/* One touchscreen contact in window pixels; ids stay stable while the finger
 * is down. touch_major/minor are the contact diameters (0 = unknown). */
typedef struct Apk64AndroidTouchPoint {
    int32_t id;
    float x;
    float y;
    float touch_major;
    float touch_minor;
} Apk64AndroidTouchPoint;

/* Feed one frame of touchscreen state. The difference to the previous frame
 * becomes Android's MotionEvent stream (DOWN, POINTER_DOWN, MOVE, POINTER_UP,
 * UP), dispatched on the calling main thread through
 * activity.dispatchTouchEvent and the view tree. */
void apk64_android_dispatch_touches(void *activity_ref, const Apk64AndroidTouchPoint *points, size_t count);
/* End an active gesture with ACTION_CANCEL (focus loss, backgrounding). */
void apk64_android_cancel_touches(void *activity_ref);

Apk64Result apk64_android_find_glsurfaceview(void *env_handle,
                                             Apk64AndroidGLSurfaceViewInfo *out_info);
/* Pop one Runnable queued through GLSurfaceView.queueEvent(). The returned
 * Runnable is a local JNI reference owned by the caller. */
Apk64Result apk64_android_glsurfaceview_dequeue_event(void *env_handle,
                                                       void *view_ref,
                                                       void **out_runnable_ref,
                                                       bool *out_found);
/* Wait for main-Looper work, capped by the remaining input/vsync budget.
 * Existing due work returns immediately; inserting earlier work wakes it. */
void apk64_android_main_looper_wait(uint64_t max_wait_ns);
/* Pop one Runnable posted through Activity.runOnUiThread(). The returned
 * Runnable is a local JNI reference owned by the caller. */
Apk64Result apk64_android_activity_dequeue_ui_event(void *env_handle,
                                                     void **out_runnable_ref,
                                                     bool *out_found);
/* As above, but yield after this much main-Looper Message work. A false
 * out_found may mean the budget was exhausted; remaining events stay queued. */
Apk64Result apk64_android_activity_dequeue_ui_event_budget(void *env_handle,
                                                     void **out_runnable_ref,
                                                     bool *out_found,
                                                     uint64_t message_budget_ns);

/* Diagnostics / validation. */
/* Platform adapters can represent a local activity flow that completes with
 * RESULT_CANCELED because its external service is unavailable. No identity,
 * token or successful result is manufactured. Returned ref is caller-owned. */
void *apk64_android_unavailable_activity_intent(void *env_handle, const char *action, void *failure_data);

size_t apk64_android_class_count(void);
size_t apk64_android_preference_store_count(void);


/* api_stubs interface */
#ifndef APK64_API_STUBS_H
#define APK64_API_STUBS_H

#include <stdbool.h>
#include <stddef.h>


/* Declared Android API-30 platform surface (generated from AOSP current.txt,
 * android.* packages only).
 *
 * APK64 reports API level 30, so every class/member in that public API exists
 * on the device an app expects. Where APK64 has no model for one, linkage
 * would otherwise fail with NoClassDefFoundError/NoSuchMethodError/
 * NoSuchFieldError. These entry points instead bind an inert implementation
 * that follows the declaration: constructors chain to a modeled superclass
 * constructor of the same descriptor, methods return the type's neutral value
 * (0/false/null, empty arrays and collections, an inert instance of a
 * table-only platform class, the first constant of a platform enum), enum
 * classes get their constants, values() and valueOf(String). Each binding is
 * logged once. They are only consulted after every real model has missed. */

/* Class metadata. interfaces is '\n'-separated; super is java/lang/Object for
 * interfaces. Returns false when the class is not a declared android.* class. */
bool apk64_api_class_info(const char *class_name, char *out_super, size_t super_cap,
                          char *out_interfaces, size_t interfaces_cap, bool *out_is_interface);

/* Bind an inert model for a method declared by class_name or an ancestor
 * (API superclasses/interfaces; registry superclasses for app/host classes).
 * On success the method is registered on its declaring class, whose name is
 * copied to out_owner, and true is returned. env may be NULL only when no
 * registry superclass walk is needed. */
bool apk64_api_stub_bind_method(void *env, const char *class_name, const char *name,
                                const char *signature, bool is_static,
                                char *out_owner, size_t owner_cap);

/* Bind a declared field (constant value, enum constant or neutral value) on
 * its declaring class. Same walk and contract as apk64_api_stub_bind_method. */
bool apk64_api_stub_bind_field(void *env, const char *class_name, const char *name,
                               const char *signature, bool is_static,
                               char *out_owner, size_t owner_cap);

/* Declared type graph of any table class, including the header-only java.* /
 * javax.* rows that never back stubs: direct superclass and '\n'-separated
 * direct interfaces (superinterfaces for an interface). Used to give host
 * models the interfaces the platform declares for them. */
bool apk64_api_declared_supertypes(const char *class_name, char *out_super, size_t super_cap,
                                   char *out_interfaces, size_t interfaces_cap,
                                   bool *out_is_interface);

/* Number of members bound so far (diagnostics/tests). */
size_t apk64_api_stub_bound_count(void);

#endif


/* assets interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>


typedef struct Apk64AssetManager Apk64AssetManager;
typedef struct Apk64Asset Apk64Asset;
typedef struct Apk64AssetDir Apk64AssetDir;

/* Resource qualifiers associated with an AssetManager. Values intentionally
 * match android/configuration.h so the same state can back both Java
 * Resources.Configuration and native AConfiguration. */
typedef struct Apk64AssetConfiguration {
    int32_t mcc;
    int32_t mnc;
    char language[2];
    char country[2];
    int32_t orientation;
    int32_t touchscreen;
    int32_t density;
    int32_t keyboard;
    int32_t navigation;
    int32_t keys_hidden;
    int32_t nav_hidden;
    int32_t sdk_version;
    int32_t screen_size;
    int32_t screen_long;
    int32_t ui_mode_type;
    int32_t ui_mode_night;
    int32_t screen_width_dp;
    int32_t screen_height_dp;
    int32_t smallest_screen_width_dp;
    int32_t layout_direction;
    int32_t screen_round;
} Apk64AssetConfiguration;

enum {
    APK64_ACONFIGURATION_ORIENTATION_ANY = 0,
    APK64_ACONFIGURATION_ORIENTATION_PORT = 1,
    APK64_ACONFIGURATION_ORIENTATION_LAND = 2,
    APK64_ACONFIGURATION_ORIENTATION_SQUARE = 3,
    APK64_ACONFIGURATION_TOUCHSCREEN_ANY = 0,
    APK64_ACONFIGURATION_TOUCHSCREEN_NOTOUCH = 1,
    APK64_ACONFIGURATION_TOUCHSCREEN_STYLUS = 2,
    APK64_ACONFIGURATION_TOUCHSCREEN_FINGER = 3,
    APK64_ACONFIGURATION_KEYBOARD_ANY = 0,
    APK64_ACONFIGURATION_KEYBOARD_NOKEYS = 1,
    APK64_ACONFIGURATION_KEYBOARD_QWERTY = 2,
    APK64_ACONFIGURATION_KEYBOARD_12KEY = 3,
    APK64_ACONFIGURATION_NAVIGATION_ANY = 0,
    APK64_ACONFIGURATION_NAVIGATION_NONAV = 1,
    APK64_ACONFIGURATION_NAVIGATION_DPAD = 2,
    APK64_ACONFIGURATION_NAVIGATION_TRACKBALL = 3,
    APK64_ACONFIGURATION_NAVIGATION_WHEEL = 4,
    APK64_ACONFIGURATION_KEYSHIDDEN_ANY = 0,
    APK64_ACONFIGURATION_KEYSHIDDEN_NO = 1,
    APK64_ACONFIGURATION_KEYSHIDDEN_YES = 2,
    APK64_ACONFIGURATION_KEYSHIDDEN_SOFT = 3,
    APK64_ACONFIGURATION_NAVHIDDEN_ANY = 0,
    APK64_ACONFIGURATION_NAVHIDDEN_NO = 1,
    APK64_ACONFIGURATION_NAVHIDDEN_YES = 2,
    APK64_ACONFIGURATION_SCREENSIZE_ANY = 0,
    APK64_ACONFIGURATION_SCREENSIZE_SMALL = 1,
    APK64_ACONFIGURATION_SCREENSIZE_NORMAL = 2,
    APK64_ACONFIGURATION_SCREENSIZE_LARGE = 3,
    APK64_ACONFIGURATION_SCREENSIZE_XLARGE = 4,
    APK64_ACONFIGURATION_SCREENLONG_ANY = 0,
    APK64_ACONFIGURATION_SCREENLONG_NO = 1,
    APK64_ACONFIGURATION_SCREENLONG_YES = 2,
    APK64_ACONFIGURATION_UI_MODE_TYPE_ANY = 0,
    APK64_ACONFIGURATION_UI_MODE_TYPE_NORMAL = 1,
    APK64_ACONFIGURATION_UI_MODE_TYPE_DESK = 2,
    APK64_ACONFIGURATION_UI_MODE_TYPE_CAR = 3,
    APK64_ACONFIGURATION_UI_MODE_TYPE_TELEVISION = 4,
    APK64_ACONFIGURATION_UI_MODE_TYPE_APPLIANCE = 5,
    APK64_ACONFIGURATION_UI_MODE_TYPE_WATCH = 6,
    APK64_ACONFIGURATION_UI_MODE_TYPE_VR_HEADSET = 7,
    APK64_ACONFIGURATION_UI_MODE_NIGHT_ANY = 0,
    APK64_ACONFIGURATION_UI_MODE_NIGHT_NO = 1,
    APK64_ACONFIGURATION_UI_MODE_NIGHT_YES = 2,
    APK64_ACONFIGURATION_LAYOUTDIR_ANY = 0,
    APK64_ACONFIGURATION_LAYOUTDIR_LTR = 1,
    APK64_ACONFIGURATION_LAYOUTDIR_RTL = 2,
    APK64_ACONFIGURATION_SCREENROUND_ANY = 0,
    APK64_ACONFIGURATION_SCREENROUND_NO = 1,
    APK64_ACONFIGURATION_SCREENROUND_YES = 2,
};

enum {
    APK64_AASSET_MODE_UNKNOWN = 0,
    APK64_AASSET_MODE_RANDOM = 1,
    APK64_AASSET_MODE_STREAMING = 2,
    APK64_AASSET_MODE_BUFFER = 3,
};

/* Initialize the process-global asset manager. Either source may be NULL/empty.
 * If both are present, the extracted directory has priority over APK/ZIP on
 * name collisions. APK paths are resolved from the standard assets/ prefix. */
Apk64Result apk64_assets_init(const char *directory_root, const char *apk_path);
void apk64_assets_reset(void);
Apk64AssetManager *apk64_assets_manager(void);
size_t apk64_assets_backend_count(void);
size_t apk64_assets_zip_entry_count(void);
bool apk64_assets_set_configuration(Apk64AssetManager *manager,
                                    const Apk64AssetConfiguration *configuration);
bool apk64_assets_get_configuration(const Apk64AssetManager *manager,
                                    Apk64AssetConfiguration *out_configuration);

Apk64Asset *apk64_asset_open(Apk64AssetManager *manager, const char *filename, int mode);
int apk64_asset_read(Apk64Asset *asset, void *buffer, size_t count);
int64_t apk64_asset_seek64(Apk64Asset *asset, int64_t offset, int whence);
int64_t apk64_asset_length64(const Apk64Asset *asset);
int64_t apk64_asset_remaining64(const Apk64Asset *asset);
const void *apk64_asset_buffer(Apk64Asset *asset);
int apk64_asset_is_allocated(const Apk64Asset *asset);
int apk64_asset_open_fd64(Apk64Asset *asset, int64_t *start, int64_t *length);
/* Host file apk64_asset_open_fd64 opens (APK or extracted file), or NULL when
 * the asset cannot be exposed as a descriptor (compressed APK entry). */
const char *apk64_asset_fd_host_path(const Apk64Asset *asset);
void apk64_asset_close(Apk64Asset *asset);
/* Takes ownership of data (freed by apk64_asset_close). */
Apk64Asset *apk64_asset_from_owned_buffer(uint8_t *data, size_t length);

Apk64AssetDir *apk64_asset_open_dir(Apk64AssetManager *manager, const char *dirname);
const char *apk64_asset_dir_next(Apk64AssetDir *dir);
void apk64_asset_dir_rewind(Apk64AssetDir *dir);
void apk64_asset_dir_close(Apk64AssetDir *dir);

/* Android NDK ABI entry points exported through virtual libandroid.so. */
void *AAssetManager_fromJava(void *env, void *asset_manager_object);
Apk64Asset *AAssetManager_open(Apk64AssetManager *manager, const char *filename, int mode);
Apk64AssetDir *AAssetManager_openDir(Apk64AssetManager *manager, const char *dirname);
const char *AAssetDir_getNextFileName(Apk64AssetDir *dir);
void AAssetDir_rewind(Apk64AssetDir *dir);
void AAssetDir_close(Apk64AssetDir *dir);
int AAsset_read(Apk64Asset *asset, void *buffer, size_t count);
int64_t AAsset_seek(Apk64Asset *asset, int64_t offset, int whence);
int64_t AAsset_seek64(Apk64Asset *asset, int64_t offset, int whence);
int64_t AAsset_getLength(Apk64Asset *asset);
int64_t AAsset_getLength64(Apk64Asset *asset);
int64_t AAsset_getRemainingLength(Apk64Asset *asset);
int64_t AAsset_getRemainingLength64(Apk64Asset *asset);
const void *AAsset_getBuffer(Apk64Asset *asset);
int AAsset_openFileDescriptor(Apk64Asset *asset, int64_t *start, int64_t *length);
int AAsset_openFileDescriptor64(Apk64Asset *asset, int64_t *start, int64_t *length);
int AAsset_isAllocated(Apk64Asset *asset);
void AAsset_close(Apk64Asset *asset);


/* audio interface */


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define APK64_AUDIO_WAVE_SAMPLES 128

typedef size_t (*Apk64AudioRenderFn)(int16_t *stereo_pcm, size_t frames, void *user);


typedef enum Apk64AudioMode {
    APK64_AUDIO_SILENCE = 0,
    APK64_AUDIO_LEFT = 1,
    APK64_AUDIO_RIGHT = 2,
    APK64_AUDIO_STEREO = 3,
} Apk64AudioMode;

typedef struct Apk64AudioSnapshot {
    bool initialized;
    bool running;
    bool paused;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t pcm_format;
    Apk64AudioMode mode;
    float frequency_hz;
    float volume;
    float peak_left;
    float peak_right;
    uint32_t queued_buffers;
    uint64_t played_samples;
    uint32_t underruns;
    float wave_left[APK64_AUDIO_WAVE_SAMPLES];
    float wave_right[APK64_AUDIO_WAVE_SAMPLES];
} Apk64AudioSnapshot;

typedef struct Apk64AudioSessionStats {
    bool init_ok;
    bool format_ok;
    bool saw_left;
    bool saw_right;
    bool saw_stereo;
    bool saw_volume_change;
    bool saw_pause_resume;
    bool saw_restart;
    uint64_t buffers_submitted;
    uint64_t buffers_released;
    uint64_t frames_generated;
    uint64_t played_samples;
    uint32_t underruns;
    uint32_t min_queued;
    bool worker_started;
    uint64_t worker_wakeups;
    uint64_t worker_refills;
    uint64_t worker_timeouts;
    uint64_t renderer_shortfall_frames;
    uint64_t max_service_gap_us;
    uint64_t max_refill_us;
} Apk64AudioSessionStats;

void apk64_audio_set_renderer(Apk64AudioRenderFn renderer, void *user);
bool apk64_audio_init(void);
bool apk64_audio_update(void);
bool apk64_audio_start_worker(void);
void apk64_audio_stop_worker(void);
bool apk64_audio_worker_running(void);
void apk64_audio_set_mode(Apk64AudioMode mode);
void apk64_audio_set_frequency(float hz);
bool apk64_audio_set_volume(float volume);
bool apk64_audio_toggle_pause(void);
bool apk64_audio_restart(void);
void apk64_audio_get_snapshot(Apk64AudioSnapshot *out);
void apk64_audio_get_stats(Apk64AudioSessionStats *out);
void apk64_audio_shutdown(void);


/* bitmap interface */


#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/* Android NDK android/bitmap.h ABI (including the API-30 flags word). */
typedef struct Apk64AndroidBitmapInfo {
    uint32_t width, height, stride;
    int32_t format;
    uint32_t flags;
} Apk64AndroidBitmapInfo;

#define BITMAP_PAYLOAD_MAGIC UINT64_C(0x41504b3642495431)
typedef struct BitmapPayload {
    uint64_t magic;
    int32_t width, height;
    uint32_t stride;
    int32_t format;
    uint32_t flags;
    _Atomic uint32_t locks;
    int32_t density;
    bool is_mutable, recycled;
    uint8_t *pixels;
    struct BitmapPayload *next;
} BitmapPayload;

/* Framework creation and NDK access share the same native pixel allocation. */
BitmapPayload *apk64_bitmap_alloc(int32_t width, int32_t height, int32_t config);
void apk64_bitmap_store_color(BitmapPayload *bitmap, int32_t x, int32_t y, uint32_t argb);
int AndroidBitmap_getInfo(void *env, void *bitmap, Apk64AndroidBitmapInfo *info);
int AndroidBitmap_lockPixels(void *env, void *bitmap, void **pixels);
int AndroidBitmap_unlockPixels(void *env, void *bitmap);


/* cocos_keyboard interface */


#include <stddef.h>


/* Generic Cocos/Android text-entry backend. On Switch this launches the libnx
 * software keyboard. Host builds preserve the supplied text so JNI/DEX bridge
 * regressions can run without a platform applet. */
Apk64Result apk64_cocos_keyboard_show(const char *initial_text,
                                      char *out_text,
                                      size_t out_text_size);


/* cocos_text interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct Apk64CocosTextRequest {
    const char *text;
    const char *font_name;
    int32_t font_size;
    float tint_r;
    float tint_g;
    float tint_b;
    float tint_a;
    int32_t alignment;
    int32_t width;
    int32_t height;
    float line_spacing;
    bool no_wrap;
    bool shrink;
    bool shrink_wrap;
    bool shadow;
    float shadow_dx;
    float shadow_dy;
    float shadow_blur;
    float shadow_opacity;
    bool stroke;
    float stroke_r;
    float stroke_g;
    float stroke_b;
    float stroke_a;
    float stroke_size;
} Apk64CocosTextRequest;

typedef struct Apk64CocosTextBitmap {
    int32_t width;
    int32_t height;
    uint8_t *pixels; /* Android-like RGBA_8888 byte order, premultiplied alpha. */
    size_t byte_count;
    size_t line_count;
    int32_t font_size; /* selected size after overflow fitting */
    char font_source[32];
} Apk64CocosTextBitmap;

/* Generic Cocos2d-x Android bitmap-text compatibility. The Switch backend uses
 * FreeType and prefers a requested .ttf/.otf APK asset, falling back to the
 * system shared font. The host implementation is deterministic and intentionally
 * simple so regressions can validate sizing/pixel/callback plumbing without a
 * Switch service dependency. */
Apk64Result apk64_cocos_text_rasterize(const Apk64CocosTextRequest *request,
                                       Apk64CocosTextBitmap *out_bitmap);
void apk64_cocos_text_bitmap_free(Apk64CocosTextBitmap *bitmap);

/* android.graphics.Paint text queries against the default typeface (system
 * font): unhinted advance width of UTF-8 text at font_size pixels, and whether
 * every code point has a glyph. Weak so framework-only host links omit them. */
Apk64Result apk64_cocos_text_measure(const char *text, float font_size, float *out_width) __attribute__((weak));
Apk64Result apk64_cocos_text_has_glyphs(const char *text, bool *out_has) __attribute__((weak));


/* compat interface */


#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>


struct Apk64Linker;
struct Apk64Module;
struct stat;

/* Generic host compatibility surface introduced in Stage 4 and extended in Stage 5.
 *
 * Android system libraries such as libc.so, libandroid.so, libEGL.so and libGLESv2/3.so are virtual providers inside APK64:
 * they satisfy DT_NEEDED without being loaded as guest ELF files. Individual
 * undefined symbols are then resolved through the compatibility export table.
 */
bool apk64_compat_is_virtual_library(const char *name);

Apk64Result apk64_compat_resolve(const char *name,
                                 uintptr_t *out_address,
                                 const char **out_provider);

size_t apk64_compat_export_count(void);
void apk64_compat_set_active_linker(struct Apk64Linker *linker);
void apk64_compat_pin_guest_address(const void *address);

/* Strict 0.0.15 rule: this interface intentionally exposes no API for
 * application/engine semantic rewrites. Guest executable code must remain
 * unchanged except for architecture-level execution translations owned by the
 * loader itself. */

/* Configure the current Android application's persistent host storage. Guest
 * native code may use normal Android absolute paths (/data/data/<pkg>,
 * /data/user/0/<pkg>, /sdcard) while APK64 stores the bytes beneath the
 * per-package Switch runtime root. Passing NULL clears the mapping. */
void apk64_compat_set_app_storage(const char *package_name, const char *app_root);

/* Configure the host APK backing Android's guest-visible package code path.
 * Java APIs expose a normal Android absolute path (/data/app/<pkg>/base.apk);
 * libc/stat/open calls on that path are translated back to the installed APK. */
bool apk64_compat_set_package_apk(const char *host_apk_path);
const char *apk64_compat_guest_package_apk(void);

typedef struct Apk64CompatStorageStats {
    uint64_t mapped_paths;
    uint64_t write_opens;
    uint64_t mutations;
} Apk64CompatStorageStats;
void apk64_compat_get_storage_stats(Apk64CompatStorageStats *out_stats);

/* Host-layout metadata through the same open-file-aware storage boundary used
 * by guest native stat; the caller has already mapped the Android path. */
int apk64_compat_stat_host_path(const char *host_path, struct stat *st);

/* Host heap/process budget diagnostics; does not change guest memory policy. */
void apk64_compat_log_memory_state(const char *reason);

/* Generic Android networking status/trust helpers. The socket compatibility
 * layer owns Switch BSD/NIFM initialization so the Java framework and guest
 * native code observe one process-wide network state. */
bool apk64_compat_network_is_available(void);
int apk64_compat_network_initialize(void); /* Initialize shared BSD/NIFM services without a connectivity query. */
void apk64_compat_network_shutdown(void); /* socketExit/nifmExit at process end */

/* Guest Android auxv view. Kept public inside APK64 so the ELF IFUNC resolver
 * bridge and guest getauxval() export consume exactly the same HWCAP/HWCAP2
 * values. */
unsigned long apk64_bionic_getauxval(unsigned long type);
const char *apk64_compat_network_ca_bundle(void);

/* Android Java TLS obtains platform trust anchors through the Java
 * TrustManagerFactory/X509TrustManager API rather than SSL_CERT_FILE.  Expose
 * the same firmware-root DER set to the framework without weakening
 * certificate or hostname verification. */
size_t apk64_compat_network_ca_count(void);
bool apk64_compat_network_ca_copy(size_t index, uint8_t *out, size_t capacity, size_t *out_size);

/* DNS servers of the active network as IPv4 addresses in network byte order
 * (in_addr.s_addr). Backs android.net.LinkProperties.getDnsServers, the only
 * API 26+ source of resolver addresses for apps with their own DNS client. */
size_t apk64_compat_network_dns_servers(uint32_t *out, size_t capacity);
/* Active-network HTTP proxy: 1 configured, 0 absent, -1 query failed.
 * Copies only host/port; credentials remain owned by the host. */
int apk64_compat_network_proxy(char *host, size_t capacity, uint16_t *port);

/* Open one Android guest path through APK64's package/storage namespace.
 * The returned descriptor is a normal host-backed descriptor owned by caller. */
int apk64_compat_open_guest_readonly(const char *path);
bool apk64_compat_map_guest_path(const char *guest_path, char *out, size_t cap);

/* Android arm64 uses TPIDR_EL0 for Bionic's fixed TLS slots. Horizon/libnx
 * uses TPIDRRO_EL0 for its own TLS, so APK64 provides a separate Android TLS
 * block in TPIDR_EL0 before guest AArch64 executes. Slot 5 is the stack
 * protector guard used by NDK Clang-generated code. */
void apk64_bionic_tls_activate_current(void);
void *apk64_bionic_tls_current_base(void);
uintptr_t apk64_bionic_tls_stack_guard(void);
bool apk64_bionic_tls_validate_current(void);
Apk64Result apk64_bionic_tls_register_module(struct Apk64Module *module);
void *apk64_bionic_tls_get_addr(size_t module_id, size_t offset);
void *apk64_bionic___tls_get_addr(const void *tls_index);

/* Bionic / Itanium C++ ABI destructor registration. These are process-wide
 * compatibility services, not per-title shims. */
int apk64_bionic_cxa_atexit(void (*func)(void *), void *arg, void *dso);
int apk64_bionic_cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso);
void apk64_bionic_cxa_thread_finalize_current(void);
/* Exit sequence for an adopted (non-Bionic) host thread; call before it returns. */
void apk64_bionic_thread_external_exit(void);
void apk64_bionic_cxa_finalize(void *dso);
int apk64_bionic_cxa_guard_acquire(uint64_t *guard);
void apk64_bionic_cxa_guard_release(uint64_t *guard);
void apk64_bionic_cxa_guard_abort(uint64_t *guard);
void apk64_bionic_cxa_pure_virtual(void) __attribute__((noreturn));
size_t apk64_bionic_cxa_pending_count(void);
void apk64_bionic_cxa_reset(void);

/* Generic Switch thread-placement policy. Guest workers may use the full
 * application-core mask until a runtime-owned renderer explicitly reserves
 * one core; only then are workers constrained to the remaining cores. Host
 * builds are reset/no-op and pin helpers return -1. */
void apk64_thread_policy_reset(void);
int apk64_thread_policy_pin_render(void);
int apk64_thread_policy_pin_background(void);

/* Process-kill state. While set, a host thread that exits because the
 * simulated Android process is being killed does not run guest pthread-key
 * destructors: a killed Android process never runs them. */
void apk64_bionic_set_process_terminating(bool terminating);

/* Stage 5 translated Bionic pthread/TLS/semaphore entry points. */
typedef uint64_t Apk64GuestPthread;
typedef int32_t Apk64GuestKey;
typedef int32_t Apk64GuestOnce;

/* Android/Bionic arm64 pthread_attr_t public ABI. Keep this layout shared
 * between the attribute wrappers and the guest-thread registry so
 * pthread_getattr_np() can return actual thread state without host casts. */
#define APK64_BIONIC_PTHREAD_ATTR_SIZE 56u
#define APK64_BIONIC_PTHREAD_ATTR_DETACHED 0x1u
#define APK64_BIONIC_PTHREAD_ATTR_JOINED   0x2u
#define APK64_BIONIC_PTHREAD_ATTR_INHERIT  0x4u
#define APK64_BIONIC_PTHREAD_ATTR_EXPLICIT 0x8u
#define APK64_BIONIC_PAGE_SIZE 4096u
#define APK64_BIONIC_PTHREAD_STACK_MIN (4u * APK64_BIONIC_PAGE_SIZE)
/* Android 11 arm64 Bionic reserves 32 KiB of the traditional 1 MiB budget
 * for its alternate signal stack. This is the observable pthread_attr_init
 * default stored in pthread_attr_t. */
#define APK64_BIONIC_PTHREAD_STACK_DEFAULT ((1024u * 1024u) - (32u * 1024u))
typedef struct Apk64BionicPthreadAttrLayout {
    uint32_t flags;
    uint32_t _pad_flags;
    void *stack_base;
    size_t stack_size;
    size_t guard_size;
    int32_t sched_policy;
    int32_t sched_priority;
    uint8_t _reserved[16];
} Apk64BionicPthreadAttrLayout;
#ifdef __cplusplus
static_assert(sizeof(Apk64BionicPthreadAttrLayout) == APK64_BIONIC_PTHREAD_ATTR_SIZE,
              "Bionic arm64 pthread_attr_t ABI size");
#else
_Static_assert(sizeof(Apk64BionicPthreadAttrLayout) == APK64_BIONIC_PTHREAD_ATTR_SIZE,
               "Bionic arm64 pthread_attr_t ABI size");
#endif

int apk64_bionic_pthread_create(Apk64GuestPthread *out_thread, const void *guest_attr,
                                void *(*start_routine)(void *), void *arg);
int apk64_bionic_pthread_join(Apk64GuestPthread guest_thread, void **retval);
int apk64_bionic_pthread_detach(Apk64GuestPthread guest_thread);
Apk64GuestPthread apk64_bionic_pthread_self(void);
int apk64_bionic_pthread_equal(Apk64GuestPthread a, Apk64GuestPthread b);
int apk64_bionic_pthread_setname_np(Apk64GuestPthread guest_thread, const char *name);
int apk64_bionic_pthread_getname_np(Apk64GuestPthread guest_thread, char *name, size_t len);
int apk64_bionic_pthread_gettid_np(Apk64GuestPthread guest_thread);
int apk64_bionic_pthread_getschedparam(Apk64GuestPthread guest_thread, int *policy, struct sched_param *param);
int apk64_bionic_pthread_setschedparam(Apk64GuestPthread guest_thread, int policy, const struct sched_param *param);
int apk64_bionic_pthread_setschedprio(Apk64GuestPthread guest_thread, int priority);
int apk64_bionic_pthread_getattr_np(Apk64GuestPthread guest_thread, void *attr);

/* Linux/Bionic nice priority ABI. APK64 models PRIO_PROCESS for the current
 * guest thread and maps it to the real host scheduler on Switch. */
int apk64_bionic_gettid(void);
int apk64_bionic_getpriority(int which, int who);
int apk64_bionic_setpriority(int which, int who, int prio);
/* Raw Linux/Android arm64 syscall bridges used by libc++/engine code that
 * intentionally calls syscall(2) instead of a public pthread/sched wrapper. */
long apk64_bionic_futex(uint32_t *uaddr, int op, uint32_t val,
                        const void *timeout, uint32_t *uaddr2, uint32_t val3);
/* Inline SVC ABI: negative Linux errno in x0; caller's errno is preserved. */
long apk64_bionic_raw_syscall(long number, uintptr_t a0, uintptr_t a1,
                             uintptr_t a2, uintptr_t a3, uintptr_t a4,
                             uintptr_t a5, uintptr_t caller);
/* Internal syscall entry: preserve the guest caller and its frame for wait diagnostics. */
long apk64_bionic_futex_at(uint32_t *uaddr, int op, uint32_t val,
                           const void *timeout, uint32_t *uaddr2, uint32_t val3,
                           uintptr_t caller, const void *frame);
int apk64_bionic_sched_setaffinity(int pid, size_t cpusetsize, const void *mask);
int apk64_bionic_sched_getaffinity(int pid, size_t cpusetsize, void *mask);
unsigned apk64_compat_cpu_count(void);
/* Reclaim exited native/Java host workers and excess idle signal proxies on
 * resource exhaustion. Guest joinable results survive host cleanup.
 * Use for native and framework workers; signal proxies use raw pthread_create. */
int apk64_host_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                              void *(*start)(void *), void *arg);
int apk64_bionic_thread_set_current_name(const char *name);
int apk64_bionic_thread_get_current_name(char out[16]);
void apk64_bionic_pthread_exit(void *retval);
size_t apk64_bionic_pthread_live_guest_count(void);
bool apk64_bionic_pthread_wait_guest_idle(uint32_t timeout_ms);
/* Diagnostics: log each live registered thread and, for threads blocked in a
 * Bionic wait, the primitive, object, duration and guest call site. */
size_t apk64_bionic_pthread_log_wait_states(const char *reason);
/* Diagnostics (sampling profiler): one entry per live registered thread;
 * waiting is true while it is inside a tracked blocking primitive. */
typedef struct Apk64ThreadSample {
    pthread_t host;
    uint64_t guest_id; /* Stable registry generation, including recycled host handles. */
    char name[16];
    bool waiting;
    uint64_t cpu_ticks; /* kernel CPU time (system ticks), 0 when unavailable */
    bool cpu_valid;     /* zero CPU usage and unavailable counters are distinct */
} Apk64ThreadSample;
size_t apk64_bionic_thread_samples(Apk64ThreadSample *out, size_t cap);
typedef struct Apk64ThreadScheduling {
    uint64_t guest_id, effective_mask;
    int android_tid, ideal_core, host_priority;
    uint32_t core_result, priority_result; /* raw host diagnostic results */
} Apk64ThreadScheduling;
/* Low-frequency diagnostics; query under the registry's lifetime lock. */
bool apk64_bionic_thread_scheduling(pthread_t host, Apk64ThreadScheduling *out) __attribute__((weak));
void apk64_bionic_thread_diagnostics_report(void) __attribute__((weak));
/* Diagnostics only: briefly pause/resume an idle-signal-state thread to copy
 * kernel registers. Serializes with signal delivery and pins its lifetime.
 * The optional probe reads only validated permanent allocator storage. */
#define APK64_NATIVE_STACK_CAP 8u
typedef struct Apk64NativeContext {
    uintptr_t pc, lr, sp, fp;
    uintptr_t x[29];
    uintptr_t probe_address;
    uint64_t probe_words[24];
    uintptr_t stack[APK64_NATIVE_STACK_CAP]; /* PC, LR, then validated frame records. */
    unsigned stack_count, stack_status; /* 0=end, 1=range/alignment, 2=capacity, 3=no registered stack */
    unsigned capture_status; /* stack API: 0=success, 1=registry, 2=signal/lifetime, 3=pause, 4=context, 5=resume */
    uint32_t pause_result, context_result, resume_result;
} Apk64NativeContext;
int apk64_bionic_thread_sample_context(pthread_t host, Apk64NativeContext *out);
int apk64_bionic_thread_sample_stack(pthread_t host, Apk64NativeContext *out);
/* Also copy x19+960..1151 from permanent non-page allocator storage when valid.
 * Raw diagnostic words; callers must not assume a particular guest type. */
int apk64_bionic_thread_sample_context_probe(pthread_t host, Apk64NativeContext *out);
int apk64_bionic_thread_sample_pc(pthread_t host, uintptr_t *pc, uintptr_t *lr, uintptr_t *sp);

typedef struct Apk64ThreadDiagSummary {
    size_t live;
    size_t waiting;
    size_t active;
} Apk64ThreadDiagSummary;
/* Diagnostic-only snapshot. waiting means the thread is inside an APK64-modeled
 * Bionic wait; active means a runtime activity label is currently published. */
void apk64_bionic_pthread_diag_summary(Apk64ThreadDiagSummary *out_summary);

/* Diagnostics: publish the runtime call the current registered thread is
 * running (for example a Java native method) so stall reports can place a
 * thread blocked outside any Bionic wait. Enter saves the previous activity
 * into *saved; leave restores it, so nested calls report the innermost one. */
#define APK64_THREAD_ACTIVITY_TEXT 112
typedef struct Apk64ThreadActivity {
    char text[APK64_THREAD_ACTIVITY_TEXT];
    uint64_t since_ms;
} Apk64ThreadActivity;
void apk64_bionic_thread_activity_enter(Apk64ThreadActivity *saved, const char *kind,
                                        const char *class_name, const char *method_name);
void apk64_bionic_thread_activity_leave(const Apk64ThreadActivity *saved);

/* Stack range of the current registered thread, for bounded frame-record
 * walks in diagnostics. False when the thread or its stack is unknown. */
bool apk64_bionic_current_stack_bounds(uintptr_t *out_low, uintptr_t *out_high);
bool apk64_compat_describe_guest_address(uintptr_t address, char *out, size_t cap);
/* Last failed W^X page transition (Switch), for crash reports. */
bool apk64_wx_describe_failure(char *out, size_t cap);

int apk64_bionic_pthread_attr_init(void *attr);
int apk64_bionic_pthread_attr_destroy(void *attr);
int apk64_bionic_pthread_attr_setdetachstate(void *attr, int state);
int apk64_bionic_pthread_attr_getdetachstate(const void *attr, int *state);
int apk64_bionic_pthread_attr_setstacksize(void *attr, size_t stack_size);
int apk64_bionic_pthread_attr_getstacksize(const void *attr, size_t *stack_size);
int apk64_bionic_pthread_attr_setguardsize(void *attr, size_t guard_size);
int apk64_bionic_pthread_attr_getguardsize(const void *attr, size_t *guard_size);
int apk64_bionic_pthread_attr_getstack(const void *attr, void **stack_base, size_t *stack_size);
int apk64_bionic_pthread_attr_setstack(void *attr, void *stack_base, size_t stack_size);
int apk64_bionic_pthread_attr_setinheritsched(void *attr, int inherit);
int apk64_bionic_pthread_attr_getinheritsched(const void *attr, int *inherit);
int apk64_bionic_pthread_attr_setschedpolicy(void *attr, int policy);
int apk64_bionic_pthread_attr_getschedpolicy(const void *attr, int *policy);
int apk64_bionic_pthread_attr_setschedparam(void *attr, const struct sched_param *param);
int apk64_bionic_pthread_attr_getschedparam(const void *attr, struct sched_param *param);
int apk64_bionic_pthread_attr_setscope(void *attr, int scope);
int apk64_bionic_pthread_attr_getscope(const void *attr, int *scope);
int apk64_bionic_pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

typedef struct Apk64BionicPthreadCleanup {
    struct Apk64BionicPthreadCleanup *cleanup_prev;
    void (*cleanup_routine)(void *);
    void *cleanup_arg;
} Apk64BionicPthreadCleanup;
void apk64_bionic_pthread_cleanup_push(Apk64BionicPthreadCleanup *cleanup, void (*routine)(void *), void *arg);
void apk64_bionic_pthread_cleanup_pop(Apk64BionicPthreadCleanup *cleanup, int execute);

int apk64_bionic_pthread_mutex_init(void *guest_mutex, const long *guest_attr);
int apk64_bionic_pthread_mutex_destroy(void *guest_mutex);
int apk64_bionic_pthread_mutex_lock(void *guest_mutex);
int apk64_bionic_pthread_mutex_trylock(void *guest_mutex);
int apk64_bionic_pthread_mutex_timedlock(void *guest_mutex, const void *guest_abstime);
int apk64_bionic_pthread_mutex_timedlock_monotonic_np(void *guest_mutex, const void *guest_abstime);
int apk64_bionic_pthread_mutex_clocklock(void *guest_mutex, int guest_clock_id, const void *guest_abstime);
int apk64_bionic_pthread_mutex_unlock(void *guest_mutex);

int apk64_bionic_pthread_condattr_init(long *guest_attr);
int apk64_bionic_pthread_condattr_destroy(long *guest_attr);
int apk64_bionic_pthread_condattr_getclock(const long *guest_attr, int *clock_id);
int apk64_bionic_pthread_condattr_setclock(long *guest_attr, int clock_id);
int apk64_bionic_pthread_condattr_getpshared(const long *guest_attr, int *pshared);
int apk64_bionic_pthread_condattr_setpshared(long *guest_attr, int pshared);
int apk64_bionic_pthread_cond_init(void *guest_cond, const long *guest_attr);
int apk64_bionic_pthread_cond_destroy(void *guest_cond);
int apk64_bionic_pthread_cond_wait(void *guest_cond, void *guest_mutex);
int apk64_bionic_pthread_cond_timedwait(void *guest_cond, void *guest_mutex, const void *guest_abstime);
int apk64_bionic_pthread_cond_timedwait_monotonic_np(void *guest_cond, void *guest_mutex, const void *guest_abstime);
int apk64_bionic_pthread_cond_timedwait_relative_np(void *guest_cond, void *guest_mutex, const void *guest_reltime);
int apk64_bionic_pthread_cond_clockwait(void *guest_cond, void *guest_mutex, int guest_clock_id, const void *guest_abstime);
int apk64_bionic_pthread_cond_signal(void *guest_cond);
int apk64_bionic_pthread_cond_broadcast(void *guest_cond);

int apk64_bionic_pthread_rwlockattr_init(long *guest_attr);
int apk64_bionic_pthread_rwlockattr_destroy(long *guest_attr);
int apk64_bionic_pthread_rwlockattr_getpshared(const long *guest_attr, int *pshared);
int apk64_bionic_pthread_rwlockattr_setpshared(long *guest_attr, int pshared);
int apk64_bionic_pthread_rwlockattr_getkind_np(const long *guest_attr, int *kind);
int apk64_bionic_pthread_rwlockattr_setkind_np(long *guest_attr, int kind);
int apk64_bionic_pthread_rwlock_init(void *guest_rwlock, const long *guest_attr);
int apk64_bionic_pthread_rwlock_destroy(void *guest_rwlock);
int apk64_bionic_pthread_rwlock_rdlock(void *guest_rwlock);
int apk64_bionic_pthread_rwlock_timedrdlock(void *guest_rwlock, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_timedrdlock_monotonic_np(void *guest_rwlock, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_clockrdlock(void *guest_rwlock, int guest_clock_id, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_tryrdlock(void *guest_rwlock);
int apk64_bionic_pthread_rwlock_wrlock(void *guest_rwlock);
int apk64_bionic_pthread_rwlock_timedwrlock(void *guest_rwlock, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_timedwrlock_monotonic_np(void *guest_rwlock, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_clockwrlock(void *guest_rwlock, int guest_clock_id, const void *guest_abstime);
int apk64_bionic_pthread_rwlock_trywrlock(void *guest_rwlock);
int apk64_bionic_pthread_rwlock_unlock(void *guest_rwlock);

int apk64_bionic_pthread_spin_init(void *guest_spin, int pshared);
int apk64_bionic_pthread_spin_destroy(void *guest_spin);
int apk64_bionic_pthread_spin_lock(void *guest_spin);
int apk64_bionic_pthread_spin_trylock(void *guest_spin);
int apk64_bionic_pthread_spin_unlock(void *guest_spin);

int apk64_bionic_pthread_barrierattr_init(long *guest_attr);
int apk64_bionic_pthread_barrierattr_destroy(long *guest_attr);
int apk64_bionic_pthread_barrierattr_getpshared(const long *guest_attr, int *pshared);
int apk64_bionic_pthread_barrierattr_setpshared(long *guest_attr, int pshared);
int apk64_bionic_pthread_barrier_init(void *guest_barrier, const long *guest_attr, unsigned count);
int apk64_bionic_pthread_barrier_destroy(void *guest_barrier);
int apk64_bionic_pthread_barrier_wait(void *guest_barrier);

int apk64_bionic_pthread_once(Apk64GuestOnce *once_control, void (*init_routine)(void));
int apk64_bionic_pthread_key_create(Apk64GuestKey *out_key, void (*destructor)(void *));
int apk64_bionic_pthread_key_delete(Apk64GuestKey key);
int apk64_bionic_pthread_setspecific(Apk64GuestKey key, const void *value);
void *apk64_bionic_pthread_getspecific(Apk64GuestKey key);

void *apk64_bionic_emutls_get_address(void *control);

int apk64_bionic_sem_init(void *guest_sem, int pshared, unsigned value);
int apk64_bionic_sem_destroy(void *guest_sem);
int apk64_bionic_sem_wait(void *guest_sem);
int apk64_bionic_sem_trywait(void *guest_sem);
int apk64_bionic_sem_timedwait(void *guest_sem, const void *guest_abstime);
int apk64_bionic_sem_post(void *guest_sem);
int apk64_bionic_sem_getvalue(void *guest_sem, int *out_value);

/* errno ABI boundary (compat_threads.c).
 *
 * Guest code is compiled against Linux/Bionic errno numbers; the host libc
 * (newlib on Switch) agrees only for 1..34 (EPERM..ERANGE). The guest sees a
 * per-thread errno slot returned by Bionic's __errno(). Host code keeps using
 * host `errno` with host constants; __errno() imports any host errno raised
 * since the last import (translated to Bionic numbering) and leaves a
 * sentinel behind so nothing is translated twice. Functions that *return* an
 * error number (pthread_*) translate with apk64_errno_host_to_android(). */
int apk64_errno_host_to_android(int host_value);
int apk64_errno_android_to_host(int android_value);
int *apk64_guest_errno_location(void);  /* Bionic __errno() */
void apk64_guest_errno_publish(void);   /* import pending host errno now */

/* Linux/Bionic signal runtime (compat_threads.c). Android arm64 layouts:
 * sigset_t 8, struct sigaction 32, stack_t 24 bytes.
 *
 * Horizon has no Linux signals. APK64 keeps POSIX signal state per guest
 * thread and delivers a thread-directed signal (pthread_kill/raise) the way
 * the Linux kernel does: the guest handler runs on the target thread's stack,
 * below its stack pointer, as that thread, while the thread itself is stopped.
 * A target blocked in an APK64 wait is handled at that safe point; a target
 * executing guest code is paused for the duration of the handler. */
#define APK64_BIONIC_NSIG 65
int apk64_bionic_sigaction(int signo, const void *new_action, void *old_action);
int apk64_bionic_sigaltstack(const void *new_stack, void *old_stack);
int apk64_bionic_raise(int signo);
int apk64_bionic_pthread_kill(Apk64GuestPthread thread, int signo);
int apk64_bionic_pthread_sigmask(int how, const uint64_t *set, uint64_t *old_set);
int apk64_bionic_sigprocmask(int how, const uint64_t *set, uint64_t *old_set);
int apk64_bionic_sigsuspend(const uint64_t *mask);
int apk64_bionic_sigpending(uint64_t *set);
/* Low 32 signals of the calling thread's mask, for the setjmp bridge. */
uint32_t apk64_guest_signal_mask_get32(void);
void apk64_guest_signal_mask_set32(uint32_t mask);

/* Run a blocking host wait as a signal safe point: while fn blocks, a pending
 * handler for the calling guest thread may run on its behalf. fn must not
 * execute guest code. errno from fn is preserved. */
long apk64_signal_safe_call(long (*fn)(void *), void *ctx);
/* pthread_cond_timedwait (deadline NULL: untimed) as a safe point. While a
 * handler runs for the caller, the mutex is released; it is held again on
 * return, which then reports a spurious wakeup (0) or the timeout. */
int apk64_signal_safe_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                     const struct timespec *deadline);


/* dex_vm interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#ifndef APK64_DEX_SIGNATURE_CAP
#define APK64_DEX_SIGNATURE_CAP 2048u
#endif

typedef Apk64Result (*Apk64DexNativeResolver)(const char *class_name,
                                                const char *method_name,
                                                const char *signature,
                                                void **out_fn,
                                                bool *out_found,
                                                void *user_data);

/* Optional resolver for ordinary JNI native methods declared in DEX. RegisterNatives
 * bindings are checked first; the resolver is then used for exported Java_* symbols
 * or another runtime-specific native namespace. */
/* Verifies the AArch64 JNI call bridge (register and stack argument
 * placement) against a compiler-generated callee. Returns true when every
 * argument arrives intact or on hosts without the AArch64 bridge. */
bool apk64_dex_native_bridge_selftest(char *detail, size_t detail_cap);
/* Install before guest Java starts; clear only after guest workers are idle.
 * Lifecycle callbacks and diagnostic preflight share this process binding. */
void apk64_dex_set_native_resolver(Apk64DexNativeResolver resolver, void *user_data);
void apk64_dex_get_native_resolver_cache_stats(uint64_t *out_hits, uint64_t *out_misses);

typedef struct Apk64DexRuntimeProgress {
    uint64_t instructions;
    uint64_t calls;
    uint64_t native_calls;
    uint64_t last_update_ms;
    uint32_t pc;
    uint8_t opcode;
    char kind[16];
    char class_name[256];
    char method_name[128];
    char signature[256];
} Apk64DexRuntimeProgress;
/* Process-wide diagnostic execution progress. DEX instruction publication is
 * batched so observation does not add an atomic operation to every opcode. */
void apk64_dex_get_runtime_progress(Apk64DexRuntimeProgress *out_progress);
/* Stops and joins the sampling profiler thread (idempotent). Called before the
 * process returns to the loader, which unmaps the NRO. */
void apk64_dex_profiler_shutdown(void);
/* Start explicitly for native-heavy applications too; OFF starts no worker. */
void apk64_dex_profiler_start(void);
Apk64Result apk64_dex_prewarm_native_method(const char *class_name,
                                            const char *method_name,
                                            const char *signature,
                                            bool *out_found);

/* Invoke an already modeled/exported static JNI native method directly through
 * APK64's AArch64 JNI bridge. This is intended for engine adapters whose Java
 * method is proven to be a pure native forwarder (for example Cocos renderer
 * touch methods), avoiding a DEX round-trip on latency-sensitive input. */
Apk64Result apk64_dex_call_static_native_method(const char *source_path,
                                                 const char *class_name,
                                                 const char *method_name,
                                                 const char *signature,
                                                 void *env_handle,
                                                 const Apk64JniValue *args,
                                                 size_t arg_count,
                                                 Apk64JniValue *out_result);

typedef struct Apk64DexStackFrame {
    char class_name[512];
    char method_name[256];
} Apk64DexStackFrame;

/* Snapshot the package-DEX frames executing on the current host thread. Frames
 * are returned innermost-first, matching Thread.getStackTrace() ordering. The
 * runtime deliberately exposes only frames it can prove from its interpreter;
 * framework/native frames can be added later without fabricating source lines. */
size_t apk64_dex_current_stack(Apk64DexStackFrame *out_frames, size_t capacity);

/* Actual innermost executing DEX class for caller-sensitive platform checks.
 * Unlike diagnostic snapshots this is not truncated at a fixed stack depth.
 * Borrowed name, valid until that frame returns; NULL without a DEX caller. */
const char *apk64_dex_current_class_host(void);

typedef struct Apk64DexExecReport {
    bool completed;
    uint32_t pc;
    uint8_t opcode;
    size_t instructions;
    size_t calls;
    char class_name[512];
    char method_name[256];
    char signature[APK64_DEX_SIGNATURE_CAP];
    char detail[512];
} Apk64DexExecReport;

typedef struct Apk64DexPreflightReport {
    size_t methods_scanned;
    size_t invoke_refs;
    size_t package_targets;
    size_t framework_targets;
    size_t unresolved_methods;
    size_t unresolved_native_methods;
    size_t unsupported_opcode_sites;
    size_t unresolved_fields;
    bool truncated;
} Apk64DexPreflightReport;

/* Statically inventory direct invoke targets reachable from one package DEX
 * method without executing bytecode. Package-defined callees are traversed
 * recursively; framework/JNI targets are checked against the modeled registry.
 * Missing targets and bytecode sites not implemented by APK64's interpreter are
 * logged individually. The scan is diagnostic-only and execution remains strict. */
Apk64Result apk64_dex_preflight_method_tree(const char *source_path,
                                             const char *class_name,
                                             const char *method_name,
                                             const char *signature,
                                             bool is_static,
                                             Apk64DexPreflightReport *out_report);

/* Execute the manifest Activity's real DEX onCreate(Bundle) method through the
 * bounded APK64 interpreter. This is intentionally generic: package bytecode is
 * resolved from DEX class_data and framework calls are dispatched through the
 * JNI model. Unsupported bytecode/methods stop with a precise report. */
Apk64Result apk64_dex_execute_activity_on_create(const char *source_path,
                                                 const char *activity_class,
                                                 void *env_handle,
                                                 void *activity_ref,
                                                 Apk64DexExecReport *out_report);

/* Execute one ordinary instance method through the same DEX/framework/JNI
 * resolution path used by Activity.onCreate. This is the lifecycle/render
 * callback bridge for Java Activities and GLSurfaceView renderers. Float/double
 * arguments remain outside the current generic GPR ABI slice. */
Apk64Result apk64_dex_execute_instance_method(const char *source_path,
                                              const char *class_name,
                                              const char *method_name,
                                              const char *signature,
                                              void *env_handle,
                                              void *receiver_ref,
                                              const Apk64JniValue *args,
                                              size_t arg_count,
                                              Apk64JniValue *out_result,
                                              Apk64DexExecReport *out_report);

/* Allow guest native JNI code to resolve package-defined Java static methods
 * lazily through GetStaticMethodID/CallStatic*Method. The source remains active
 * until cleared with NULL. */
void apk64_dex_set_java_method_source(const char *source_path);
/* Active package source for lazy Java resolution, or NULL when none is set. */
const char *apk64_dex_java_method_source(void);
bool apk64_dex_class_access_flags(const char *class_name, uint32_t *out_class_flags);
bool apk64_dex_method_access_flags(const char *class_name, const char *method_name,
                                   const char *signature, uint32_t *out_method_flags,
                                   uint32_t *out_class_flags);

/* Complete one deferred Cocos IME commit on the runtime/GL thread. Switch's
 * software keyboard is a modal library applet, while Android's IME delivery is
 * asynchronous; the openIMEKeyboard bridge therefore stores the committed text
 * and this pump applies/verifies it on a later frame before nativeTextClosed(). */
Apk64Result apk64_dex_pump_cocos_keyboard_commit(bool *out_completed);

/* Initialize a package class (JLS 12.4.2): superclasses first, then its
 * <clinit>, once per process. Framework/host classes are a successful no-op.
 * Reflection uses this before touching a static field. */
Apk64Result apk64_dex_initialize_class(const char *source_path,
                                       const char *class_name,
                                       void *env_handle,
                                       Apk64DexExecReport *out_report);

/* Execute one ordinary static package method with full DEX register packing for
 * integer/object and float/double arguments. This is the backend used by the
 * native-JNI-to-DEX method bridge. */
Apk64Result apk64_dex_execute_static_method(const char *source_path,
                                            const char *class_name,
                                            const char *method_name,
                                            const char *signature,
                                            void *env_handle,
                                            const Apk64JniValue *args,
                                            size_t arg_count,
                                            Apk64JniValue *out_result,
                                            Apk64DexExecReport *out_report);

/* Linkage-failure policy. Default (false) follows ART: an unresolvable invoke
 * throws NoSuchMethodError / NoClassDefFoundError / UnsatisfiedLinkError into
 * the guest. true stops at the first unresolved member without throwing. */
void apk64_dex_set_strict_linkage(bool strict);
/* Distinct unresolved Java members recorded this run (first-fault ordered). */
size_t apk64_dex_unresolved_count(void);
void apk64_dex_log_unresolved_summary(void);


/* diagnostics interface */


#include <stdbool.h>
#include <stdarg.h>

bool apk64_diag_init(const char *log_path);
void apk64_diag_set_stdout(bool enabled);
void apk64_diag_set_quiet(bool enabled);
void apk64_diag_set_full_trace(bool enabled);
bool apk64_diag_full_trace_enabled(void);
void apk64_diag_flush(void);
/* Wake the writer without performing output I/O on the calling thread. */
void apk64_diag_request_flush(void);
void apk64_diag_shutdown(void);
/* Crash path (CPU exception handler): writes queued lines plus `text` straight
 * to the outputs and flushes, only try-locking (the caller may hold a lock). */
void apk64_diag_emergency_write(const char *text);
void apk64_log(const char *level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void apk64_vlog(const char *level, const char *fmt, va_list args);
void apk64_log_category(const char *category, const char *level, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define APK64_LOGI(...) apk64_log("INFO", __VA_ARGS__)
#define APK64_LOGW(...) apk64_log("WARN", __VA_ARGS__)
#define APK64_LOGE(...) apk64_log("ERROR", __VA_ARGS__)
#define APK64_LOGT(...) apk64_log("TEST", __VA_ARGS__)

#define APK64_LOGCI(category, ...) apk64_log_category((category), "INFO", __VA_ARGS__)
#define APK64_LOGCW(category, ...) apk64_log_category((category), "WARN", __VA_ARGS__)
#define APK64_LOGCE(category, ...) apk64_log_category((category), "ERROR", __VA_ARGS__)


/* input interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APK64_INPUT_MAX_TOUCHES 16
#define APK64_INPUT_MAX_SENSORS 2

enum {
    APK64_BTN_A       = UINT64_C(1) << 0,
    APK64_BTN_B       = UINT64_C(1) << 1,
    APK64_BTN_X       = UINT64_C(1) << 2,
    APK64_BTN_Y       = UINT64_C(1) << 3,
    APK64_BTN_LSTICK  = UINT64_C(1) << 4,
    APK64_BTN_RSTICK  = UINT64_C(1) << 5,
    APK64_BTN_L       = UINT64_C(1) << 6,
    APK64_BTN_R       = UINT64_C(1) << 7,
    APK64_BTN_ZL      = UINT64_C(1) << 8,
    APK64_BTN_ZR      = UINT64_C(1) << 9,
    APK64_BTN_PLUS    = UINT64_C(1) << 10,
    APK64_BTN_MINUS   = UINT64_C(1) << 11,
    APK64_BTN_DLEFT   = UINT64_C(1) << 12,
    APK64_BTN_DUP     = UINT64_C(1) << 13,
    APK64_BTN_DRIGHT  = UINT64_C(1) << 14,
    APK64_BTN_DDOWN   = UINT64_C(1) << 15,
    APK64_BTN_SL      = UINT64_C(1) << 16,
    APK64_BTN_SR      = UINT64_C(1) << 17,
};

typedef struct Apk64TouchPoint {
    uint32_t id;
    float x;
    float y;
    float diameter_x;
    float diameter_y;
    uint32_t attributes;
} Apk64TouchPoint;

typedef struct Apk64MotionSample {
    bool available;
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
} Apk64MotionSample;

typedef struct Apk64InputSnapshot {
    bool controller_connected;
    bool handheld;
    uint32_t style_set;
    uint64_t buttons;
    uint64_t buttons_down;
    uint64_t buttons_up;
    float left_x;
    float left_y;
    float right_x;
    float right_y;
    int touch_count;
    Apk64TouchPoint touches[APK64_INPUT_MAX_TOUCHES];
    int sensor_count;
    Apk64MotionSample sensors[APK64_INPUT_MAX_SENSORS];
} Apk64InputSnapshot;

typedef struct Apk64InputSessionStats {
    bool saw_controller;
    bool saw_button;
    bool saw_left_stick;
    bool saw_right_stick;
    bool saw_touch;
    bool saw_multitouch;
    bool saw_sensor;
    uint64_t frames;
    uint32_t max_touches;
} Apk64InputSessionStats;

bool apk64_input_init(void);
void apk64_input_update(Apk64InputSnapshot *out);
void apk64_input_get_stats(Apk64InputSessionStats *out);
void apk64_input_shutdown(void);


/* dialog interface */

#include <stddef.h>
#include <stdint.h>


/* Plain-text system presentation of Android modal content. Items remain in
 * guest hierarchy order; only explicitly actionable, enabled items activate. */
typedef struct Apk64DialogItem {
    const char *text;
    bool actionable;
    bool enabled;
} Apk64DialogItem;
typedef struct Apk64DialogSheet {
    const void *identity;
    const char *title;
    const Apk64DialogItem *items;
    size_t count;
} Apk64DialogSheet;
/* -1: no action, -2: back, otherwise the activated item index. */
int apk64_dialog_present(const Apk64DialogSheet *, const Apk64InputSnapshot *);
void apk64_dialog_close(void);


/* directory_fd interface */


/* Open a directory through the host descriptor backend. -2 asks the caller to
 * use its normal open path (not a directory, or native POSIX already handles it).
 * -1 is a real failure; >=0 is an owned, closeable descriptor. */
int apk64_directory_open(const char *host_path, int host_flags);


/* display interface */


#include <stdbool.h>
#include <stdint.h>

/* Android ActivityInfo screenOrientation values. Keep these independent of
 * Android headers so the Switch build remains self-contained. */
enum {
    APK64_SCREEN_ORIENTATION_UNSPECIFIED      = -1,
    APK64_SCREEN_ORIENTATION_LANDSCAPE        = 0,
    APK64_SCREEN_ORIENTATION_PORTRAIT         = 1,
    APK64_SCREEN_ORIENTATION_USER             = 2,
    APK64_SCREEN_ORIENTATION_BEHIND           = 3,
    APK64_SCREEN_ORIENTATION_SENSOR           = 4,
    APK64_SCREEN_ORIENTATION_NOSENSOR         = 5,
    APK64_SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6,
    APK64_SCREEN_ORIENTATION_SENSOR_PORTRAIT  = 7,
    APK64_SCREEN_ORIENTATION_REVERSE_LANDSCAPE= 8,
    APK64_SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9,
    APK64_SCREEN_ORIENTATION_FULL_SENSOR      = 10,
    APK64_SCREEN_ORIENTATION_USER_LANDSCAPE   = 11,
    APK64_SCREEN_ORIENTATION_USER_PORTRAIT    = 12,
    APK64_SCREEN_ORIENTATION_FULL_USER        = 13,
    APK64_SCREEN_ORIENTATION_LOCKED           = 14,
};

typedef enum Apk64DisplayAspect {
    APK64_DISPLAY_LANDSCAPE = 0,
    APK64_DISPLAY_PORTRAIT = 1,
} Apk64DisplayAspect;

typedef enum Apk64DisplayRotation {
    APK64_DISPLAY_ROTATION_0 = 0,
    APK64_DISPLAY_ROTATION_CW_90 = 1,
    APK64_DISPLAY_ROTATION_CCW_90 = 3,
} Apk64DisplayRotation;

typedef struct Apk64DisplayPlan {
    int32_t screen_orientation;
    bool orientation_declared;
    Apk64DisplayAspect aspect;
    bool reverse;
    Apk64DisplayRotation rotation;

    int32_t physical_width;
    int32_t physical_height;
    int32_t logical_width;
    int32_t logical_height;

    /* Content rectangle in physical touchscreen/display coordinates.
     * Rotated portrait fills the complete 1280x720 panel; landscape does too. */
    float content_x;
    float content_y;
    float content_width;
    float content_height;
} Apk64DisplayPlan;

/* Resolve an Android activity orientation into the fixed Switch display. For
 * unconstrained/sensor modes APK64 uses the current physical Switch aspect,
 * which is landscape. Explicit portrait/landscape families are honored.
 * Portrait keeps a 720x1280 Android logical surface but is rotated onto the
 * complete 1280x720 Switch panel so the user can hold the console vertically. */
bool apk64_display_make_plan(int32_t screen_orientation,
                             bool orientation_declared,
                             int32_t physical_width,
                             int32_t physical_height,
                             Apk64DisplayPlan *out);

/* Android Display.getRotation()/Surface.ROTATION_* value represented by this
 * plan. This is the device/display rotation from the natural Switch landscape
 * orientation. Android reports the drawn graphics rotation, so it matches
 * the compositor quarter-turn placing the guest on the physical panel. */
int32_t apk64_display_android_rotation(const Apk64DisplayPlan *plan);

const char *apk64_display_orientation_name(int32_t screen_orientation,
                                           bool orientation_declared);

/* Return the effective orientation presented on the fixed physical display.
 * Explicit portrait/landscape families retain their Android manifest name.
 * Unspecified/user/sensor/locked families report AUTO LANDSCAPE/PORTRAIT so
 * launcher diagnostics show APK64's resolved display plan, not only the raw
 * manifest token. */
const char *apk64_display_resolved_orientation_name(int32_t screen_orientation,
                                                    bool orientation_declared,
                                                    int32_t physical_width,
                                                    int32_t physical_height);

/* Map a physical Switch touchscreen point into the Android logical window,
 * including the inverse 90-degree transform used by rotated portrait mode. */
bool apk64_display_map_touch(const Apk64DisplayPlan *plan,
                             float physical_x,
                             float physical_y,
                             float *out_x,
                             float *out_y);


/* epoll_fd interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

/* Android arm64 epoll_event has natural 8-byte alignment (unlike x86's packed ABI). */
typedef struct Apk64EpollEvent { uint32_t events, padding; uint64_t data; } Apk64EpollEvent;
typedef struct Apk64EpollPollfd { int fd; int16_t events, revents; } Apk64EpollPollfd;
typedef int (*Apk64EpollPoll)(Apk64EpollPollfd *, uint32_t, int);
int apk64_epoll_create(int flags);
int apk64_epoll_ctl(int epfd, int op, int fd, const Apk64EpollEvent *event);
int apk64_epoll_wait(int epfd, Apk64EpollEvent *events, int maximum, int timeout_ms, Apk64EpollPoll poller);
/* -1 means not owned for readiness; -2 for control/dup/stat. */
int apk64_epoll_poll(int fd, short events, Apk64EpollPoll poller);
int apk64_epoll_control(int fd, int command, int argument);
int apk64_epoll_duplicate(int fd, int minimum, bool cloexec);
int apk64_epoll_stat(int fd, struct stat *out);
void apk64_epoll_closed(int fd);
void apk64_epoll_duplicated(int oldfd, int newfd);
void apk64_epoll_reset(void);


/* event_fd interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

int apk64_eventfd_create(unsigned initial, int android_flags);
/* -1 from poll means not owned; readiness uses host poll bit units. */
int apk64_eventfd_poll(int fd, short events);
ssize_t apk64_eventfd_read(int fd, void *buffer, size_t count);
ssize_t apk64_eventfd_write(int fd, const void *buffer, size_t count);
int apk64_eventfd_close(int fd);
/* Control/dup/stat return -2 for an unowned descriptor. */
int apk64_eventfd_control(int fd, int android_command, int argument);
int apk64_eventfd_duplicate(int fd, int minimum, bool cloexec);
int apk64_eventfd_stat(int fd, struct stat *out);
void apk64_eventfd_reset(void);
typedef struct Apk64TimerSpec {
    int64_t interval_sec, interval_nsec, value_sec, value_nsec;
} Apk64TimerSpec;
int apk64_timerfd_create(int android_clock,int android_flags);
int apk64_timerfd_settime(int fd,int flags,const Apk64TimerSpec *value,Apk64TimerSpec *old);
int apk64_timerfd_gettime(int fd,Apk64TimerSpec *value);


/* fd_compat interface */


#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Small process-local descriptor layer used for Android/Bionic facilities that
 * Horizon/newlib does not provide natively (notably pipe()). Virtual fds live
 * in a reserved high range and never collide with normal newlib descriptors. */
int apk64_fd_pipe(int fds[2]);
int apk64_fd_open_urandom(void);
void apk64_fd_random_fill(void *buffer, size_t count);
bool apk64_fd_is_virtual(int fd);
bool apk64_fd_is_random(int fd);
bool apk64_fd_is_pipe(int fd);
ssize_t apk64_fd_read(int fd, void *buffer, size_t count);
ssize_t apk64_fd_write(int fd, const void *buffer, size_t count);
int apk64_fd_close(int fd);
short apk64_fd_poll_revents(int fd, short requested_events);
void apk64_fd_reset(void);

#ifdef __cplusplus
}
#endif


/* file_contract interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct stat;

#ifdef __cplusplus
extern "C" {
#endif

/* Tracked-file provenance, descriptor flags and fsdev duplicate backend. */
void apk64_file_contract_reset(void);
void apk64_file_contract_open(int fd, const char *guest_path, const char *host_path,
                              int android_flags, int host_flags, bool package_apk);
void apk64_file_contract_dup(int oldfd, int newfd);
/* libnx/newlib fsdev does not expose a native duplicate operation for regular
 * files. Re-open a tracked path-backed descriptor at the same current offset.
 * Returns >=0 on success, -1 for a tracked descriptor whose duplicate failed,
 * and -2 when the descriptor is not tracked by this module. */
int apk64_file_contract_reopen_duplicate(int oldfd, int minimum, bool cloexec);
/* Native dup on POSIX, tracked-file reopen on fsdev, with independent flags. */
int apk64_file_contract_duplicate(int oldfd, int minimum, bool cloexec);
/* Native FD_* flag units. -2 means untracked; -1 is an actual failure. */
int apk64_file_contract_descriptor_flags(int fd, bool set, int flags);
/* Native F_GETFL units. -2 means untracked; -1 is an actual failure. */
int apk64_file_contract_status_flags(int fd);
/* POSIX error-number result; preserves errno. Caller serializes regular-file
 * offset operations. Switch supports tracked SD fsdev files (no sparse holes). */
int apk64_file_contract_allocate(int fd, int64_t offset, int64_t length);
/* Register a live socket's per-descriptor flags; no guest exec backend exists. */
int apk64_file_contract_socket_open(int fd, bool cloexec);
void apk64_file_contract_read(int fd, size_t requested, int64_t result);
/* pread-style read at an explicit offset (the descriptor offset is unchanged). */
void apk64_file_contract_read_at(int fd, int64_t offset, int64_t result);
void apk64_file_contract_write(int fd, size_t requested, int64_t result);
void apk64_file_contract_ftruncate(int fd, int64_t requested_size, int result, int error_code);
void apk64_file_contract_fstat(int fd, int result, int64_t size, uint32_t mode, int error_code);
void apk64_file_contract_mmap(int fd, size_t length, int64_t offset,
                              const void *result, int error_code);
void apk64_file_contract_close(int fd);
/* Copy the host path a tracked descriptor was opened with; false if untracked. */
bool apk64_file_contract_host_path(int fd, char *out, size_t cap);
/* A tracked open descriptor for host_path, or -1. */
int apk64_file_contract_fd_for_host_path(const char *host_path);
/* Metadata from a tracked open file, pinning its descriptor against close. */
int apk64_file_contract_stat_open(const char *host_path, struct stat *st);
/* Resize through a tracked writable regular-file descriptor while pinning its
 * registry entry against close. -2 means no suitable descriptor is tracked. */
int apk64_file_contract_truncate_open(const char *host_path, int64_t length);

#ifdef __cplusplus
}
#endif


/* file_pin interface */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct Apk64FilePin Apk64FilePin;
/* Called with the tracked descriptor registry locked, before guest close. */
Apk64FilePin *apk64_file_pin_create(int fd, const char *path, int flags) __attribute__((weak));
Apk64FilePin *apk64_file_contract_pin(int fd);
void apk64_file_pin_close(Apk64FilePin *);
ssize_t apk64_file_pin_read(Apk64FilePin *, void *, size_t, int64_t);
int apk64_file_pin_write(Apk64FilePin *, const void *, size_t, int64_t);
int apk64_file_pin_stat(Apk64FilePin *, struct stat *);
int apk64_file_pin_sync(Apk64FilePin *);
/* Independent descriptor offset; retains the same underlying file object. */
int apk64_file_pin_open(Apk64FilePin *, int flags);


/* framework_contracts interface */
#ifndef APK64_FRAMEWORK_CONTRACTS_H
#define APK64_FRAMEWORK_CONTRACTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APK64_CONTEXT_MODE_APPEND 0x00008000

enum {
    APK64_FO_ACCESS        = 0x00000001,
    APK64_FO_MODIFY        = 0x00000002,
    APK64_FO_ATTRIB        = 0x00000004,
    APK64_FO_CLOSE_WRITE   = 0x00000008,
    APK64_FO_CLOSE_NOWRITE = 0x00000010,
    APK64_FO_OPEN          = 0x00000020,
    APK64_FO_MOVED_FROM    = 0x00000040,
    APK64_FO_MOVED_TO      = 0x00000080,
    APK64_FO_CREATE        = 0x00000100,
    APK64_FO_DELETE        = 0x00000200,
    APK64_FO_DELETE_SELF   = 0x00000400,
    APK64_FO_MOVE_SELF     = 0x00000800,
    APK64_FO_ALL_EVENTS    = 0x00000fff,
};

typedef struct Apk64FileObserverSnapshot {
    bool exists;
    uint64_t inode;
    int64_t size;
    int64_t mtime;
    uint32_t mode;
} Apk64FileObserverSnapshot;

#define APK64_FO_NAME_CAP 256

typedef struct Apk64FileObserverEntry {
    char name[APK64_FO_NAME_CAP];
    uint64_t inode;
    int64_t size;
    int64_t mtime;
    int64_t ctime;
    uint32_t mode;
    bool is_directory;
} Apk64FileObserverEntry;

typedef struct Apk64FileObserverDirectorySnapshot {
    Apk64FileObserverEntry *entries;
    size_t count;
} Apk64FileObserverDirectorySnapshot;

typedef struct Apk64FileObserverEvent {
    int event;
    bool has_path;
    char path[APK64_FO_NAME_CAP];
} Apk64FileObserverEvent;

typedef enum Apk64EnumMapKeyPolicy {
    APK64_ENUM_MAP_KEY_OK = 0,
    APK64_ENUM_MAP_KEY_NULL,
    APK64_ENUM_MAP_KEY_WRONG_TYPE,
    APK64_ENUM_MAP_KEY_QUERY_MISS,
} Apk64EnumMapKeyPolicy;

/* Android ContextImpl.makeFilename() rejects File.separatorChar ('/' on
 * Android) but does not treat backslash as a path separator. */
bool apk64_context_stream_path(const char *files_dir, const char *name,
                               char *out, size_t cap);
const char *apk64_context_output_fopen_mode(int32_t mode);

/* Generic polling backend primitives shared with FileObserver host tests. */
bool apk64_file_observer_path_exists(const char *path);
int apk64_file_observer_pick_event(int mask,
                                   const Apk64FileObserverSnapshot *before,
                                   const Apk64FileObserverSnapshot *after);

bool apk64_file_observer_immediate_child(const char *directory, const char *path,
                                         char *out_name, size_t out_cap);
bool apk64_file_observer_scan_directory(const char *path,
                                        Apk64FileObserverDirectorySnapshot *out);
void apk64_file_observer_release_directory(Apk64FileObserverDirectorySnapshot *snapshot);
size_t apk64_file_observer_diff_directory(
    int mask,
    const Apk64FileObserverDirectorySnapshot *before,
    const Apk64FileObserverDirectorySnapshot *after,
    Apk64FileObserverEvent *out_events,
    size_t event_capacity);

/* Runtime bridge: filesystem code queues events here; JNI callbacks always run
 * on FileObserver's worker thread. */
void apk64_android_file_observer_notify_event(const char *host_path, int event);
void apk64_android_file_observer_notify_move(const char *old_host_path,
                                              const char *new_host_path);

/* EnumMap insertion rejects null/wrong-type keys; lookup/removal of such keys
 * simply miss. The runtime supplies key_matches_type using JNI instanceof. */
Apk64EnumMapKeyPolicy apk64_enum_map_key_policy(bool inserting,
                                                 bool key_nonnull,
                                                 bool key_matches_type);

#endif


/* future_diag interface */


#include <stdint.h>

struct Apk64Module;

/* Observe the Firebase SWIG future ABI without changing completion/results. */
uintptr_t apk64_future_diag_bind(const struct Apk64Module *provider,
                                const char *name, uintptr_t address);
/* Guest callers must be quiescent before clearing provider/callback pointers. */
void apk64_future_diag_reset(void);


/* graphics interface */


#include <stddef.h>
#include <stdint.h>



/* Stage 9 graphics compatibility layer.
 * EGL calls are wrapped where native-window translation is required.
 * GLES calls are ABI-compatible AArch64 C entry points and are resolved from
 * Switch Mesa via eglGetProcAddress at guest relocation time.
 */
Apk64Result apk64_graphics_resolve(const char *name,
                                   uintptr_t *out_address,
                                   const char **out_provider);
size_t apk64_graphics_export_count(void);
void apk64_graphics_reset(void);
uint64_t apk64_graphics_present_count(void);
/* Software Surface/Canvas frames, in premultiplied RGBA byte order. */
bool apk64_graphics_present_rgba(const uint8_t *pixels, int32_t width, int32_t height, uint32_t stride);
void apk64_graphics_release_software_surface(void);
bool apk64_graphics_presentation_plan(Apk64DisplayPlan *out);

/* Configure the host presentation target for an Android logical display plan.
 * Both orientations keep libnx's hardware-proven default NWindow. Portrait
 * virtualizes Android's logical default framebuffer and composites it into the
 * single physical Switch window with aspect-preserving pillarboxing. */
bool apk64_graphics_configure_presentation(const Apk64DisplayPlan *plan);

/* Guest-facing EGL wrappers. Keep these signatures ABI-simple and independent
 * of the host EGL typedef headers. */
void *apk64_eglGetDisplay(void *display_id);
uint32_t apk64_eglInitialize(void *display, int32_t *major, int32_t *minor);
uint32_t apk64_eglTerminate(void *display);
const char *apk64_eglQueryString(void *display, int32_t name);
uint32_t apk64_eglGetConfigs(void *display, void **configs, int32_t config_size, int32_t *num_config);
uint32_t apk64_eglChooseConfig(void *display, const int32_t *attribs, void **configs, int32_t config_size, int32_t *num_config);
uint32_t apk64_eglGetConfigAttrib(void *display, void *config, int32_t attribute, int32_t *value);
void *apk64_eglCreateWindowSurface(void *display, void *config, void *guest_native_window, const int32_t *attribs);
void *apk64_eglCreatePbufferSurface(void *display, void *config, const int32_t *attribs);
void *apk64_eglCreatePixmapSurface(void *display, void *config, void *pixmap, const int32_t *attribs);
uint32_t apk64_eglDestroySurface(void *display, void *surface);
uint32_t apk64_eglQuerySurface(void *display, void *surface, int32_t attribute, int32_t *value);
uint32_t apk64_eglBindAPI(uint32_t api);
uint32_t apk64_eglQueryAPI(void);
uint32_t apk64_eglWaitClient(void);
uint32_t apk64_eglReleaseThread(void);
uint32_t apk64_eglSurfaceAttrib(void *display, void *surface, int32_t attribute, int32_t value);
uint32_t apk64_eglBindTexImage(void *display, void *surface, int32_t buffer);
uint32_t apk64_eglReleaseTexImage(void *display, void *surface, int32_t buffer);
uint32_t apk64_eglSwapInterval(void *display, int32_t interval);
void *apk64_eglCreateContext(void *display, void *config, void *share_context, const int32_t *attribs);
uint32_t apk64_eglDestroyContext(void *display, void *context);
uint32_t apk64_eglMakeCurrent(void *display, void *draw, void *read, void *context);
void *apk64_eglGetCurrentContext(void);
void *apk64_eglGetCurrentSurface(int32_t readdraw);
void *apk64_eglGetCurrentDisplay(void);
uint32_t apk64_eglQueryContext(void *display, void *context, int32_t attribute, int32_t *value);
uint32_t apk64_eglWaitGL(void);
uint32_t apk64_eglWaitNative(int32_t engine);
uint32_t apk64_eglSwapBuffers(void *display, void *surface);
uint32_t apk64_eglCopyBuffers(void *display, void *surface, void *target);
int32_t apk64_eglGetError(void);
void *apk64_eglGetProcAddress(const char *name);
uint32_t apk64_eglPresentationTimeANDROID(void *display, void *surface, int64_t time_ns);


/* heap_cache interface */
#ifndef APK64_HEAP_CACHE_H
#define APK64_HEAP_CACHE_H

/* Per-thread small-block cache in front of the host C library allocator.
 * newlib serializes every malloc/free behind one process-wide lock; runtime
 * and guest threads allocating concurrently then spend most of their time
 * handing that lock to each other. Weak so builds without the cache link. */
void apk64_heap_cache_enable(void) __attribute__((weak));
/* Returns the calling thread's cached blocks to the shared heap. Called by
 * thread exit paths; later frees on the thread bypass the cache. */
void apk64_heap_cache_thread_exit(void) __attribute__((weak));

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Diagnostic-only bounded copy from a permanent small-block arena. Page-sized
 * classes are excluded because their aligned blocks may back guest aliases. */
bool apk64_heap_cache_read_small_block(const void *block, size_t offset, void *out, size_t bytes)
    __attribute__((weak));

/* One request size (bucketed to 4 KiB above the cache) that went to the host
 * allocator since the last call, with its count. */
typedef struct Apk64HeapCacheStat {
    const char *kind; /* malloc, calloc, memalign, realloc, free */
    size_t size;
    uint32_t count;
} Apk64HeapCacheStat;

/* Drains the counters; returns up to cap entries with the highest counts. */
size_t apk64_heap_cache_take_stats(Apk64HeapCacheStat *out, size_t cap) __attribute__((weak));

#endif


/* host_name interface */


#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Android's ext4/f2fs accept every byte except '/' and NUL in a file name.
 * The Switch SD card is FAT, which rejects control characters and
 * " * : < > ? \ |. Guest names store each such character c as the private-use
 * code point U+F000+c (UTF-8 EF 80|c>>6 80|c&3F), the reversible convention
 * Cygwin and WSL use for the same FAT/NTFS restriction. Directory listings
 * decode it back, so guests always see their original names. */

static inline bool apk64_host_name_reserved(unsigned char c) {
    return (c != 0 && c < 0x20u) || (c != 0 && strchr("\"*:<>?\\|", c) != NULL);
}

/* Encode a guest relative path ('/' separators kept). False if it does not fit. */
static inline bool apk64_host_name_encode(const char *guest, char *out, size_t cap) {
    if (!guest || !out || cap == 0) return false;
    size_t w = 0;
    for (const unsigned char *s = (const unsigned char *)guest; *s; ++s) {
        if (apk64_host_name_reserved(*s)) {
            if (w + 3u >= cap) return false;
            out[w++] = (char)0xEF;
            out[w++] = (char)(0x80u | (*s >> 6));
            out[w++] = (char)(0x80u | (*s & 0x3Fu));
        } else {
            if (w + 1u >= cap) return false;
            out[w++] = (char)*s;
        }
    }
    out[w] = '\0';
    return true;
}

/* Decode a host directory entry name back to the guest name. */
static inline void apk64_host_name_decode(const char *host, char *out, size_t cap) {
    if (!out || cap == 0) return;
    size_t w = 0;
    for (const unsigned char *s = (const unsigned char *)(host ? host : ""); *s && w + 1u < cap;) {
        if (s[0] == 0xEFu && (s[1] & 0xFEu) == 0x80u && (s[2] & 0xC0u) == 0x80u) {
            const unsigned char c = (unsigned char)(((s[1] & 1u) << 6) | (s[2] & 0x3Fu));
            if (apk64_host_name_reserved(c)) { out[w++] = (char)c; s += 3; continue; }
        }
        out[w++] = (char)*s++;
    }
    out[w] = '\0';
}


/* ifunc interface */


#include <stdint.h>

/* Invoke one Android guest GNU IFUNC/IRELATIVE resolver using the ABI exposed
 * by the configured Android runtime profile. */
uintptr_t apk64_call_android_ifunc_resolver(uintptr_t resolver_address);


/* il2cpp_diag interface */


#include <stdint.h>

struct Apk64Module;

/* Observational wrappers for the documented IL2CPP invocation API. */
uintptr_t apk64_il2cpp_diag_bind(const struct Apk64Module *provider,
                                const char *name, uintptr_t address);
/* Guest callers must be quiescent, as for freeing their linker mappings. */
void apk64_il2cpp_diag_reset(void);


/* inline_syscall interface */


/* Version 1: Linux AArch64 SVC #0 -> APK64 resumable BRK bridge. */
#define APK64_LINUX_SVC_INSN 0xd4000001u
#define APK64_LINUX_SYSCALL_BRK 0xd43e2000u /* BRK #0xf100 */
#define APK64_LINUX_SYSCALL_ESR 0xf100u



/* io_timing interface */
#ifndef APK64_IO_TIMING_H
#define APK64_IO_TIMING_H
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
void apk64_log_category(const char *,const char *,const char *,...) __attribute__((weak));
/* Bounded operation timing only. Preserve guest-visible errno on every exit. */
typedef struct { uint64_t calls,total_us,max_us,reported_us; unsigned slow_reports,total_reports; } Apk64IoTotals;
typedef struct { uint64_t begin; const char *operation; uintptr_t caller; Apk64IoTotals *totals; const char *path; } Apk64IoTiming;
static inline uint64_t apk64_io_time_us(void) {
    const int saved=errno;struct timespec ts={0};clock_gettime(CLOCK_MONOTONIC,&ts);errno=saved;
    return (uint64_t)ts.tv_sec*1000000+(uint64_t)ts.tv_nsec/1000;
}
static inline void apk64_io_timing_end_at(Apk64IoTiming *scope,uint64_t now) {
    const int saved=errno;
    if(now<scope->begin){errno=saved;return;}
    const uint64_t elapsed=now-scope->begin;
    Apk64IoTotals *t=scope->totals;
    const uint64_t calls=__atomic_add_fetch(&t->calls,1,__ATOMIC_RELAXED);
    const uint64_t total=__atomic_add_fetch(&t->total_us,elapsed,__ATOMIC_RELAXED);
    uint64_t maximum=__atomic_load_n(&t->max_us,__ATOMIC_RELAXED);
    while(maximum<elapsed&&!__atomic_compare_exchange_n(&t->max_us,&maximum,elapsed,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED)){}
    uint64_t reported=__atomic_load_n(&t->reported_us,__ATOMIC_RELAXED);
    if(total>=reported && total-reported>=1000000 && apk64_log_category &&
       __atomic_compare_exchange_n(&t->reported_us,&reported,total,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED) &&
       __atomic_fetch_add(&t->total_reports,1u,__ATOMIC_RELAXED)<16)
        apk64_log_category("PERF","INFO","IO totals operation=%s calls=%llu totalUs=%llu maxUs=%llu",scope->operation,(unsigned long long)calls,(unsigned long long)total,(unsigned long long)__atomic_load_n(&t->max_us,__ATOMIC_RELAXED));
    if(now>=scope->begin && now-scope->begin>=50000 && apk64_log_category &&
       __atomic_fetch_add(&t->slow_reports,1u,__ATOMIC_RELAXED)<16)
        apk64_log_category("PERF","INFO","slow IO operation=%s elapsedUs=%llu caller=0x%llx",scope->operation,(unsigned long long)(now-scope->begin),(unsigned long long)scope->caller);
    /* Sample short metadata stalls too, without flooding steady-state I/O. */
    if (elapsed >= 2000 && scope->path && apk64_log_category &&
        (calls <= 8 || ((calls & (calls - 1)) == 0)))
        apk64_log_category("PERF","INFO","IO path operation=%s calls=%llu elapsedUs=%llu caller=0x%llx path=%.512s",
                           scope->operation,(unsigned long long)calls,(unsigned long long)elapsed,
                           (unsigned long long)scope->caller,scope->path);
    errno=saved;
}
static inline void apk64_io_timing_end(Apk64IoTiming *scope){apk64_io_timing_end_at(scope,apk64_io_time_us());}
#define APK64_IO_TIMING_PATH(path) \
    static Apk64IoTotals apk64_io_totals_; \
    Apk64IoTiming apk64_io_scope_ __attribute__((cleanup(apk64_io_timing_end))) = \
        {apk64_io_time_us(),__func__,(uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),&apk64_io_totals_,(path)}
#define APK64_IO_TIMING() APK64_IO_TIMING_PATH(NULL)
#endif


/* java_annotations interface */
#ifndef APK64_JAVA_ANNOTATIONS_H
#define APK64_JAVA_ANNOTATIONS_H


#ifdef __cplusplus
extern "C" {
#endif

/* Java annotation reflection over DEX annotations_directory_item metadata
 * (Class/Method/Constructor/Field getAnnotation*, isAnnotationPresent,
 * Method.getParameterAnnotations, Class.isAnnotation). SYSTEM metadata also
 * supplies member-class enumeration, nesting and reflective class modifiers.
 *
 * Only RUNTIME-visible annotations are reflected (JLS 9.6.4.2); BUILD and
 * SYSTEM annotations are not. Instances are java.lang.reflect.Proxy objects
 * implementing the annotation interface, with element values decoded from
 * the encoded_annotation and dalvik.annotation.AnnotationDefault defaults,
 * following java.lang.annotation.Annotation equals/hashCode/toString. */
Apk64Result apk64_java_annotations_register_members(void);
void apk64_java_annotations_reset(void);

#ifdef __cplusplus
}
#endif

#endif


/* java_crypto interface */

#include <stdint.h>

/* javax.crypto Cipher (AES ECB/CBC/CTR), SecretKeySpec and IvParameterSpec. */
Apk64Result apk64_java_crypto_register(void);
void apk64_java_crypto_reset(void);

/* JNI boot natives used by the AOSP Mac overlay's streaming HMAC provider. */
void apk64_java_hmac_create(void *env, void *self, void *digest);
int32_t apk64_java_hmac_length(void *env, void *self);
void apk64_java_hmac_init(void *env, void *self, void *key, void *params);
void apk64_java_hmac_update_byte(void *env, void *self, int8_t byte);
void apk64_java_hmac_update_array(void *env, void *self, void *array, int32_t offset, int32_t length);
void *apk64_java_hmac_final(void *env, void *self);
void apk64_java_hmac_reset(void *env, void *self);
void *apk64_java_hmac_clone(void *env, void *self);
void *apk64_java_pbkdf2_derive(void *env, void *self, void *password, void *salt,
                              int32_t iterations, int32_t bits, void *digest);


/* java_egl interface */


/* javax.microedition.khronos.egl EGL10/EGL11 Java binding (EGLContext.getEGL)
 * and the GL10 facade returned by EGLContext.getGL(). */
Apk64Result apk64_java_egl_register(void);
void apk64_java_egl_reset(void);


/* java_http interface */
#ifndef APK64_JAVA_HTTP_H
#define APK64_JAVA_HTTP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct Apk64HttpHeader {
    char *key, *value;
    struct Apk64HttpHeader *next;
} Apk64HttpHeader;
typedef struct Apk64HttpTransfer Apk64HttpTransfer;
typedef struct Apk64HttpRequest {
    const char *url, *method;
    const Apk64HttpHeader *headers;
    const uint8_t *body;
    size_t body_size;
    int32_t connect_timeout, read_timeout;
    bool has_body, strip_sensitive;
    bool chunked;
} Apk64HttpRequest;
Apk64HttpTransfer *apk64_http_create(const Apk64HttpRequest *request);
bool apk64_http_response(Apk64HttpTransfer *transfer);
bool apk64_http_connect_probe(Apk64HttpTransfer *transfer);
int32_t apk64_http_read(Apk64HttpTransfer *transfer, uint8_t *out, size_t size);
void apk64_http_cancel(Apk64HttpTransfer *transfer);
void apk64_http_destroy(Apk64HttpTransfer *transfer);
int apk64_http_status(const Apk64HttpTransfer *transfer);
const Apk64HttpHeader *apk64_http_headers(const Apk64HttpTransfer *transfer);
const char *apk64_http_header(const Apk64HttpTransfer *transfer, const char *key);
const char *apk64_http_exception(const Apk64HttpTransfer *transfer);
const char *apk64_http_error(const Apk64HttpTransfer *transfer);
char *apk64_http_redirect_url(const Apk64HttpTransfer *transfer);
bool apk64_http_same_origin(const char *left, const char *right);
#endif


/* java_lang interface */
#ifndef APK64_JAVA_LANG_H
#define APK64_JAVA_LANG_H

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Character.isWhitespace, shared by String stripping and Properties parsing. */
static inline bool apk64_java_character_is_whitespace(uint16_t c) {
    return c == ' ' || (c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x1F) || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x2006) || (c >= 0x2008 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
           c == 0x205F || c == 0x3000;
}

/* Core java.lang model: String/StringBuilder/StringBuffer text operations on
 * exact UTF-16 data, CharSequence/Appendable, Object.wait/clone (java_lang.c)
 * and the Character/Integer/Long/Float/Double/Number/Math statics
 * (java_lang_numbers.c). Registered by the Android framework model before its
 * own members, so these definitions take precedence. */
Apk64Result apk64_java_lang_register_classes(void);
Apk64Result apk64_java_lang_register_members(void);
Apk64Result apk64_java_lang_numbers_register_members(void);
void apk64_java_lang_reset(void);
/* Canonical (interned) String for a modified-UTF-8 literal; local reference. */
void *apk64_java_lang_intern_literal(void *env, const char *mutf8);

/* malloc-owned UTF-16 contents of any CharSequence (String, StringBuilder,
 * StringBuffer directly; anything else through its virtual toString()).
 * NULL with NullPointerException pending for null. */
uint16_t *apk64_java_charseq_utf16(void *env, void *ref, int32_t *out_length);

#ifdef __cplusplus
}
#endif

#endif


/* java_net interface */
#ifndef APK64_JAVA_NET_H
#define APK64_JAVA_NET_H

#include <stdbool.h>
Apk64Result apk64_java_net_register(void);
void apk64_java_net_reset(void);
/* Whether a native TLS factory uses the platform root store, so the HTTP
 * backend can reject custom trust configuration rather than ignore it. */
bool apk64_java_net_factory_platform_trust(void *factory_ref);
#endif


/* java_regex interface */
#ifndef APK64_JAVA_REGEX_H
#define APK64_JAVA_REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Clean-room java.util.regex engine: Java pattern syntax and leftmost-first
 * backtracking semantics over UTF-16 text, matching by code point.
 * POSIX regcomp (Bionic) is leftmost-longest and byte oriented, so it cannot
 * stand in for Pattern (lazy/possessive quantifiers, lookaround, \Q..\E,
 * inline flags, named groups, backreferences). */

/* java.util.regex.Pattern flag values. */
enum {
    APK64_JREGEX_UNIX_LINES = 0x01,
    APK64_JREGEX_CASE_INSENSITIVE = 0x02,
    APK64_JREGEX_COMMENTS = 0x04,
    APK64_JREGEX_MULTILINE = 0x08,
    APK64_JREGEX_LITERAL = 0x10,
    APK64_JREGEX_DOTALL = 0x20,
    APK64_JREGEX_UNICODE_CASE = 0x40,
    APK64_JREGEX_CANON_EQ = 0x80,
    APK64_JREGEX_UNICODE_CHARACTER_CLASS = 0x100,
};

typedef enum Apk64JavaRegexMode {
    APK64_JREGEX_FIND,        /* Matcher.find: first match at or after from */
    APK64_JREGEX_MATCHES,     /* Matcher.matches: the whole region */
    APK64_JREGEX_LOOKING_AT,  /* Matcher.lookingAt: a prefix of the region */
} Apk64JavaRegexMode;

typedef struct Apk64JavaRegex Apk64JavaRegex;

/* Compile; NULL on a syntax error with Pattern's description in error and the
 * pattern index in *error_index (PatternSyntaxException). */
Apk64JavaRegex *apk64_java_regex_compile(const uint16_t *pattern, size_t length, int flags,
                                         char *error, size_t error_cap, int32_t *error_index);
void apk64_java_regex_free(Apk64JavaRegex *regex);
/* Capturing groups, not counting group 0 (Matcher.groupCount). */
size_t apk64_java_regex_group_count(const Apk64JavaRegex *regex);
/* Group number of a named group, or -1. */
int32_t apk64_java_regex_group_index(const Apk64JavaRegex *regex, const char *name);

/* Match in input[region_start, region_end) starting at from. groups receives
 * 2 * (group_count + 1) offsets, -1 for a group that did not participate.
 * last_append_end is the previous match end for \G. Returns 1 on a match,
 * 0 for none, -1 when the backtracking depth limit is exceeded (Android
 * reports StackOverflowError for such patterns). */
int apk64_java_regex_match(const Apk64JavaRegex *regex, const uint16_t *input, size_t region_start,
                           size_t region_end, size_t from, size_t last_match_end,
                           Apk64JavaRegexMode mode, int32_t *groups);

/* Matcher bounds affect anchors and assertions, never ordinary consumption. */
int apk64_java_regex_match_bounds(const Apk64JavaRegex *regex, const uint16_t *input, size_t input_length,
                                  size_t region_start, size_t region_end, size_t from, size_t last_match_end,
                                  bool anchoring, bool transparent, Apk64JavaRegexMode mode, int32_t *groups);

/* Framework bindings (java.util.regex.Pattern/Matcher and the regex methods of
 * java.lang.String), registered by the Android framework model. */
Apk64Result apk64_java_regex_register_classes(void);
Apk64Result apk64_java_regex_register_members(void);
void apk64_java_regex_reset(void);

#ifdef __cplusplus
}
#endif

#endif


/* java_security interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


Apk64Result apk64_java_security_register(void);
void apk64_java_security_reset(void);

/* Extract the exact DER Name TLV from an X.509 certificate. The caller owns
 * *out_der and must free it. This parser only identifies certificate structure;
 * it does not imply certificate trust or signature verification. */
Apk64Result apk64_java_security_x509_name_der(const uint8_t *certificate_der,
                                              size_t certificate_size,
                                              bool subject,
                                              uint8_t **out_der,
                                              size_t *out_size);

typedef struct Apk64JavaX509Metadata {
    int32_t version; /* Java X509Certificate numbering: v1=1, v2=2, v3=3. */
    const uint8_t *serial_value;
    size_t serial_value_size;
    int64_t not_before_ms;
    int64_t not_after_ms;
    char signature_oid[96];
    const uint8_t *signature_algorithm_params;
    size_t signature_algorithm_params_size;
    const uint8_t *signature_value;
    size_t signature_value_size;
    const uint8_t *tbs_certificate;
    size_t tbs_certificate_size;
    char public_key_oid[96];
    const uint8_t *subject_public_key_info;
    size_t subject_public_key_info_size;
} Apk64JavaX509Metadata;

/* Parse commonly consumed X509Certificate metadata. Pointer fields reference
 * the caller-owned certificate_der buffer and remain valid only while it does. */
Apk64Result apk64_java_security_x509_metadata(const uint8_t *certificate_der,
                                              size_t certificate_size,
                                              Apk64JavaX509Metadata *out_metadata);

typedef struct Apk64JavaX509Extension {
    bool critical;
    const uint8_t *encoded_value; /* exact DER OCTET STRING TLV */
    size_t encoded_value_size;
    const uint8_t *value;         /* OCTET STRING contents */
    size_t value_size;
} Apk64JavaX509Extension;

/* Extension helpers operate on the certificate DER and never imply trust. */
Apk64Result apk64_java_security_x509_extension(const uint8_t *certificate_der,
                                               size_t certificate_size,
                                               const char *oid,
                                               Apk64JavaX509Extension *out_extension,
                                               bool *out_found);
Apk64Result apk64_java_security_x509_extension_oids(const uint8_t *certificate_der,
                                                    size_t certificate_size,
                                                    bool critical,
                                                    char ***out_oids,
                                                    size_t *out_count);
void apk64_java_security_x509_free_oids(char **oids, size_t count);

typedef struct Apk64JavaX509GeneralName {
    int32_t type;
    char *text; /* owned UTF-8, or NULL for a DER-valued name */
    const uint8_t *der; /* borrowed exact GeneralName TLV */
    size_t der_size;
} Apk64JavaX509GeneralName;
Apk64Result apk64_java_security_x509_general_names(const uint8_t *der, size_t size,
    bool subject, Apk64JavaX509GeneralName **out_names, size_t *out_count);
void apk64_java_security_x509_free_names(Apk64JavaX509GeneralName *names, size_t count);
Apk64Result apk64_java_security_x509_key_usage(const uint8_t *certificate_der,
                                               size_t certificate_size,
                                               bool **out_usage,
                                               size_t *out_count,
                                               bool *out_present);
Apk64Result apk64_java_security_x509_basic_constraints(const uint8_t *certificate_der,
                                                       size_t certificate_size,
                                                       bool *out_ca,
                                                       int32_t *out_path_len,
                                                       bool *out_present);
Apk64Result apk64_java_security_x509_has_unsupported_critical_extension(
    const uint8_t *certificate_der, size_t certificate_size, bool *out_unsupported);


/* java_text interface */
#ifndef APK64_JAVA_TEXT_H
#define APK64_JAVA_TEXT_H

/* Host-side implementations of java.lang text contracts that are independent
 * of the JNI object model: java.util.Formatter conversion semantics and
 * String.getBytes(charsetName) encoding. The framework layer adapts Java
 * objects into these value descriptions. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum Apk64JavaFormatArgKind {
    APK64_JFMT_ARG_NULL = 0,
    APK64_JFMT_ARG_STRING,   /* java.lang.String; text is UTF-8 */
    APK64_JFMT_ARG_INTEGRAL, /* Byte/Short/Integer/Long; bits = 8/16/32/64 */
    APK64_JFMT_ARG_FLOATING, /* Float/Double */
    APK64_JFMT_ARG_BOOLEAN,
    APK64_JFMT_ARG_CHAR,     /* Character; code unit in integral */
    APK64_JFMT_ARG_OBJECT,   /* any other object; text is toString() */
    APK64_JFMT_ARG_BIG_INTEGER, /* java.math.BigInteger; signum in integral, big-endian magnitude */
} Apk64JavaFormatArgKind;

/* Resolve date/time arguments lazily, preserving Date.getTime() overrides and
 * avoiding Calendar normalization for unrelated conversions. */
typedef bool (*Apk64JavaFormatDateFn)(void *user, size_t index, char conversion,
                                     char *out, size_t cap);

typedef struct Apk64JavaFormatArg {
    Apk64JavaFormatArgKind kind;
    const char *text;
    const char *type_name; /* binary class name for exception messages */
    int64_t integral;
    unsigned bits;
    double floating;
    bool boolean;
    const uint8_t *magnitude; /* APK64_JFMT_ARG_BIG_INTEGER */
    size_t magnitude_len;
    Apk64JavaFormatDateFn date_time;
    void *date_user;
} Apk64JavaFormatArg;

/* Supplies the UTF-8 text of argument `index` on demand (Object.toString()
 * is only invoked for conversions that require it). Returns NULL on failure
 * (a pending Java exception, for example). */
typedef const char *(*Apk64JavaFormatTextFn)(void *user, size_t index);

typedef enum Apk64JavaFormatStatus {
    APK64_JFMT_OK = 0,
    APK64_JFMT_NO_MEMORY,
    APK64_JFMT_TEXT_FAILED,               /* text callback failed */
    APK64_JFMT_UNKNOWN_CONVERSION,        /* UnknownFormatConversionException */
    APK64_JFMT_MISSING_ARGUMENT,          /* MissingFormatArgumentException */
    APK64_JFMT_ILLEGAL_CONVERSION,        /* IllegalFormatConversionException */
    APK64_JFMT_MISSING_WIDTH,             /* MissingFormatWidthException */
    APK64_JFMT_ILLEGAL_PRECISION,         /* IllegalFormatPrecisionException */
    APK64_JFMT_FLAGS_CONVERSION_MISMATCH, /* FormatFlagsConversionMismatchException */
    APK64_JFMT_ILLEGAL_FLAGS,             /* IllegalFormatFlagsException */
    APK64_JFMT_UNSUPPORTED,               /* hash or hexadecimal floating conversions */
} Apk64JavaFormatStatus;

/* Formats per java.util.Formatter with Locale.ROOT symbols. On success
 * *out receives a malloc'd NUL-terminated UTF-8 string. On failure `detail`
 * receives the offending conversion/specifier text for the exception. */
Apk64JavaFormatStatus apk64_java_format(const char *format,
                                        const Apk64JavaFormatArg *args, size_t argc,
                                        Apk64JavaFormatTextFn text_fn, void *text_user,
                                        char **out, char *detail, size_t detail_cap);

/* Java exception class (JNI internal name) for a failure status. */
const char *apk64_java_format_exception_class(Apk64JavaFormatStatus status);

/* Double.toString / Float.toString text (is_float selects Float digits). */
void apk64_java_floating_to_string(double v, bool is_float, char *out, size_t cap);

/* Encodes UTF-16 code units per String.getBytes(charsetName). Unmappable or
 * malformed input is replaced as Java's default encoder replacement does.
 * Returns false with *out_supported=false for an unknown charset. */
bool apk64_java_encode_utf16(const uint16_t *chars, size_t length, const char *charset,
                             uint8_t **out_bytes, size_t *out_length, bool *out_supported);

/* Simple (1:1) Unicode case mapping of one code point, as Character.toLowerCase
 * / toUpperCase: ASCII, Latin-1, Latin Extended-A/Additional, Greek, Cyrillic,
 * Armenian and fullwidth Latin. Identical on every host (newlib's towlower is
 * not relied upon). */
uint32_t apk64_java_to_lower(uint32_t c);
uint32_t apk64_java_to_upper(uint32_t c);

/* Character.isLetter / isDigit over the major Unicode letter and decimal
 * digit blocks (same on every host). */
bool apk64_java_is_letter(uint32_t c);
bool apk64_java_is_digit(uint32_t c);
/* Decimal value of a Unicode digit, or -1. */
int apk64_java_digit_value(uint32_t c);

/* Host-independent core for java.util.StringTokenizer. The framework owns the
 * UTF-16 buffers and adapts Java objects around this state. A NULL delimiter
 * pointer represents Java's null delimiter value; an empty, non-NULL buffer is
 * a valid empty delimiter set. Positions and slices are UTF-16 code-unit
 * offsets, matching java.lang.String substring boundaries. */
typedef struct Apk64JavaStringTokenizerState {
    const uint16_t *text;
    size_t text_length;
    const uint16_t *delimiters;
    size_t delimiter_length;
    size_t position;
    bool return_delimiters;
} Apk64JavaStringTokenizerState;

typedef enum Apk64JavaStringTokenizerStatus {
    APK64_JST_OK = 0,
    APK64_JST_NO_SUCH_ELEMENT,
    APK64_JST_NULL_DELIMITERS,
} Apk64JavaStringTokenizerStatus;

Apk64JavaStringTokenizerStatus apk64_java_string_tokenizer_has_more(
    const Apk64JavaStringTokenizerState *state, bool *out_has_more);
Apk64JavaStringTokenizerStatus apk64_java_string_tokenizer_next(
    Apk64JavaStringTokenizerState *state, size_t *out_start, size_t *out_length);
Apk64JavaStringTokenizerStatus apk64_java_string_tokenizer_count(
    const Apk64JavaStringTokenizerState *state, size_t *out_count);

#endif


/* keyboard interface */

#include <stdbool.h>
#include <stddef.h>


/* Host UTF-8, not JNI modified UTF-8. The caller owns main-thread scheduling.
 * accepted is false on cancellation or failure; those never commit text. */
typedef struct Apk64KeyboardConfig {
    const char *initial_text;
    const char *action_label;
    bool password;
    bool numeric;
    bool decimal;
    bool signed_number;
    bool multiline;
} Apk64KeyboardConfig;
Apk64Result apk64_keyboard_show(const Apk64KeyboardConfig *config,char *out_text,
                                 size_t capacity,bool *accepted);


/* media interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APK64_MEDIA_OUTPUT_RATE 48000u
#define APK64_MEDIA_OUTPUT_CHANNELS 2u
#define APK64_MEDIA_ANDROIDFD_USE_FILE_SIZE INT64_C(-1)

typedef enum Apk64MediaState {
    APK64_MEDIA_STOPPED = 1,
    APK64_MEDIA_PAUSED = 2,
    APK64_MEDIA_PLAYING = 3,
} Apk64MediaState;

typedef enum Apk64MediaCodec {
    APK64_MEDIA_CODEC_UNKNOWN = 0,
    APK64_MEDIA_CODEC_MP3 = 1,
    APK64_MEDIA_CODEC_WAV = 2,
    APK64_MEDIA_CODEC_VORBIS = 3,
} Apk64MediaCodec;

typedef struct Apk64MediaPlayer Apk64MediaPlayer;
typedef struct Apk64MediaClip Apk64MediaClip;

typedef struct Apk64MediaStats {
    uint64_t players_created;
    uint64_t players_realized;
    uint64_t players_started;
    uint64_t players_finished;
    uint64_t mp3_frames_decoded;
    uint64_t wav_clips_decoded;
    uint64_t vorbis_clips_decoded;
    uint64_t source_bytes_read;
    uint64_t output_frames_mixed;
    uint64_t decode_starvations;
    uint32_t active_players;
} Apk64MediaStats;

bool apk64_media_init(void);
void apk64_media_reset(void);

Apk64MediaPlayer *apk64_media_player_create_android_fd(int fd,
                                                        int64_t offset,
                                                        int64_t length,
                                                        uint32_t container_type,
                                                        const char *mime_type);

/* Generic path/clip playback used by Android Java MediaPlayer/SoundPool. Path
 * decoding supports PCM WAV on every build and Ogg Vorbis when APK64 is linked
 * with the Switch libvorbis portlib. Clips are normalized to stereo signed-16
 * PCM at their source sample rate and resampled by APK64's existing mixer. */
Apk64MediaClip *apk64_media_clip_load_path(const char *path);
/* Decode a byte range of an open descriptor (an APK asset opened with
 * AssetManager.openFd, a ParcelFileDescriptor, ...). The container is
 * identified from its content (RIFF/WAVE, Ogg), not a file name. The
 * descriptor is only read during the call. */
Apk64MediaClip *apk64_media_clip_load_fd(int fd, int64_t offset, int64_t length);
void apk64_media_clip_retain(Apk64MediaClip *clip);
void apk64_media_clip_release(Apk64MediaClip *clip);
uint32_t apk64_media_clip_duration_ms(const Apk64MediaClip *clip);

Apk64MediaPlayer *apk64_media_player_create_clip(Apk64MediaClip *clip);
Apk64MediaPlayer *apk64_media_player_create_path(const char *path);
/* Player for a descriptor range, by content: WAV/Ogg decode to a clip; MPEG
 * audio (ID3 tag or frame sync) streams through the MP3 decoder, which keeps
 * reading fd, so the caller must keep fd open until the player is destroyed. */
Apk64MediaPlayer *apk64_media_player_create_fd(int fd, int64_t offset, int64_t length);

bool apk64_media_player_realize(Apk64MediaPlayer *player);
void apk64_media_player_destroy(Apk64MediaPlayer *player);

bool apk64_media_player_set_state(Apk64MediaPlayer *player, Apk64MediaState state);
Apk64MediaState apk64_media_player_get_state(Apk64MediaPlayer *player);
uint32_t apk64_media_player_get_duration_ms(Apk64MediaPlayer *player);
uint32_t apk64_media_player_get_position_ms(Apk64MediaPlayer *player);
bool apk64_media_player_seek_ms(Apk64MediaPlayer *player, uint32_t position_ms);
bool apk64_media_player_set_loop(Apk64MediaPlayer *player, bool enabled,
                                  uint32_t start_ms, uint32_t end_ms);
bool apk64_media_player_set_volume(Apk64MediaPlayer *player, float left, float right);
/* SoundPool playback pitch/speed, 0.5 through 2.0; invalid rates are rejected. */
bool apk64_media_player_set_rate(Apk64MediaPlayer *player, float rate);

/* Called by the Stage-11 hardware streamer. Produces exactly `frames` of
 * interleaved 48 kHz stereo signed-16 PCM unless frames is zero. */
size_t apk64_media_render_s16(int16_t *out_interleaved, size_t frames, void *unused);
void apk64_media_get_stats(Apk64MediaStats *out);

/* Test/support hooks. */
bool apk64_media_source_range_valid(int fd, int64_t offset, int64_t length,
                                    int64_t *resolved_length);


/* media_ndk interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct Apk64AMediaFormat Apk64AMediaFormat;

Apk64AMediaFormat *apk64_AMediaFormat_new(void);
int apk64_AMediaFormat_delete(Apk64AMediaFormat *format);
bool apk64_AMediaFormat_getInt32(Apk64AMediaFormat *format, const char *name, int32_t *out);
bool apk64_AMediaFormat_getInt64(Apk64AMediaFormat *format, const char *name, int64_t *out);
bool apk64_AMediaFormat_getFloat(Apk64AMediaFormat *format, const char *name, float *out);
bool apk64_AMediaFormat_getString(Apk64AMediaFormat *format, const char *name, const char **out);
void apk64_AMediaFormat_setInt32(Apk64AMediaFormat *format, const char *name, int32_t value);
void apk64_AMediaFormat_setInt64(Apk64AMediaFormat *format, const char *name, int64_t value);
void apk64_AMediaFormat_setFloat(Apk64AMediaFormat *format, const char *name, float value);
void apk64_AMediaFormat_setString(Apk64AMediaFormat *format, const char *name, const char *value);
bool apk64_AMediaFormat_getBuffer(Apk64AMediaFormat *format, const char *name, void **data, size_t *size);
void apk64_AMediaFormat_setBuffer(Apk64AMediaFormat *format, const char *name, const void *data, size_t size);
const char *apk64_AMediaFormat_toString(Apk64AMediaFormat *format);
extern const char *apk64_AMEDIAFORMAT_KEY_BIT_RATE;
extern const char *apk64_AMEDIAFORMAT_KEY_I_FRAME_INTERVAL;

extern const char *apk64_AMEDIAFORMAT_KEY_CHANNEL_COUNT;
extern const char *apk64_AMEDIAFORMAT_KEY_COLOR_FORMAT;
extern const char *apk64_AMEDIAFORMAT_KEY_COLOR_RANGE;
extern const char *apk64_AMEDIAFORMAT_KEY_COLOR_STANDARD;
extern const char *apk64_AMEDIAFORMAT_KEY_DURATION;
extern const char *apk64_AMEDIAFORMAT_KEY_ENCODER_DELAY;
extern const char *apk64_AMEDIAFORMAT_KEY_FRAME_RATE;
extern const char *apk64_AMEDIAFORMAT_KEY_HEIGHT;
extern const char *apk64_AMEDIAFORMAT_KEY_LANGUAGE;
extern const char *apk64_AMEDIAFORMAT_KEY_MIME;
extern const char *apk64_AMEDIAFORMAT_KEY_ROTATION;
extern const char *apk64_AMEDIAFORMAT_KEY_SAMPLE_RATE;
extern const char *apk64_AMEDIAFORMAT_KEY_SLICE_HEIGHT;
extern const char *apk64_AMEDIAFORMAT_KEY_STRIDE;
extern const char *apk64_AMEDIAFORMAT_KEY_WIDTH;

typedef struct Apk64AMediaExtractor Apk64AMediaExtractor;
typedef struct Apk64AMediaCodec Apk64AMediaCodec;
typedef struct Apk64AMediaDataSource Apk64AMediaDataSource;
typedef ssize_t (*Apk64AMediaDataSourceReadAt)(void *userdata, int64_t offset, void *buffer, size_t size);
typedef ssize_t (*Apk64AMediaDataSourceGetSize)(void *userdata);
typedef void (*Apk64AMediaDataSourceClose)(void *userdata);
typedef struct Apk64AMediaCodecBufferInfo {
    int32_t offset;
    int32_t size;
    int64_t presentationTimeUs;
    uint32_t flags;
} Apk64AMediaCodecBufferInfo;
_Static_assert(sizeof(Apk64AMediaCodecBufferInfo) == 24, "Android AArch64 AMediaCodecBufferInfo ABI size");

enum {
    APK64_AMEDIA_OK = 0,
    APK64_AMEDIA_ERROR_UNKNOWN = -10000,
    APK64_AMEDIA_ERROR_MALFORMED = -10001,
    APK64_AMEDIA_ERROR_UNSUPPORTED = -10002,
    APK64_AMEDIA_ERROR_INVALID_OBJECT = -10003,
    APK64_AMEDIA_ERROR_INVALID_PARAMETER = -10004,
    APK64_AMEDIA_ERROR_INVALID_OPERATION = -10005,
    APK64_AMEDIA_ERROR_END_OF_STREAM = -10006,
    APK64_AMEDIA_ERROR_IO = -10007,
    APK64_AMEDIA_ERROR_WOULD_BLOCK = -10008,
    APK64_AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED = -3,
    APK64_AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED = -2,
    APK64_AMEDIACODEC_INFO_TRY_AGAIN_LATER = -1,
    APK64_AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM = 4,
    APK64_AMEDIACODEC_CONFIGURE_FLAG_ENCODE = 1,
};

Apk64AMediaExtractor *apk64_AMediaExtractor_new(void);
int apk64_AMediaExtractor_delete(Apk64AMediaExtractor *ex);
int apk64_AMediaExtractor_setDataSourceFd(Apk64AMediaExtractor *ex, int fd, int64_t offset, int64_t length);
int apk64_AMediaExtractor_setDataSource(Apk64AMediaExtractor *ex, const char *location);
int apk64_AMediaExtractor_setDataSourceCustom(Apk64AMediaExtractor *ex, Apk64AMediaDataSource *source);
size_t apk64_AMediaExtractor_getTrackCount(Apk64AMediaExtractor *ex);
Apk64AMediaFormat *apk64_AMediaExtractor_getTrackFormat(Apk64AMediaExtractor *ex, size_t idx);
int apk64_AMediaExtractor_selectTrack(Apk64AMediaExtractor *ex, size_t idx);
ssize_t apk64_AMediaExtractor_readSampleData(Apk64AMediaExtractor *ex, uint8_t *buffer, size_t capacity);
int apk64_AMediaExtractor_getSampleTrackIndex(Apk64AMediaExtractor *ex);
int64_t apk64_AMediaExtractor_getSampleTime(Apk64AMediaExtractor *ex);
bool apk64_AMediaExtractor_advance(Apk64AMediaExtractor *ex);
int apk64_AMediaExtractor_seekTo(Apk64AMediaExtractor *ex, int64_t seek_pos_us, int mode);

Apk64AMediaCodec *apk64_AMediaCodec_createDecoderByType(const char *mime_type);
Apk64AMediaCodec *apk64_AMediaCodec_createEncoderByType(const char *mime_type);
int apk64_AMediaCodec_delete(Apk64AMediaCodec *codec);
int apk64_AMediaCodec_configure(Apk64AMediaCodec *codec, const Apk64AMediaFormat *format,
                                void *surface, void *crypto, uint32_t flags);
int apk64_AMediaCodec_start(Apk64AMediaCodec *codec);
int apk64_AMediaCodec_stop(Apk64AMediaCodec *codec);
int apk64_AMediaCodec_flush(Apk64AMediaCodec *codec);
ssize_t apk64_AMediaCodec_dequeueInputBuffer(Apk64AMediaCodec *codec, int64_t timeout_us);
uint8_t *apk64_AMediaCodec_getInputBuffer(Apk64AMediaCodec *codec, size_t idx, size_t *out_size);
int apk64_AMediaCodec_queueInputBuffer(Apk64AMediaCodec *codec, size_t idx, off_t offset,
                                       size_t size, uint64_t time_us, uint32_t flags);
ssize_t apk64_AMediaCodec_dequeueOutputBuffer(Apk64AMediaCodec *codec,
                                               Apk64AMediaCodecBufferInfo *info,
                                               int64_t timeout_us);
Apk64AMediaFormat *apk64_AMediaCodec_getOutputFormat(Apk64AMediaCodec *codec);
uint8_t *apk64_AMediaCodec_getOutputBuffer(Apk64AMediaCodec *codec, size_t idx, size_t *out_size);
int apk64_AMediaCodec_releaseOutputBuffer(Apk64AMediaCodec *codec, size_t idx, bool render);
int apk64_AMediaCodec_setOutputSurface(Apk64AMediaCodec *codec, void *surface);

Apk64AMediaDataSource *apk64_AMediaDataSource_new(void);
void apk64_AMediaDataSource_delete(Apk64AMediaDataSource *source);
void apk64_AMediaDataSource_setUserdata(Apk64AMediaDataSource *source, void *userdata);
void apk64_AMediaDataSource_setReadAt(Apk64AMediaDataSource *source, Apk64AMediaDataSourceReadAt read_at);
void apk64_AMediaDataSource_setGetSize(Apk64AMediaDataSource *source, Apk64AMediaDataSourceGetSize get_size);
void apk64_AMediaDataSource_setClose(Apk64AMediaDataSource *source, Apk64AMediaDataSourceClose close_cb);

typedef struct Apk64AImage Apk64AImage;
typedef struct Apk64AImageCropRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} Apk64AImageCropRect;
_Static_assert(sizeof(Apk64AImageCropRect) == 16, "Android AImageCropRect ABI size");

int apk64_AImage_getWidth(const Apk64AImage *image, int32_t *width);
int apk64_AImage_getHeight(const Apk64AImage *image, int32_t *height);
int apk64_AImage_getFormat(const Apk64AImage *image, int32_t *format);
int apk64_AImage_getTimestamp(const Apk64AImage *image, int64_t *timestamp_ns);
int apk64_AImage_getNumberOfPlanes(const Apk64AImage *image, int32_t *num_planes);
int apk64_AImage_getCropRect(const Apk64AImage *image, Apk64AImageCropRect *rect);


/* monotonic interface */

#include <stdint.h>
#include <time.h>

/* CLOCK_MONOTONIC shared by host monotonic timers and Android uptime facade.
 * Android uptime/elapsedRealtime must remain separate public entry points. */
static inline int64_t apk64_clock_monotonic_ns(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * INT64_C(1000000000) + (int64_t)ts.tv_nsec;
}
static inline uint64_t apk64_clock_monotonic_ms(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000) +
           (uint64_t)ts.tv_nsec / UINT64_C(1000000);
}


/* native_activity interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/* Minimal Android NDK NativeActivity ABI used by APK64 Stage 12.2.  The field
 * order matches android/native_activity.h for LP64.  These definitions are kept
 * local so APK64 does not depend on Android headers at Switch-build time. */
typedef struct Apk64AInputEvent Apk64AInputEvent;
typedef struct Apk64AInputQueue Apk64AInputQueue;
typedef struct Apk64ALooper Apk64ALooper;
typedef struct Apk64ANativeWindow Apk64ANativeWindow;
typedef struct Apk64AConfiguration Apk64AConfiguration;

typedef struct Apk64ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} Apk64ARect;

typedef struct Apk64ANativeActivity Apk64ANativeActivity;

typedef struct Apk64ANativeActivityCallbacks {
    void (*onStart)(Apk64ANativeActivity *activity);
    void (*onResume)(Apk64ANativeActivity *activity);
    void *(*onSaveInstanceState)(Apk64ANativeActivity *activity, size_t *out_size);
    void (*onPause)(Apk64ANativeActivity *activity);
    void (*onStop)(Apk64ANativeActivity *activity);
    void (*onDestroy)(Apk64ANativeActivity *activity);
    void (*onWindowFocusChanged)(Apk64ANativeActivity *activity, int has_focus);
    void (*onNativeWindowCreated)(Apk64ANativeActivity *activity, Apk64ANativeWindow *window);
    void (*onNativeWindowResized)(Apk64ANativeActivity *activity, Apk64ANativeWindow *window);
    void (*onNativeWindowRedrawNeeded)(Apk64ANativeActivity *activity, Apk64ANativeWindow *window);
    void (*onNativeWindowDestroyed)(Apk64ANativeActivity *activity, Apk64ANativeWindow *window);
    void (*onInputQueueCreated)(Apk64ANativeActivity *activity, Apk64AInputQueue *queue);
    void (*onInputQueueDestroyed)(Apk64ANativeActivity *activity, Apk64AInputQueue *queue);
    void (*onContentRectChanged)(Apk64ANativeActivity *activity, const Apk64ARect *rect);
    void (*onConfigurationChanged)(Apk64ANativeActivity *activity);
    void (*onLowMemory)(Apk64ANativeActivity *activity);
} Apk64ANativeActivityCallbacks;

struct Apk64ANativeActivity {
    Apk64ANativeActivityCallbacks *callbacks;
    void *vm;
    void *env;
    void *clazz;
    const char *internalDataPath;
    const char *externalDataPath;
    int32_t sdkVersion;
    void *instance;
    Apk64AssetManager *assetManager;
    const char *obbPath;
};

typedef struct Apk64NativeActivityConfig {
    const char *internal_data_path;
    const char *external_data_path;
    const char *obb_path;
    int32_t sdk_version;
    int32_t window_width;
    int32_t window_height;
    int32_t window_format;
    Apk64DisplayPlan display_plan;
} Apk64NativeActivityConfig;

/* Host/framework side construction and lifecycle driving. */
bool apk64_native_activity_init(const Apk64NativeActivityConfig *config,
                                void *vm, void *env, void *activity_object,
                                Apk64AssetManager *asset_manager);
void apk64_native_activity_reset(void);
Apk64ANativeActivity *apk64_native_activity(void);
Apk64ANativeWindow *apk64_native_window(void);
void apk64_native_window_prepare(int32_t width, int32_t height, int32_t format);
Apk64AInputQueue *apk64_native_input_queue(void);
bool apk64_native_window_dimensions(const Apk64ANativeWindow *window, int32_t *out_width, int32_t *out_height);

/* Drive the callbacks written by the guest's ANativeActivity_onCreate. */
bool apk64_native_activity_start(void);
void apk64_native_activity_pump_input(bool *out_exit_requested);
void apk64_native_activity_stop(void);

/* libandroid.so ABI exports. */
void ANativeActivity_finish(Apk64ANativeActivity *activity);
void ANativeActivity_setWindowFormat(Apk64ANativeActivity *activity, int32_t format);
void ANativeActivity_setWindowFlags(Apk64ANativeActivity *activity, uint32_t add_flags, uint32_t remove_flags);
void ANativeActivity_showSoftInput(Apk64ANativeActivity *activity, uint32_t flags);
void ANativeActivity_hideSoftInput(Apk64ANativeActivity *activity, uint32_t flags);

void ANativeWindow_acquire(Apk64ANativeWindow *window);
void ANativeWindow_release(Apk64ANativeWindow *window);
int32_t ANativeWindow_getWidth(Apk64ANativeWindow *window);
int32_t ANativeWindow_getHeight(Apk64ANativeWindow *window);
int32_t ANativeWindow_getFormat(Apk64ANativeWindow *window);
int32_t ANativeWindow_setBuffersGeometry(Apk64ANativeWindow *window, int32_t width, int32_t height, int32_t format);
Apk64ANativeWindow *ANativeWindow_fromSurface(void *env, void *surface);
void *ANativeWindow_toSurface(void *env, Apk64ANativeWindow *window);
int32_t ANativeWindow_lock(Apk64ANativeWindow *window, void *buffer, Apk64ARect *dirty);
int32_t ANativeWindow_unlockAndPost(Apk64ANativeWindow *window);

typedef struct Apk64AChoreographer Apk64AChoreographer;
Apk64AChoreographer *AChoreographer_getInstance(void);
void AChoreographer_postFrameCallback(Apk64AChoreographer *choreographer,
                                      void (*callback)(long frame_time_nanos, void *data),
                                      void *data);
void AChoreographer_postFrameCallback64(Apk64AChoreographer *choreographer,
                                        void (*callback)(int64_t frame_time_nanos, void *data),
                                        void *data);
void AChoreographer_postFrameCallbackDelayed(Apk64AChoreographer *choreographer,
                                             void (*callback)(long frame_time_nanos, void *data),
                                             void *data, long delay_millis);
void AChoreographer_postFrameCallbackDelayed64(Apk64AChoreographer *choreographer,
                                               void (*callback)(int64_t frame_time_nanos, void *data),
                                               void *data, uint32_t delay_millis);
/* Run due callbacks posted for the current thread using CLOCK_MONOTONIC frame time. */
void apk64_choreographer_pump(void);

Apk64AConfiguration *AConfiguration_new(void);
void AConfiguration_delete(Apk64AConfiguration *config);
void AConfiguration_copy(Apk64AConfiguration *dest, Apk64AConfiguration *src);
void AConfiguration_fromAssetManager(Apk64AConfiguration *config, Apk64AssetManager *manager);
int32_t AConfiguration_getMcc(Apk64AConfiguration *config);
void AConfiguration_setMcc(Apk64AConfiguration *config, int32_t mcc);
int32_t AConfiguration_getMnc(Apk64AConfiguration *config);
void AConfiguration_setMnc(Apk64AConfiguration *config, int32_t mnc);
void AConfiguration_getLanguage(Apk64AConfiguration *config, char *out_language);
void AConfiguration_setLanguage(Apk64AConfiguration *config, const char *language);
void AConfiguration_getCountry(Apk64AConfiguration *config, char *out_country);
void AConfiguration_setCountry(Apk64AConfiguration *config, const char *country);
int32_t AConfiguration_getOrientation(Apk64AConfiguration *config);
void AConfiguration_setOrientation(Apk64AConfiguration *config, int32_t orientation);
int32_t AConfiguration_getTouchscreen(Apk64AConfiguration *config);
void AConfiguration_setTouchscreen(Apk64AConfiguration *config, int32_t touchscreen);
int32_t AConfiguration_getDensity(Apk64AConfiguration *config);
void AConfiguration_setDensity(Apk64AConfiguration *config, int32_t density);
int32_t AConfiguration_getKeyboard(Apk64AConfiguration *config);
void AConfiguration_setKeyboard(Apk64AConfiguration *config, int32_t keyboard);
int32_t AConfiguration_getNavigation(Apk64AConfiguration *config);
void AConfiguration_setNavigation(Apk64AConfiguration *config, int32_t navigation);
int32_t AConfiguration_getKeysHidden(Apk64AConfiguration *config);
void AConfiguration_setKeysHidden(Apk64AConfiguration *config, int32_t keys_hidden);
int32_t AConfiguration_getNavHidden(Apk64AConfiguration *config);
void AConfiguration_setNavHidden(Apk64AConfiguration *config, int32_t nav_hidden);
int32_t AConfiguration_getSdkVersion(Apk64AConfiguration *config);
void AConfiguration_setSdkVersion(Apk64AConfiguration *config, int32_t sdk_version);
int32_t AConfiguration_getScreenSize(Apk64AConfiguration *config);
void AConfiguration_setScreenSize(Apk64AConfiguration *config, int32_t screen_size);
int32_t AConfiguration_getScreenLong(Apk64AConfiguration *config);
void AConfiguration_setScreenLong(Apk64AConfiguration *config, int32_t screen_long);
int32_t AConfiguration_getUiModeType(Apk64AConfiguration *config);
void AConfiguration_setUiModeType(Apk64AConfiguration *config, int32_t ui_mode_type);
int32_t AConfiguration_getUiModeNight(Apk64AConfiguration *config);
void AConfiguration_setUiModeNight(Apk64AConfiguration *config, int32_t ui_mode_night);
int32_t AConfiguration_getScreenWidthDp(Apk64AConfiguration *config);
void AConfiguration_setScreenWidthDp(Apk64AConfiguration *config, int32_t width_dp);
int32_t AConfiguration_getScreenHeightDp(Apk64AConfiguration *config);
void AConfiguration_setScreenHeightDp(Apk64AConfiguration *config, int32_t height_dp);
int32_t AConfiguration_getSmallestScreenWidthDp(Apk64AConfiguration *config);
void AConfiguration_setSmallestScreenWidthDp(Apk64AConfiguration *config, int32_t width_dp);
int32_t AConfiguration_getLayoutDirection(Apk64AConfiguration *config);
void AConfiguration_setLayoutDirection(Apk64AConfiguration *config, int32_t layout_direction);
int32_t AConfiguration_getScreenRound(Apk64AConfiguration *config);
void AConfiguration_setScreenRound(Apk64AConfiguration *config, int32_t screen_round);

Apk64ALooper *ALooper_forThread(void);
Apk64ALooper *ALooper_prepare(int opts);
void ALooper_acquire(Apk64ALooper *looper);
void ALooper_release(Apk64ALooper *looper);
int ALooper_addFd(Apk64ALooper *looper, int fd, int ident, int events,
                  int (*callback)(int fd, int events, void *data), void *data);
int ALooper_removeFd(Apk64ALooper *looper, int fd);
int ALooper_pollOnce(int timeout_millis, int *out_fd, int *out_events, void **out_data);
int ALooper_pollAll(int timeout_millis, int *out_fd, int *out_events, void **out_data);
void ALooper_wake(Apk64ALooper *looper);

void AInputQueue_attachLooper(Apk64AInputQueue *queue, Apk64ALooper *looper, int ident,
                              int (*callback)(int fd, int events, void *data), void *data);
void AInputQueue_detachLooper(Apk64AInputQueue *queue);
int32_t AInputQueue_getEvent(Apk64AInputQueue *queue, Apk64AInputEvent **out_event);
int32_t AInputQueue_preDispatchEvent(Apk64AInputQueue *queue, Apk64AInputEvent *event);
void AInputQueue_finishEvent(Apk64AInputQueue *queue, Apk64AInputEvent *event, int handled);
int32_t AInputEvent_getType(const Apk64AInputEvent *event);
int32_t AKeyEvent_getAction(const Apk64AInputEvent *event);
int32_t AKeyEvent_getKeyCode(const Apk64AInputEvent *event);
int32_t AMotionEvent_getAction(const Apk64AInputEvent *event);
size_t AMotionEvent_getPointerCount(const Apk64AInputEvent *event);
int32_t AMotionEvent_getPointerId(const Apk64AInputEvent *event, size_t pointer_index);
float AMotionEvent_getX(const Apk64AInputEvent *event, size_t pointer_index);
float AMotionEvent_getY(const Apk64AInputEvent *event, size_t pointer_index);


/* opensles interface */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Small OpenSL ES 1.0 compatibility surface. AndroidFD/MIME media players are
 * backed by the APK64 generic media engine. Android Simple Buffer Queue PCM
 * players are mixed directly into the same Stage-11 audout stream so engines
 * such as FMOD can use the standard Android OpenSL fallback path. */

typedef uint32_t Apk64SLresult;
typedef uint32_t Apk64SLboolean;
typedef uint32_t Apk64SLuint32;
typedef int32_t Apk64SLint32;
typedef uint32_t Apk64SLmillisecond;

enum {
    APK64_SL_RESULT_SUCCESS = 0,
    APK64_SL_RESULT_PRECONDITIONS_VIOLATED = 1,
    APK64_SL_RESULT_PARAMETER_INVALID = 2,
    APK64_SL_RESULT_MEMORY_FAILURE = 3,
    APK64_SL_RESULT_BUFFER_INSUFFICIENT = 7,
    APK64_SL_RESULT_FEATURE_UNSUPPORTED = 12,
    APK64_SL_RESULT_CONTENT_UNSUPPORTED = 9,
    APK64_SL_PLAYSTATE_STOPPED = 1,
    APK64_SL_PLAYSTATE_PAUSED = 2,
    APK64_SL_PLAYSTATE_PLAYING = 3,
};

/* Exported data symbols: the guest links against the address of these pointer
 * variables exactly as it would against Android's libOpenSLES.so. */
extern const void *const APK64_SL_IID_NULL;
extern const void *APK64_SL_IID_ENGINE;
extern const void *APK64_SL_IID_ENGINECAPABILITIES;
extern const void *APK64_SL_IID_AUDIOIODEVICECAPABILITIES;
extern const void *APK64_SL_IID_OUTPUTMIX;
extern const void *APK64_SL_IID_PLAY;
extern const void *APK64_SL_IID_SEEK;
extern const void *APK64_SL_IID_MUTESOLO;
extern const void *APK64_SL_IID_VOLUME;
extern const void *APK64_SL_IID_ENVIRONMENTALREVERB;
extern const void *APK64_SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
extern const void *APK64_SL_IID_ANDROIDCONFIGURATION;
extern const void *APK64_SL_IID_RECORD;
extern const void *APK64_SL_IID_BUFFERQUEUE;
extern const void *APK64_SL_IID_PREFETCHSTATUS;
extern const void *APK64_SL_IID_METADATAEXTRACTION;

Apk64SLresult slCreateEngine(void *out_engine_object,
                             Apk64SLuint32 num_options,
                             const void *options,
                             Apk64SLuint32 num_interfaces,
                             const void *interface_ids,
                             const void *interface_required);

/* True only when this APK64 build includes the PCM Simple Buffer Queue shape
 * used by Android low-latency engines such as FMOD. The OpenSL PCM backend is
 * part of the core runtime (not a runtime-optional module), so this is a
 * compile-time capability. If OpenSL becomes optional, replace this inline
 * answer with a real runtime backend-state query. */
static inline bool apk64_opensles_pcm_backend_available(void) { return true; }

void apk64_opensles_reset(void);
uint64_t apk64_opensles_player_create_count(void);

typedef struct Apk64SLPcmRuntimeStats {
    uint32_t players;
    uint32_t queued_buffers;
    uint64_t frames_mixed;
    uint64_t buffers_enqueued;
    uint64_t buffers_consumed;
} Apk64SLPcmRuntimeStats;

void apk64_opensles_get_pcm_runtime_stats(Apk64SLPcmRuntimeStats *out);

/* Mix queued OpenSL PCM into an existing interleaved stereo S16 buffer.
 * Returns the number of output frames for which at least one queue player had
 * PCM available. This is called by the generic media mixer, not by guest code. */
size_t apk64_opensles_mix_s16(int16_t *io_interleaved, size_t frames);


/* optional_sdk interface */


#include <stdbool.h>
#include <stddef.h>



/* Optional third-party SDK policy.
 *
 * APK64 does not provide an advertising surface/browser/video stack. Known ad
 * SDK entry points are answered with deterministic "unavailable" semantics
 * instead of executing provider code that cannot present an ad on Switch. The
 * policy is namespace-wide for recognized advertising SDKs: configuration and
 * constructors are inert, load/show operations do no work, readiness is false,
 * and no impression/click/reward completion is fabricated. Relocated library
 * dependencies in shadow/shaded namespaces and non-ad methods execute normally.
 */
/* Returns true for third-party advertising/monetization SDK class namespaces
 * that APK64 deliberately exposes only as unavailable. Manifest providers in
 * these namespaces must not be executed: provider startup is an SDK bootstrap
 * path and can spawn background workers even when no ad surface exists. */
/* Returns a stable policy reason for external SDK namespaces whose service
 * operations are unavailable. Their class metadata may still initialize. */
const char *apk64_optional_sdk_unavailable_reason(const char *class_name);
bool apk64_optional_sdk_is_unavailable_class(const char *class_name);
bool apk64_optional_sdk_skip_class_initialization(const char *class_name);

bool apk64_optional_sdk_can_handle(const char *class_name,
                                     const char *method_name,
                                     const char *signature);

Apk64Result apk64_optional_sdk_try_call(void *env_handle,
                                        void *receiver_ref,
                                        const char *class_name,
                                        const char *method_name,
                                        const char *signature,
                                        bool is_static,
                                        const Apk64JniValue *args,
                                        size_t arg_count,
                                        Apk64JniValue *out_result,
                                        bool *out_handled);


/* package interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#ifndef APK64_DEX_SIGNATURE_CAP
#define APK64_DEX_SIGNATURE_CAP 2048u
#endif

/* A package source is either one standalone APK or an XAPK ZIP containing a
 * base APK, zero or more split APKs, and optional OBB expansion files. The
 * public runtime still consumes Apk64ApkInfo so existing APK call sites remain
 * small; package-specific bookkeeping is reconstructed when extraction is
 * needed. */
bool apk64_package_is_source_name(const char *name);
bool apk64_package_is_xapk_name(const char *name);

/* Probe a logical Android package without executing it. For XAPK sources the
 * base APK is materialized into APK64's runtime cache and base_apk_path points
 * at it. The return value keeps the existing runtime classification semantics:
 * a valid Java-Activity package returns APK64_ERR_APK_UNSUPPORTED, while its
 * metadata remains populated for launcher diagnostics. */
Apk64Result apk64_package_probe(const char *source_path, Apk64ApkInfo *out_info);

/* Lightweight launcher-only metadata. This deliberately does not build the
 * runtime package plan, scan DEX, select ABI splits, or classify launchability.
 * It only obtains display metadata (label/version/package/source/base) needed by
 * the browser. Full package probing is deferred until the user selects a game. */
Apk64Result apk64_package_probe_launcher(const char *source_path, Apk64ApkInfo *out_info);

/* Extract all arm64-v8a libraries from the selected APK components. Components
 * without arm64 payload are ignored; no filename/package-specific rules are
 * used. */
Apk64Result apk64_package_extract_arm64(const char *source_path,
                                        const char *destination_dir,
                                        size_t *out_count);

/* Merge assets/ entries from selected split APKs into destination_dir. The base
 * APK remains the normal ZIP asset backend; this directory overlays split
 * assets with the existing directory-first AssetManager semantics. */
Apk64Result apk64_package_extract_split_assets(const char *source_path,
                                               const char *destination_dir,
                                               size_t *out_count);

/* Extract OBB expansion files carried by an XAPK into the app's Android-visible
 * OBB directory. Standalone APKs simply report zero files. */
Apk64Result apk64_package_extract_obb(const char *source_path,
                                      const char *destination_dir,
                                      size_t *out_count);

/* Launcher icon lookup across the logical package, preferring the base APK and
 * falling back to split APKs when a density split owns the icon. */
Apk64Result apk64_package_load_launcher_icon(const char *source_path,
                                             uint32_t **out_pixels,
                                             int *out_width,
                                             int *out_height,
                                             char *out_entry_name,
                                             size_t out_entry_name_cap);

/* Fast launcher path after apk64_package_probe(). For XAPK it reuses the
 * materialized component cache named by info->base_apk_path instead of
 * rebuilding the complete package plan just to locate a density-split icon. */
Apk64Result apk64_package_load_launcher_icon_from_info(const Apk64ApkInfo *info,
                                                       uint32_t **out_pixels,
                                                       int *out_width,
                                                       int *out_height,
                                                       char *out_entry_name,
                                                       size_t out_entry_name_cap);

/* Read an exact ZIP entry across every selected APK component of the logical
 * package and concatenate the payloads in component order. A newline is inserted
 * between non-empty component payloads. Missing entries are normal and produce
 * an empty successful result. This is suitable for class-path style resources
 * such as META-INF/services descriptors without exposing XAPK internals. */
Apk64Result apk64_package_read_selected_entry_concat(const char *source_path,
                                                       const char *entry_name,
                                                       uint8_t **out_data,
                                                       size_t *out_size);

/* Read an exact ZIP entry from the first selected APK component (class-path
 * order) that contains it, as dalvik.system.DexPathList.findResource does.
 * *out_found distinguishes a missing entry from an empty one. */
Apk64Result apk64_package_read_selected_entry_first(const char *source_path,
                                                      const char *entry_name,
                                                      uint8_t **out_data,
                                                      size_t *out_size,
                                                      bool *out_found);
/* Locate a resource without inflating it. Empty output means a class-path miss. */
Apk64Result apk64_package_find_selected_entry_path(const char *source_path, const char *entry_name,
                                                   char *out_path, size_t capacity);

/* Adds one dalvik.system.BaseDexClassLoader dexPath element (a .dex file, or a
 * .jar/.apk/.zip with classes.dex, classes2.dex, ...) to the class path
 * searched after the package and core DEX tiers. Elements load once per
 * process. *out_images counts DEX images found; APK64_ERR_SYMBOL_NOT_FOUND
 * when a zip element contains none. */
Apk64Result apk64_package_add_secondary_dex(const char *host_path, size_t *out_images);

/* An opened DexFile owns an immutable inventory, without changing the class
 * path. Publishing makes its images available to the interpreter resolver;
 * published bytes remain resident for interpreted methods after close. */
typedef struct Apk64DexFile Apk64DexFile;
Apk64Result apk64_package_dex_file_open(const char *host_path, Apk64DexFile **out_file);
size_t apk64_package_dex_file_count(const Apk64DexFile *file);
const char *apk64_package_dex_file_name(const Apk64DexFile *file, size_t index);
bool apk64_package_dex_file_contains(const Apk64DexFile *file, const char *slash_name);
Apk64Result apk64_package_dex_file_publish(Apk64DexFile *file);
void apk64_package_dex_file_close(Apk64DexFile *file);

/* Ensure the materialized base APK needed by AssetManager/framework startup is
 * available for a package selected from the launcher.  Persistent launcher
 * metadata can be read without re-extracting an XAPK; this call performs the
 * deferred base materialization only when the selected game is actually
 * launched. Standalone APKs are a no-op. */
Apk64Result apk64_package_prepare_launch_source(const char *source_path,
                                                Apk64ApkInfo *inout_info);

/* Resolve one JNI slash-form class name against the DEX class definitions in
 * the selected APK components. This is metadata lookup only: no bytecode is
 * interpreted and no package-specific class list is embedded in APK64.
 * out_found=false is a normal ClassNotFound result. When available, the
 * direct superclass is returned in the same slash form. */
Apk64Result apk64_package_find_dex_class(const char *source_path,
                                         const char *class_name,
                                         char *out_super_name,
                                         size_t out_super_name_cap,
                                         bool *out_found);
/* As above, and also returns the class_def's directly implemented interfaces
 * (slash form, '\n'-separated, in declaration order) in out_interfaces. */
Apk64Result apk64_package_find_dex_class_ex(const char *source_path,
                                            const char *class_name,
                                            char *out_super_name,
                                            size_t out_super_name_cap,
                                            char *out_interfaces,
                                            size_t out_interfaces_cap,
                                            bool *out_found);


/* Initial value for one DEX-defined static field. DEX class_def.static_values_off
 * supplies a compact encoded_array whose elements correspond to the class's
 * static fields in declaration order. Fields beyond the encoded array use the
 * Java zero/null default. APK64 exposes the decoded constant here so the DEX
 * interpreter can materialize a real Java static slot on first access without
 * silently discarding class-file constants. */
typedef enum Apk64DexStaticValueType {
    APK64_DEX_STATIC_DEFAULT = 0,
    APK64_DEX_STATIC_BYTE,
    APK64_DEX_STATIC_SHORT,
    APK64_DEX_STATIC_CHAR,
    APK64_DEX_STATIC_INT,
    APK64_DEX_STATIC_LONG,
    APK64_DEX_STATIC_FLOAT,
    APK64_DEX_STATIC_DOUBLE,
    APK64_DEX_STATIC_STRING,
    APK64_DEX_STATIC_NULL,
    APK64_DEX_STATIC_BOOLEAN,
    APK64_DEX_STATIC_UNSUPPORTED,
} Apk64DexStaticValueType;

typedef struct Apk64DexStaticValue {
    Apk64DexStaticValueType type;
    uint64_t bits;
    char string_value[512];
} Apk64DexStaticValue;

Apk64Result apk64_package_get_dex_static_field_initial_value(const char *source_path,
                                                              const char *class_name,
                                                              const char *field_name,
                                                              const char *signature,
                                                              Apk64DexStaticValue *out_value,
                                                              bool *out_class_found,
                                                              bool *out_field_found);

/* Build/load a persistent, package-fingerprint-scoped DEX cache and compact
 * class-to-DEX index. The cache stores raw DEX entries on disk so repeated
 * class/method resolution does not repeatedly inflate APK ZIP entries. */
Apk64Result apk64_package_prepare_dex_cache(const char *source_path);

/* Materialized metadata for one DEX-defined method. The backing DEX image is
 * owned by this object so code_item/constant-pool lookups remain valid while a
 * small interpreter is running. This is deliberately a DEX metadata/runtime
 * primitive, not an Activity- or game-specific entry point. */
typedef struct Apk64DexMethodImage {
    uint8_t *dex_data;
    size_t dex_size;
    /* Internal shared backing token. Package clients must not inspect this;
     * apk64_package_release_dex_method() releases the reference. */
    void *dex_owner;
    uint32_t method_index;
    uint32_t access_flags;
    uint32_t class_access_flags;   /* declaring class_def.access_flags */
    uint32_t code_offset;
    uint16_t registers_size;
    uint16_t ins_size;
    uint16_t outs_size;
    uint32_t insns_size;
    const uint16_t *insns;
    char declaring_class[512];
    char super_class[512];
    char method_name[256];
    char signature[APK64_DEX_SIGNATURE_CAP];
    char apk_path[APK64_APK_PATH_CAP];
    char dex_name[APK64_APK_ENTRY_CAP];
} Apk64DexMethodImage;

/* Look up an exact slash-form class/method/signature in DEX class_data. A
 * defined class with no matching method is reported as class_found=true and
 * method_found=false so callers can walk the superclass chain correctly. */
Apk64Result apk64_package_load_dex_method(const char *source_path,
                                          const char *class_name,
                                          const char *method_name,
                                          const char *signature,
                                          Apk64DexMethodImage *out_method,
                                          bool *out_class_found,
                                          bool *out_method_found);
void apk64_package_release_dex_method(Apk64DexMethodImage *method);

/* One member declared by a DEX class_def (fields and methods, including
 * constructors and <clinit>). Names and signatures are heap strings. */
typedef struct Apk64DexMember {
    bool is_method;
    bool is_static;
    uint32_t access_flags;
    char *name;
    char *signature;
} Apk64DexMember;

/* Lists every field and method the package DEX declares for class_name, in
 * class_data order. Reflection (Class.getDeclared*) needs the complete
 * declared set, not only the members that were already resolved. */
Apk64Result apk64_package_list_dex_class_members(const char *source_path, const char *class_name,
                                                 Apk64DexMember **out_members, size_t *out_count,
                                                 bool *out_class_found);
void apk64_package_free_dex_members(Apk64DexMember *members, size_t count);

/* Raw view of the DEX file that defines one class. The image is shared with
 * the DEX cache and stays valid until apk64_package_release_dex_class().
 * class_def_offset locates the 32-byte class_def_item inside dex_data, so
 * metadata consumers (annotations, EnclosingClass/InnerClass, Signature)
 * decode the same bytes the interpreter executes. */
typedef struct Apk64DexClassView {
    uint8_t *dex_data;
    size_t dex_size;
    void *dex_owner;          /* internal; released by apk64_package_release_dex_class */
    uint32_t class_def_offset;
} Apk64DexClassView;

Apk64Result apk64_package_acquire_dex_class(const char *source_path, const char *class_name,
                                            Apk64DexClassView *out_view, bool *out_class_found);
void apk64_package_release_dex_class(Apk64DexClassView *view);

/* APK64 core library (boot class path). The DEX lookups above fall back to it
 * when the package DEX does not define a class. True when it defines one. */
bool apk64_package_core_library_defines(const char *class_name);
/* Returns "aosp-overlay" or "legacy" for the boot DEX layer that owns
 * class_name, or NULL when the class is not supplied by APK64's Java core. */
const char *apk64_package_core_library_origin(const char *class_name);

/* Drops the in-memory package plan, DEX index and DEX image caches. The
 * installer calls this whenever an installed package file is replaced or
 * removed; runtime lookups otherwise trust the validated index. */
void apk64_package_invalidate_source_caches(void);

/* Runtime-only diagnostics for the bounded shared/warm DEX backing cache. */
void apk64_package_get_dex_runtime_cache_stats(uint64_t *out_hits,
                                               uint64_t *out_loads,
                                               uint64_t *out_bytes);
void apk64_package_get_dex_runtime_cache_detail(uint32_t *out_warm_entries,
                                                uint64_t *out_evictions);
void apk64_package_reset_dex_runtime_cache(void);


/* path_lookup interface */

#include <stdbool.h>

/* Cache only missing ancestor directories inside APK64-owned private storage.
 * No file contents/metadata, external storage or arbitrary host paths cached.
 * Every successful mkdir/rename into this namespace must publish a change
 * before returning to guest code; epoch checks reject racing discoveries. */
void apk64_path_lookup_set_root(const char *host_root);
bool apk64_path_lookup_missing_parent(const char *host_path);
void apk64_path_lookup_record_missing_parent(const char *host_path);
void apk64_path_lookup_changed(const char *host_path);


/* perf_account interface */
#ifndef APK64_PERF_ACCOUNT_H
#define APK64_PERF_ACCOUNT_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

typedef enum Apk64PerfMode {
    APK64_PERF_OFF, APK64_PERF_LIGHT, APK64_PERF_DEEP
} Apk64PerfMode;
/* Configure before workers start; mode is fixed for the session. */
void apk64_perf_configure(Apk64PerfMode mode);
Apk64PerfMode apk64_perf_mode(void);

/* Per-thread wall-time accounting by category (diagnostics). A slow call can
 * then be broken down into where its thread actually spent the time. Nested
 * scopes of the same kind count once (the outermost). */
typedef enum Apk64PerfKind {
    APK64_PERF_FILE_IO = 0,   /* guest file syscalls (open/read/write/close/fsync/stat...) */
    APK64_PERF_JAVA,          /* Java executed for a native caller (JNI -> DEX) */
    APK64_PERF_GUEST_WAIT,    /* blocking waits (mutex/cond/futex/Java waits) */
    APK64_PERF_PRESENT,       /* eglSwapBuffers */
    APK64_PERF_HOST_LOCK,     /* contended runtime locks (registry, DEX index, malloc...) */
    APK64_PERF_NATIVE,        /* guest native JNI wall time, not CPU attribution */
    APK64_PERF_KIND_COUNT
} Apk64PerfKind;

typedef struct Apk64PerfScope {
    int kind;
    uint64_t begin_us;
} Apk64PerfScope;

uint64_t apk64_perf_now_us(void);
Apk64PerfScope apk64_perf_scope_begin(int kind);
void apk64_perf_scope_end(Apk64PerfScope *scope);
/* Cumulative microseconds of the calling thread, per kind. */
void apk64_perf_account_read(uint64_t out[APK64_PERF_KIND_COUNT]);

/* Fixed storage, no allocation/formatting/I/O on producers. A busy diagnostic
 * lock drops observations rather than blocking a guest/render thread. */
#define APK64_PERF_FRAME_BINS 512u
#define APK64_PERF_FRAME_BIN_US 250u
#define APK64_PERF_TRACE_CAP 256u
typedef struct Apk64PerfFrameStats {
    uintptr_t surface;
    pthread_t thread;
    uint64_t intervals, span_us, worst_us, swap_total_us, swap_max_us;
    uint64_t begin_us, end_us; /* Same monotonic clock as native and scheduling observations. */
    uint64_t over_33ms, over_50ms, over_100ms;
    uint64_t bins[APK64_PERF_FRAME_BINS];
} Apk64PerfFrameStats;
typedef struct Apk64PerfEvent {
    uint64_t time_us, duration_us;
    pthread_t thread;
    uintptr_t object;
    int kind; /* PerfKind = end, -kind-1 = begin, COUNT = present, COUNT+1 = PC sample */
    uint64_t cpu_ticks; /* native sample only; cumulative kernel counter */
    int cpu_valid;
} Apk64PerfEvent;
/* Only successful window swaps; begin/end use the monotonic microsecond clock.
 * Completion intervals are not GPU timestamps or proof of missed deadlines. */
void apk64_perf_present(uintptr_t surface, uint64_t begin_us, uint64_t end_us);
void apk64_perf_forget_surface(uintptr_t surface);
void apk64_perf_native_sample(pthread_t thread, uintptr_t pc, uint64_t cpu_ticks, int cpu_valid);
size_t apk64_perf_frame_take(Apk64PerfFrameStats *out, size_t cap);
uint64_t apk64_perf_frame_percentile_upper(const Apk64PerfFrameStats *s, unsigned percentile);
/* Oldest-first pre-gap trace, frozen at the first completed interval >33ms.
 * One consumer; drain after gameplay or on the existing sampler thread. */
size_t apk64_perf_trace_take(Apk64PerfEvent *out, size_t cap, uint64_t *gap_us);
uint64_t apk64_perf_dropped(void);
void apk64_perf_report(void);

#define APK64_PERF_SCOPE(kind) \
    Apk64PerfScope apk64_perf_scope_ __attribute__((cleanup(apk64_perf_scope_end))) = \
        apk64_perf_scope_begin(kind)

/* Lock contention accounting. Each APK64_PERF_LOCK call site owns a static
 * site record; an uncontended acquisition costs one trylock. A contended one
 * is timed and charged to the site and to the thread's HOST_LOCK total. */
typedef struct Apk64PerfLockSite {
    const char *file;
    int line;
    const char *name;
    uint64_t wait_us;    /* atomic */
    uint64_t contended;  /* atomic */
    uint64_t reported_us;
    uint64_t reported_n;
    struct Apk64PerfLockSite *next;
    int registered;      /* atomic */
} Apk64PerfLockSite;

typedef struct Apk64PerfLockDelta {
    const Apk64PerfLockSite *site;
    uint64_t wait_us;
    uint64_t contended;
} Apk64PerfLockDelta;

/* Weak so translation units linked without perf_account.c (host tests) keep
 * plain pthread_mutex_lock semantics. */
void apk64_perf_lock_contended(pthread_mutex_t *mutex, Apk64PerfLockSite *site) __attribute__((weak));
/* Sites with wait time since the previous call, largest first. */
size_t apk64_perf_lock_take_deltas(Apk64PerfLockDelta *out, size_t cap) __attribute__((weak));

static inline void apk64_perf_mutex_lock(pthread_mutex_t *mutex, Apk64PerfLockSite *site) {
    if (pthread_mutex_trylock(mutex) == 0) return;
    if (apk64_perf_lock_contended) apk64_perf_lock_contended(mutex, site);
    else pthread_mutex_lock(mutex);
}

#define APK64_PERF_LOCK(mutex) do { \
    static Apk64PerfLockSite apk64_lock_site_ = {__FILE__, __LINE__, #mutex, 0, 0, 0, 0, NULL, 0}; \
    apk64_perf_mutex_lock((mutex), &apk64_lock_site_); \
} while (0)

#endif


/* proc_memory interface */

#include <stdbool.h>
#include <stddef.h>

/* Map supported guest memory-accounting nodes to their live read-only backend. */
bool apk64_proc_memory_map(const char *guest_path, char *out, size_t capacity);


/* regex_compat interface */
#ifndef APK64_REGEX_COMPAT_H
#define APK64_REGEX_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Android/Bionic LP64 regex ABI (bionic/libc/include/regex.h). */
typedef struct Apk64BionicRegex {
    int re_magic;
    size_t re_nsub;
    const char *re_endp;
    void *re_g;
} Apk64BionicRegex;

typedef int64_t Apk64BionicRegoff;
typedef struct Apk64BionicRegmatch {
    Apk64BionicRegoff rm_so;
    Apk64BionicRegoff rm_eo;
} Apk64BionicRegmatch;

enum {
    APK64_REG_EXTENDED = 0001,
    APK64_REG_ICASE = 0002,
    APK64_REG_NOSUB = 0004,
    APK64_REG_NEWLINE = 0010,
    APK64_REG_NOSPEC = 0020,
    APK64_REG_PEND = 0040,

    APK64_REG_NOMATCH = 1,
    APK64_REG_BADPAT = 2,
    APK64_REG_ECOLLATE = 3,
    APK64_REG_ECTYPE = 4,
    APK64_REG_EESCAPE = 5,
    APK64_REG_ESUBREG = 6,
    APK64_REG_EBRACK = 7,
    APK64_REG_EPAREN = 8,
    APK64_REG_EBRACE = 9,
    APK64_REG_BADBR = 10,
    APK64_REG_ERANGE = 11,
    APK64_REG_ESPACE = 12,
    APK64_REG_BADRPT = 13,
    APK64_REG_EMPTY = 14,
    APK64_REG_ASSERT = 15,
    APK64_REG_INVARG = 16,
    APK64_REG_ILLSEQ = 17,

    APK64_REG_NOTBOL = 00001,
    APK64_REG_NOTEOL = 00002,
    APK64_REG_STARTEND = 00004
};

int apk64_bionic_regcomp(Apk64BionicRegex *preg, const char *pattern, int cflags);
int apk64_bionic_regexec(const Apk64BionicRegex *preg, const char *string,
                         size_t nmatch, Apk64BionicRegmatch pmatch[], int eflags);
void apk64_bionic_regfree(Apk64BionicRegex *preg);

#ifdef __cplusplus
}
#endif

#endif


/* sensors interface */


#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>


/* Minimal Android NDK sensor ABI used by NativeActivity games.  These opaque
 * types deliberately mirror android/sensor.h at the call boundary without
 * requiring Android headers in the Switch build. */
typedef struct Apk64ASensorManager Apk64ASensorManager;
typedef struct Apk64ASensor Apk64ASensor;
typedef struct Apk64ASensorEventQueue Apk64ASensorEventQueue;
typedef struct Apk64ALooper Apk64ALooper;

enum {
    APK64_ASENSOR_TYPE_ACCELEROMETER = 1,
    APK64_ASENSOR_TYPE_GYROSCOPE = 4,
};

typedef struct Apk64ASensorEvent {
    int32_t version;
    int32_t sensor;
    int32_t type;
    int32_t reserved0;
    int64_t timestamp;
    float data[16];
    uint32_t flags;
    int32_t reserved1[3];
} Apk64ASensorEvent;

Apk64ASensorManager *ASensorManager_getInstance(void);
Apk64ASensorManager *ASensorManager_getInstanceForPackage(const char *package_name);
int ASensorManager_getSensorList(Apk64ASensorManager *manager,
                                 const Apk64ASensor *const **out_list);
const Apk64ASensor *ASensorManager_getDefaultSensor(Apk64ASensorManager *manager, int type);
Apk64ASensorEventQueue *ASensorManager_createEventQueue(Apk64ASensorManager *manager,
                                                         Apk64ALooper *looper,
                                                         int ident,
                                                         int (*callback)(int fd, int events, void *data),
                                                         void *data);
int ASensorManager_destroyEventQueue(Apk64ASensorManager *manager, Apk64ASensorEventQueue *queue);
int ASensorEventQueue_enableSensor(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor);
int ASensorEventQueue_disableSensor(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor);
int ASensorEventQueue_setEventRate(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor,
                                   int32_t usec);
int ASensorEventQueue_hasEvents(Apk64ASensorEventQueue *queue);
ssize_t ASensorEventQueue_getEvents(Apk64ASensorEventQueue *queue,
                                    Apk64ASensorEvent *events, size_t count);
const char *ASensor_getName(const Apk64ASensor *sensor);
const char *ASensor_getVendor(const Apk64ASensor *sensor);
int ASensor_getType(const Apk64ASensor *sensor);
float ASensor_getResolution(const Apk64ASensor *sensor);
int ASensor_getMinDelay(const Apk64ASensor *sensor);

/* Called by the APK64 Switch input pump after each hardware sample. */
void apk64_sensors_publish(const Apk64InputSnapshot *snapshot);
/* Copy the current Android-unit samples without consuming any NDK queue. */
size_t apk64_sensors_read_latest(Apk64ASensorEvent *events, size_t capacity);
void apk64_sensors_reset(void);


/* shader_compat interface */

#include <stddef.h>

/* Compatibility retry only: make nonnegative index constants explicitly uint
 * where integer arithmetic already requires uint. NULL means no safe edits.
 * Caller owns the returned source; the shader version and lines are preserved. */
char *apk64_shader_compat_source(const char *source, size_t length, size_t *conversions);


/* shared_file interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
struct stat;
int apk64_shared_file_stat(const char *, struct stat *) __attribute__((weak));
/* -2 means the path has no live writable shared mapping. */
int apk64_shared_file_truncate_open(const char *, int64_t) __attribute__((weak));

/* Optional guest shared-file page cache. -2 means the range is not owned. */
void *apk64_shared_file_map(size_t, int, int, int, int64_t) __attribute__((weak));
int apk64_shared_file_unmap(void *, size_t) __attribute__((weak));
int apk64_shared_file_sync(void *, size_t, int) __attribute__((weak));
int apk64_shared_file_protect(void *, size_t, int) __attribute__((weak));
bool apk64_shared_file_contains(void *, size_t) __attribute__((weak));
void apk64_shared_file_reset(void) __attribute__((weak));
ssize_t apk64_shared_read(int, void *, size_t) __attribute__((weak));
ssize_t apk64_shared_write(int, const void *, size_t) __attribute__((weak));
size_t apk64_shared_fread(void *, size_t, size_t, FILE *) __attribute__((weak));
size_t apk64_shared_fwrite(const void *, size_t, size_t, FILE *) __attribute__((weak));
int apk64_shared_fflush(FILE *) __attribute__((weak));
int apk64_shared_fsync(int) __attribute__((weak));
int apk64_shared_ftruncate(int, off_t) __attribute__((weak));
int apk64_shared_open(const char *, int, ...) __attribute__((weak));
FILE *apk64_shared_fopen(const char *, const char *) __attribute__((weak));
int apk64_shared_fclose(FILE *) __attribute__((weak));
int apk64_shared_rename(const char *, const char *) __attribute__((weak));
int apk64_shared_fgetc(FILE *) __attribute__((weak));
char *apk64_shared_fgets(char *, int, FILE *) __attribute__((weak));
int apk64_shared_fputc(int, FILE *) __attribute__((weak));
int apk64_shared_fputs(const char *, FILE *) __attribute__((weak));
int apk64_shared_vfprintf(FILE *, const char *, va_list) __attribute__((weak));
int apk64_shared_vfscanf(FILE *, const char *, va_list) __attribute__((weak));
int apk64_shared_fseek(FILE *, long, int) __attribute__((weak));
int apk64_shared_fseeko(FILE *, off_t, int) __attribute__((weak));
void apk64_shared_rewind(FILE *) __attribute__((weak));
FILE *apk64_shared_freopen(const char *, const char *, FILE *) __attribute__((weak));


/* tls_peer interface */
#ifndef APK64_TLS_PEER_H
#define APK64_TLS_PEER_H
#include <sys/socket.h>
#include <arpa/inet.h>
#include <mbedtls/x509_crt.h>
#include <stdbool.h>
#include <string.h>
/* Mbed TLS 2.28 verifies DNS names. HTTPS IP identities instead require an
 * exact iPAddress SAN; never accept a common-name-only IP certificate. */
static inline void apk64_tls_verify_ip_san(const mbedtls_x509_crt *cert, const char *name, uint32_t *flags) {
    if (!name) return;
    char literal[INET6_ADDRSTRLEN];
    size_t length=strlen(name);
    if(length>2&&name[0]=='['&&name[length-1]==']'&&length-2<sizeof(literal)) {
        memcpy(literal,name+1,length-2);literal[length-2]=0;name=literal;
    }
    unsigned char ip[16];
    size_t size=inet_pton(AF_INET,name,ip)==1?4:inet_pton(AF_INET6,name,ip)==1?16:0;
    if(!size)return;
    bool matched=false;
    for(const mbedtls_x509_sequence *san=&cert->subject_alt_names;san;san=san->next)
        if((san->buf.tag&MBEDTLS_ASN1_TAG_VALUE_MASK)==MBEDTLS_X509_SAN_IP_ADDRESS&&san->buf.len==size&&!memcmp(san->buf.p,ip,size))matched=true;
    if(matched)*flags&=~MBEDTLS_X509_BADCERT_CN_MISMATCH;
    else *flags|=MBEDTLS_X509_BADCERT_CN_MISMATCH;
}
#endif


/* unicode_normalization interface */

#include <stddef.h>
#include <stdint.h>

/* Form values follow java.text.Normalizer.Form declaration order. The caller
 * owns the returned UTF-16 buffer. Unpaired surrogates and embedded NULs survive.
 * Returns 0 on success, -1 for invalid arguments, -2 for allocation failure. */
enum { APK64_UN_NFD=0, APK64_UN_NFC=1, APK64_UN_NFKD=2, APK64_UN_NFKC=3 };
int apk64_unicode_normalize(const uint16_t *input, size_t length, int form,
                            uint16_t **output, size_t *output_length);


/* vm_space interface */
#ifndef APK64_VM_SPACE_H
#define APK64_VM_SPACE_H

#include <stdbool.h>
#include <stddef.h>

/* Address-space reservations for guest images and sparse guest mappings.
 *
 * Every range APK64 hands out is recorded here as well as with libnx, so a
 * placement never overlaps another APK64 reservation even before it is
 * mapped. Placement is deterministic: ranges under 1 GiB are packed from the
 * bottom of the ASLR region and larger ones from the top, which keeps one
 * large hole in the middle for runtimes that reserve multi-GiB aligned cages
 * (V8/cppgc, Go, PartitionAlloc) the way Linux lets them. */
typedef struct Apk64VmRange Apk64VmRange;

/* Reserves len bytes (page multiple) at an address aligned to align (a power
 * of two, at least one page). Returns the base and stores the range handle,
 * or NULL when no unmapped span fits. */
void *apk64_vm_reserve(size_t len, size_t align, Apk64VmRange **out_range);

/* Narrows a reservation to [base, base + len), which must lie inside it. */
bool apk64_vm_shrink(Apk64VmRange *range, void *base, size_t len);

/* Releases a reservation; the handle is invalid afterwards. NULL is ignored. */
void apk64_vm_release(Apk64VmRange *range);

#endif


/* zip_bytes interface */

#include <stdint.h>

/* ZIP/APK little-endian metadata reads, shared by archives and asset indices. */
static inline uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint64_t rd64(const uint8_t *p) {
    return (uint64_t)rd32(p) | ((uint64_t)rd32(p + 4) << 32);
}


/* zip_crc32 interface */

#include <stddef.h>
#include <stdint.h>

/* Canonical reflected CRC32, incremental state begins with 0xffffffff. */
static const uint32_t k_crc32_table[256] = {
    UINT32_C(0x00000000), UINT32_C(0x77073096), UINT32_C(0xee0e612c), UINT32_C(0x990951ba), UINT32_C(0x076dc419), UINT32_C(0x706af48f), UINT32_C(0xe963a535), UINT32_C(0x9e6495a3),
    UINT32_C(0x0edb8832), UINT32_C(0x79dcb8a4), UINT32_C(0xe0d5e91e), UINT32_C(0x97d2d988), UINT32_C(0x09b64c2b), UINT32_C(0x7eb17cbd), UINT32_C(0xe7b82d07), UINT32_C(0x90bf1d91),
    UINT32_C(0x1db71064), UINT32_C(0x6ab020f2), UINT32_C(0xf3b97148), UINT32_C(0x84be41de), UINT32_C(0x1adad47d), UINT32_C(0x6ddde4eb), UINT32_C(0xf4d4b551), UINT32_C(0x83d385c7),
    UINT32_C(0x136c9856), UINT32_C(0x646ba8c0), UINT32_C(0xfd62f97a), UINT32_C(0x8a65c9ec), UINT32_C(0x14015c4f), UINT32_C(0x63066cd9), UINT32_C(0xfa0f3d63), UINT32_C(0x8d080df5),
    UINT32_C(0x3b6e20c8), UINT32_C(0x4c69105e), UINT32_C(0xd56041e4), UINT32_C(0xa2677172), UINT32_C(0x3c03e4d1), UINT32_C(0x4b04d447), UINT32_C(0xd20d85fd), UINT32_C(0xa50ab56b),
    UINT32_C(0x35b5a8fa), UINT32_C(0x42b2986c), UINT32_C(0xdbbbc9d6), UINT32_C(0xacbcf940), UINT32_C(0x32d86ce3), UINT32_C(0x45df5c75), UINT32_C(0xdcd60dcf), UINT32_C(0xabd13d59),
    UINT32_C(0x26d930ac), UINT32_C(0x51de003a), UINT32_C(0xc8d75180), UINT32_C(0xbfd06116), UINT32_C(0x21b4f4b5), UINT32_C(0x56b3c423), UINT32_C(0xcfba9599), UINT32_C(0xb8bda50f),
    UINT32_C(0x2802b89e), UINT32_C(0x5f058808), UINT32_C(0xc60cd9b2), UINT32_C(0xb10be924), UINT32_C(0x2f6f7c87), UINT32_C(0x58684c11), UINT32_C(0xc1611dab), UINT32_C(0xb6662d3d),
    UINT32_C(0x76dc4190), UINT32_C(0x01db7106), UINT32_C(0x98d220bc), UINT32_C(0xefd5102a), UINT32_C(0x71b18589), UINT32_C(0x06b6b51f), UINT32_C(0x9fbfe4a5), UINT32_C(0xe8b8d433),
    UINT32_C(0x7807c9a2), UINT32_C(0x0f00f934), UINT32_C(0x9609a88e), UINT32_C(0xe10e9818), UINT32_C(0x7f6a0dbb), UINT32_C(0x086d3d2d), UINT32_C(0x91646c97), UINT32_C(0xe6635c01),
    UINT32_C(0x6b6b51f4), UINT32_C(0x1c6c6162), UINT32_C(0x856530d8), UINT32_C(0xf262004e), UINT32_C(0x6c0695ed), UINT32_C(0x1b01a57b), UINT32_C(0x8208f4c1), UINT32_C(0xf50fc457),
    UINT32_C(0x65b0d9c6), UINT32_C(0x12b7e950), UINT32_C(0x8bbeb8ea), UINT32_C(0xfcb9887c), UINT32_C(0x62dd1ddf), UINT32_C(0x15da2d49), UINT32_C(0x8cd37cf3), UINT32_C(0xfbd44c65),
    UINT32_C(0x4db26158), UINT32_C(0x3ab551ce), UINT32_C(0xa3bc0074), UINT32_C(0xd4bb30e2), UINT32_C(0x4adfa541), UINT32_C(0x3dd895d7), UINT32_C(0xa4d1c46d), UINT32_C(0xd3d6f4fb),
    UINT32_C(0x4369e96a), UINT32_C(0x346ed9fc), UINT32_C(0xad678846), UINT32_C(0xda60b8d0), UINT32_C(0x44042d73), UINT32_C(0x33031de5), UINT32_C(0xaa0a4c5f), UINT32_C(0xdd0d7cc9),
    UINT32_C(0x5005713c), UINT32_C(0x270241aa), UINT32_C(0xbe0b1010), UINT32_C(0xc90c2086), UINT32_C(0x5768b525), UINT32_C(0x206f85b3), UINT32_C(0xb966d409), UINT32_C(0xce61e49f),
    UINT32_C(0x5edef90e), UINT32_C(0x29d9c998), UINT32_C(0xb0d09822), UINT32_C(0xc7d7a8b4), UINT32_C(0x59b33d17), UINT32_C(0x2eb40d81), UINT32_C(0xb7bd5c3b), UINT32_C(0xc0ba6cad),
    UINT32_C(0xedb88320), UINT32_C(0x9abfb3b6), UINT32_C(0x03b6e20c), UINT32_C(0x74b1d29a), UINT32_C(0xead54739), UINT32_C(0x9dd277af), UINT32_C(0x04db2615), UINT32_C(0x73dc1683),
    UINT32_C(0xe3630b12), UINT32_C(0x94643b84), UINT32_C(0x0d6d6a3e), UINT32_C(0x7a6a5aa8), UINT32_C(0xe40ecf0b), UINT32_C(0x9309ff9d), UINT32_C(0x0a00ae27), UINT32_C(0x7d079eb1),
    UINT32_C(0xf00f9344), UINT32_C(0x8708a3d2), UINT32_C(0x1e01f268), UINT32_C(0x6906c2fe), UINT32_C(0xf762575d), UINT32_C(0x806567cb), UINT32_C(0x196c3671), UINT32_C(0x6e6b06e7),
    UINT32_C(0xfed41b76), UINT32_C(0x89d32be0), UINT32_C(0x10da7a5a), UINT32_C(0x67dd4acc), UINT32_C(0xf9b9df6f), UINT32_C(0x8ebeeff9), UINT32_C(0x17b7be43), UINT32_C(0x60b08ed5),
    UINT32_C(0xd6d6a3e8), UINT32_C(0xa1d1937e), UINT32_C(0x38d8c2c4), UINT32_C(0x4fdff252), UINT32_C(0xd1bb67f1), UINT32_C(0xa6bc5767), UINT32_C(0x3fb506dd), UINT32_C(0x48b2364b),
    UINT32_C(0xd80d2bda), UINT32_C(0xaf0a1b4c), UINT32_C(0x36034af6), UINT32_C(0x41047a60), UINT32_C(0xdf60efc3), UINT32_C(0xa867df55), UINT32_C(0x316e8eef), UINT32_C(0x4669be79),
    UINT32_C(0xcb61b38c), UINT32_C(0xbc66831a), UINT32_C(0x256fd2a0), UINT32_C(0x5268e236), UINT32_C(0xcc0c7795), UINT32_C(0xbb0b4703), UINT32_C(0x220216b9), UINT32_C(0x5505262f),
    UINT32_C(0xc5ba3bbe), UINT32_C(0xb2bd0b28), UINT32_C(0x2bb45a92), UINT32_C(0x5cb36a04), UINT32_C(0xc2d7ffa7), UINT32_C(0xb5d0cf31), UINT32_C(0x2cd99e8b), UINT32_C(0x5bdeae1d),
    UINT32_C(0x9b64c2b0), UINT32_C(0xec63f226), UINT32_C(0x756aa39c), UINT32_C(0x026d930a), UINT32_C(0x9c0906a9), UINT32_C(0xeb0e363f), UINT32_C(0x72076785), UINT32_C(0x05005713),
    UINT32_C(0x95bf4a82), UINT32_C(0xe2b87a14), UINT32_C(0x7bb12bae), UINT32_C(0x0cb61b38), UINT32_C(0x92d28e9b), UINT32_C(0xe5d5be0d), UINT32_C(0x7cdcefb7), UINT32_C(0x0bdbdf21),
    UINT32_C(0x86d3d2d4), UINT32_C(0xf1d4e242), UINT32_C(0x68ddb3f8), UINT32_C(0x1fda836e), UINT32_C(0x81be16cd), UINT32_C(0xf6b9265b), UINT32_C(0x6fb077e1), UINT32_C(0x18b74777),
    UINT32_C(0x88085ae6), UINT32_C(0xff0f6a70), UINT32_C(0x66063bca), UINT32_C(0x11010b5c), UINT32_C(0x8f659eff), UINT32_C(0xf862ae69), UINT32_C(0x616bffd3), UINT32_C(0x166ccf45),
    UINT32_C(0xa00ae278), UINT32_C(0xd70dd2ee), UINT32_C(0x4e048354), UINT32_C(0x3903b3c2), UINT32_C(0xa7672661), UINT32_C(0xd06016f7), UINT32_C(0x4969474d), UINT32_C(0x3e6e77db),
    UINT32_C(0xaed16a4a), UINT32_C(0xd9d65adc), UINT32_C(0x40df0b66), UINT32_C(0x37d83bf0), UINT32_C(0xa9bcae53), UINT32_C(0xdebb9ec5), UINT32_C(0x47b2cf7f), UINT32_C(0x30b5ffe9),
    UINT32_C(0xbdbdf21c), UINT32_C(0xcabac28a), UINT32_C(0x53b39330), UINT32_C(0x24b4a3a6), UINT32_C(0xbad03605), UINT32_C(0xcdd70693), UINT32_C(0x54de5729), UINT32_C(0x23d967bf),
    UINT32_C(0xb3667a2e), UINT32_C(0xc4614ab8), UINT32_C(0x5d681b02), UINT32_C(0x2a6f2b94), UINT32_C(0xb40bbe37), UINT32_C(0xc30c8ea1), UINT32_C(0x5a05df1b), UINT32_C(0x2d02ef8d),
};

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        crc = k_crc32_table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
    }
    return crc;
}

static uint32_t crc32_bytes(const uint8_t *data, size_t size) {
    return ~crc32_update(UINT32_C(0xffffffff), data, size);
}



/* zip_directory_core interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* ZIP physical format only. Archive-policy decisions and error codes are left
 * to the APK and AssetManager backends. No pointers escape the input buffers. */
typedef struct Apk64ZipEocd {
    uint16_t disk;
    uint16_t central_disk;
    uint16_t disk_entries;
    uint16_t entries;
    uint32_t central_size;
    uint32_t central_offset;
} Apk64ZipEocd;

static inline bool apk64_zip_decode_eocd(const uint8_t *tail, size_t tail_size,
                                          bool require_exact_end, Apk64ZipEocd *out) {
    if (!tail || !out || tail_size < 22u) return false;
    for (size_t i = tail_size - 22u + 1u; i-- > 0;) {
        if (rd32(tail + i) != UINT32_C(0x06054b50)) continue;
        const size_t end = i + 22u + (size_t)rd16(tail + i + 20u);
        if (end > tail_size || (require_exact_end && end != tail_size)) continue;
        out->disk = rd16(tail + i + 4u);
        out->central_disk = rd16(tail + i + 6u);
        out->disk_entries = rd16(tail + i + 8u);
        out->entries = rd16(tail + i + 10u);
        out->central_size = rd32(tail + i + 12u);
        out->central_offset = rd32(tail + i + 16u);
        return true;
    }
    return false;
}

typedef struct Apk64ZipCentralEntry {
    const uint8_t *name;
    const uint8_t *extra;
    uint16_t flags, method, name_size, extra_size, comment_size;
    uint32_t dos_time, crc32, compressed_size, uncompressed_size, local_offset;
    size_t record_size;
} Apk64ZipCentralEntry;

static inline bool apk64_zip_decode_central(const uint8_t *buffer, size_t size,
                                             size_t offset, Apk64ZipCentralEntry *out) {
    if (!buffer || !out || offset > size || size - offset < 46u) return false;
    const uint8_t *hdr = buffer + offset;
    if (rd32(hdr) != UINT32_C(0x02014b50)) return false;
    const uint16_t name_len = rd16(hdr + 28u);
    const uint16_t extra_len = rd16(hdr + 30u);
    const uint16_t comment_len = rd16(hdr + 32u);
    const size_t record = 46u + (size_t)name_len + extra_len + comment_len;
    if (record > size - offset) return false;
    out->flags = rd16(hdr + 8u);
    out->method = rd16(hdr + 10u);
    out->dos_time = rd32(hdr + 12u);
    out->crc32 = rd32(hdr + 16u);
    out->compressed_size = rd32(hdr + 20u);
    out->uncompressed_size = rd32(hdr + 24u);
    out->local_offset = rd32(hdr + 42u);
    out->name_size = name_len;
    out->extra_size = extra_len;
    out->comment_size = comment_len;
    out->name = hdr + 46u;
    out->extra = out->name + name_len;
    out->record_size = record;
    return true;
}

/* Offset calculation shared by ZIP stored assets and APK extraction; caller
 * still validates file bounds and method-specific constraints. */
static inline bool apk64_zip_local_data_offset(const uint8_t *local_header, size_t size,
                                                uint64_t header_offset, uint64_t *out) {
    if (!local_header || !out || size < 30u ||
        rd32(local_header) != UINT32_C(0x04034b50)) return false;
    *out = header_offset + 30u + rd16(local_header + 26u) + rd16(local_header + 28u);
    return *out >= header_offset;
}


/* zip_inflate interface */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Canonical raw RFC 1951 decoder used by APK, assets and PNG. */
/* ---- Tiny raw-DEFLATE decoder ------------------------------------------------
 * APK entries using ZIP compression method 8 contain a raw RFC 1951 stream.
 * This decoder intentionally implements only the DEFLATE format, not zlib or
 * gzip wrappers, so the runtime has no external zlib/libzip dependency. */

typedef struct BitReader {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint32_t bits;
    unsigned count;
} BitReader;

typedef struct Huffman {
    uint16_t count[16];
    uint16_t symbol[288];
} Huffman;

static bool br_bits(BitReader *br, unsigned need, uint32_t *out) {
    if (!br || !out || need > 24) return false;
    while (br->count < need) {
        if (br->pos >= br->size) return false;
        br->bits |= (uint32_t)br->data[br->pos++] << br->count;
        br->count += 8;
    }
    *out = need ? (br->bits & ((UINT32_C(1) << need) - 1u)) : 0u;
    br->bits >>= need;
    br->count -= need;
    return true;
}

static void br_align(BitReader *br) {
    if (!br) return;
    br->bits = 0;
    br->count = 0;
}

static bool huff_construct(Huffman *h, const uint8_t *lengths, size_t count) {
    if (!h || (!lengths && count)) return false;
    memset(h, 0, sizeof(*h));
    for (size_t i = 0; i < count; ++i) {
        if (lengths[i] > 15) return false;
        h->count[lengths[i]]++;
    }
    if (h->count[0] == count) return true;

    int left = 1;
    for (unsigned len = 1; len <= 15; ++len) {
        left <<= 1;
        left -= h->count[len];
        if (left < 0) return false;
    }

    uint16_t offsets[16] = {0};
    offsets[1] = 0;
    for (unsigned len = 1; len < 15; ++len) offsets[len + 1] = (uint16_t)(offsets[len] + h->count[len]);
    for (size_t sym = 0; sym < count; ++sym) {
        const uint8_t len = lengths[sym];
        if (len) h->symbol[offsets[len]++] = (uint16_t)sym;
    }
    return true;
}

static int huff_decode(BitReader *br, const Huffman *h) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len <= 15; ++len) {
        uint32_t bit = 0;
        if (!br_bits(br, 1, &bit)) return -1;
        code |= (int)bit;
        const int count = h->count[len];
        if (code >= first && code < first + count) return h->symbol[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

static bool build_fixed(Huffman *litlen, Huffman *dist) {
    uint8_t ll[288];
    uint8_t dd[32];
    for (int i = 0; i <= 143; ++i) ll[i] = 8;
    for (int i = 144; i <= 255; ++i) ll[i] = 9;
    for (int i = 256; i <= 279; ++i) ll[i] = 7;
    for (int i = 280; i <= 287; ++i) ll[i] = 8;
    for (int i = 0; i < 32; ++i) dd[i] = 5;
    return huff_construct(litlen, ll, 288) && huff_construct(dist, dd, 32);
}

static bool build_dynamic(BitReader *br, Huffman *litlen, Huffman *dist) {
    static const uint8_t order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    uint32_t v = 0;
    if (!br_bits(br, 5, &v)) return false;
    const unsigned nlit = v + 257;
    if (!br_bits(br, 5, &v)) return false;
    const unsigned ndist = v + 1;
    if (!br_bits(br, 4, &v)) return false;
    const unsigned ncode = v + 4;
    if (nlit > 286 || ndist > 32) return false;

    uint8_t codelen[19] = {0};
    for (unsigned i = 0; i < ncode; ++i) {
        if (!br_bits(br, 3, &v)) return false;
        codelen[order[i]] = (uint8_t)v;
    }
    Huffman code_tree;
    if (!huff_construct(&code_tree, codelen, 19)) return false;

    uint8_t lengths[286 + 32];
    unsigned have = 0;
    const unsigned total = nlit + ndist;
    while (have < total) {
        const int sym = huff_decode(br, &code_tree);
        if (sym < 0) return false;
        if (sym <= 15) {
            lengths[have++] = (uint8_t)sym;
        } else if (sym == 16) {
            if (have == 0 || !br_bits(br, 2, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            const uint8_t prev = lengths[have - 1];
            while (repeat--) lengths[have++] = prev;
        } else if (sym == 17) {
            if (!br_bits(br, 3, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else if (sym == 18) {
            if (!br_bits(br, 7, &v)) return false;
            unsigned repeat = v + 11;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else {
            return false;
        }
    }
    if (lengths[256] == 0) return false;
    return huff_construct(litlen, lengths, nlit) && huff_construct(dist, lengths + nlit, ndist);
}

static bool inflate_codes(BitReader *br, const Huffman *litlen, const Huffman *dist,
                          uint8_t *out, size_t out_size, size_t *out_pos) {
    static const uint16_t len_base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
    };
    static const uint8_t len_extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
    };
    static const uint16_t dist_base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,
        4097,6145,8193,12289,16385,24577
    };
    static const uint8_t dist_extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
    };

    for (;;) {
        const int sym = huff_decode(br, litlen);
        if (sym < 0) return false;
        if (sym < 256) {
            if (*out_pos >= out_size) return false;
            out[(*out_pos)++] = (uint8_t)sym;
            continue;
        }
        if (sym == 256) return true;
        if (sym < 257 || sym > 285) return false;
        const unsigned li = (unsigned)(sym - 257);
        uint32_t extra = 0;
        if (!br_bits(br, len_extra[li], &extra)) return false;
        const size_t length = (size_t)len_base[li] + extra;

        const int dsym = huff_decode(br, dist);
        if (dsym < 0 || dsym > 29) return false;
        if (!br_bits(br, dist_extra[dsym], &extra)) return false;
        const size_t distance = (size_t)dist_base[dsym] + extra;
        if (distance == 0 || distance > *out_pos || length > out_size - *out_pos) return false;
        for (size_t i = 0; i < length; ++i) {
            out[*out_pos] = out[*out_pos - distance];
            (*out_pos)++;
        }
    }
}

static bool inflate_raw(const uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    BitReader br = {src, src_size, 0, 0, 0};
    size_t out_pos = 0;
    bool final = false;
    while (!final) {
        uint32_t bit = 0, type = 0;
        if (!br_bits(&br, 1, &bit) || !br_bits(&br, 2, &type)) return false;
        final = bit != 0;
        if (type == 0) {
            br_align(&br);
            if (br.pos + 4 > br.size) return false;
            const uint16_t len = (uint16_t)(br.data[br.pos] | ((uint16_t)br.data[br.pos + 1] << 8));
            const uint16_t nlen = (uint16_t)(br.data[br.pos + 2] | ((uint16_t)br.data[br.pos + 3] << 8));
            br.pos += 4;
            if ((uint16_t)~len != nlen || br.pos + len > br.size || len > dst_size - out_pos) return false;
            memcpy(dst + out_pos, br.data + br.pos, len);
            br.pos += len;
            out_pos += len;
        } else if (type == 1 || type == 2) {
            Huffman litlen, dist;
            if (type == 1) {
                if (!build_fixed(&litlen, &dist)) return false;
            } else if (!build_dynamic(&br, &litlen, &dist)) {
                return false;
            }
            if (!inflate_codes(&br, &litlen, &dist, dst, dst_size, &out_pos)) return false;
        } else {
            return false;
        }
    }
    return out_pos == dst_size;
}



/* media_internal interface */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/types.h>

#define APK64_MEDIA_SOURCE_RING_FRAMES 8192u
#define APK64_MEDIA_COMPRESSED_BUFFER_BYTES 32768u
#define APK64_MEDIA_DECODE_FRAME_CAP 1152u

typedef struct Apk64MediaSource {
    int fd;
    bool owns_fd;
    int64_t base;
    int64_t length;
    int64_t pos;
} Apk64MediaSource;

typedef struct Apk64Mp3Decoder Apk64Mp3Decoder;

typedef struct Apk64StereoRing {
    float *samples; /* interleaved stereo */
    size_t capacity_frames;
    size_t head;
    size_t count;
} Apk64StereoRing;

struct Apk64MediaClip {
    int16_t *samples; /* interleaved stereo at source_rate */
    size_t frames;
    uint32_t source_rate;
    _Atomic uint32_t refcount;
    int codec;
};

bool apk64_media_source_init(Apk64MediaSource *src, int fd, int64_t offset, int64_t length);
void apk64_media_source_close(Apk64MediaSource *src);
ssize_t apk64_media_source_read(Apk64MediaSource *src, void *dst, size_t bytes);
bool apk64_media_source_seek(Apk64MediaSource *src, int64_t relative);


struct Apk64MediaClip *apk64_media_clip_decode_path_internal(const char *path);
/* Content-sniffed decode of an in-memory WAV/Ogg file (borrowed bytes). */
struct Apk64MediaClip *apk64_media_clip_decode_memory_internal(const unsigned char *data, size_t length,
                                                              const char *label);

bool apk64_stereo_ring_init(Apk64StereoRing *ring, size_t capacity_frames);
void apk64_stereo_ring_reset(Apk64StereoRing *ring);
void apk64_stereo_ring_free(Apk64StereoRing *ring);
size_t apk64_stereo_ring_space(const Apk64StereoRing *ring);
bool apk64_stereo_ring_push(Apk64StereoRing *ring, float left, float right);
bool apk64_stereo_ring_peek(const Apk64StereoRing *ring, size_t index, float *left, float *right);
void apk64_stereo_ring_discard(Apk64StereoRing *ring, size_t frames);

Apk64Mp3Decoder *apk64_mp3_decoder_create(Apk64MediaSource *source);
void apk64_mp3_decoder_destroy(Apk64Mp3Decoder *decoder);
bool apk64_mp3_decoder_reset(Apk64Mp3Decoder *decoder);
bool apk64_mp3_decoder_decode_into(Apk64Mp3Decoder *decoder, Apk64StereoRing *ring,
                                   uint32_t *sample_rate, bool *eof,
                                   uint64_t *decoded_mp3_frames,
                                   uint64_t *source_bytes_read);


/* standalone interface */

#define NX_ROOT "sdmc:/switch/papaspaleteria_nx"
#define NX_GAME_APK NX_ROOT "/game.apk"
#define NX_SAVES NX_ROOT "/saves"
#define APK64_RUNTIME_APPS NX_ROOT "/cache/runtime/apps"
#define NX_LOG NX_ROOT "/logs/papaspaleteria_nx.log"
#define NX_PACKAGE "air.com.flipline.papaspaleteriatogo"
bool nx_storage_init(void);
bool nx_storage_path_allowed(const char *,bool);
