#ifndef _LTTNG_TRACEPOINT_H
#define _LTTNG_TRACEPOINT_H

/*
 * Copyright 2011-2012 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <lttng/tracepoint-types.h>
#include <lttng/tracepoint-rcu.h>
#include <urcu/compiler.h>
#include <urcu/system.h>
#include <dlfcn.h>	/* for dlopen */
#include <string.h>	/* for memset */
#include <lttng/ust-config.h>	/* for sdt */
#include <lttng/ust-compiler.h>

#ifdef LTTNG_UST_HAVE_SDT_INTEGRATION
#define SDT_USE_VARIADIC
#include <sys/sdt.h>
#define LTTNG_STAP_PROBEV STAP_PROBEV
#else
#define LTTNG_STAP_PROBEV(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define tracepoint_enabled(provider, name) \
	caa_unlikely(CMM_LOAD_SHARED(__tracepoint_##provider##___##name.state))

#define do_tracepoint(provider, name, ...) \
	__tracepoint_cb_##provider##___##name(__VA_ARGS__)

#define tracepoint(provider, name, ...)					    \
	do {								    \
		LTTNG_STAP_PROBEV(provider, name, ## __VA_ARGS__);	    \
		if (tracepoint_enabled(provider, name)) 		    \
			do_tracepoint(provider, name, __VA_ARGS__);	    \
	} while (0)

#define TP_ARGS(...)       __VA_ARGS__

#include <lttng/tracepoint_gen.h>

/*
 * The tracepoint cb is marked always inline so we can distinguish
 * between caller's ip addresses within the probe using the return
 * address.
 */
#define _DECLARE_TRACEPOINT(_provider, _name, ...)			 		\
extern struct lttng_ust_tracepoint __tracepoint_##_provider##___##_name;		\
static inline __attribute__((always_inline, unused)) lttng_ust_notrace			\
void __tracepoint_cb_##_provider##___##_name(_TP_ARGS_PROTO(__VA_ARGS__));		\
static											\
void __tracepoint_cb_##_provider##___##_name(_TP_ARGS_PROTO(__VA_ARGS__))		\
{											\
	struct lttng_ust_tracepoint_probe *__tp_probe;						\
											\
	if (caa_unlikely(!TP_RCU_LINK_TEST()))						\
		return;									\
	tp_rcu_read_lock_bp();								\
	__tp_probe = tp_rcu_dereference_bp(__tracepoint_##_provider##___##_name.probes); \
	if (caa_unlikely(!__tp_probe))							\
		goto end;								\
	do {										\
		void (*__tp_cb)(void) = __tp_probe->func;				\
		void *__tp_data = __tp_probe->data;					\
											\
		URCU_FORCE_CAST(void (*)(_TP_ARGS_DATA_PROTO(__VA_ARGS__)), __tp_cb)	\
				(_TP_ARGS_DATA_VAR(__VA_ARGS__));			\
	} while ((++__tp_probe)->func);							\
end:											\
	tp_rcu_read_unlock_bp();							\
}											\
static inline lttng_ust_notrace								\
void __tracepoint_register_##_provider##___##_name(char *name,				\
		void (*func)(void), void *data);					\
static inline										\
void __tracepoint_register_##_provider##___##_name(char *name,				\
		void (*func)(void), void *data)						\
{											\
	__tracepoint_probe_register(name, func, data,					\
		__tracepoint_##_provider##___##_name.signature);			\
}											\
static inline lttng_ust_notrace								\
void __tracepoint_unregister_##_provider##___##_name(char *name,			\
		void (*func)(void), void *data);					\
static inline										\
void __tracepoint_unregister_##_provider##___##_name(char *name,			\
		void (*func)(void), void *data)						\
{											\
	__tracepoint_probe_unregister(name, func, data);				\
}

extern int __tracepoint_probe_register(const char *name, void (*func)(void),
		void *data, const char *signature);
extern int __tracepoint_probe_unregister(const char *name, void (*func)(void),
		void *data);

/*
 * tracepoint dynamic linkage handling (callbacks). Hidden visibility:
 * shared across objects in a module/main executable.
 */
struct lttng_ust_tracepoint_dlopen {
	void *liblttngust_handle;

	int (*tracepoint_register_lib)(struct lttng_ust_tracepoint * const *tracepoints_start,
		int tracepoints_count);
	int (*tracepoint_unregister_lib)(struct lttng_ust_tracepoint * const *tracepoints_start);
	void (*rcu_read_lock_sym_bp)(void);
	void (*rcu_read_unlock_sym_bp)(void);
	void *(*rcu_dereference_sym_bp)(void *p);
};

extern struct lttng_ust_tracepoint_dlopen tracepoint_dlopen;
extern struct lttng_ust_tracepoint_dlopen *tracepoint_dlopen_ptr;

/* Disable tracepoint destructors. */
int __tracepoints__disable_destructors __attribute__((weak));

/*
 * Programs that have threads that survive after they exit, and
 * therefore call library destructors, should disable the tracepoint
 * destructors by calling tracepoint_disable_destructors(). This will
 * leak the tracepoint instrumentation library shared object, leaving
 * its teardown to the operating system process teardown.
 */
static inline void tracepoint_disable_destructors(void)
{
	__tracepoints__disable_destructors = 1;
}

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the tracepoints per shared-ojbect
 * (or for the whole main program).
 */
int __tracepoint_registered
	__attribute__((weak, visibility("hidden")));
int __tracepoint_ptrs_registered
	__attribute__((weak, visibility("hidden")));
struct lttng_ust_tracepoint_dlopen tracepoint_dlopen
	__attribute__((weak, visibility("hidden")));
/*
 * Deal with gcc O1 optimisation issues with weak hidden symbols. gcc
 * 4.8 and prior does not have the same behavior for symbol scoping on
 * 32-bit powerpc depending on the object size: symbols for objects of 8
 * bytes or less have the same address throughout a module, whereas they
 * have different addresses between compile units for objects larger
 * than 8 bytes. Add this pointer indirection to ensure that the symbol
 * scoping match that of the other weak hidden symbols found in this
 * header.
 */
struct lttng_ust_tracepoint_dlopen *tracepoint_dlopen_ptr
	__attribute__((weak, visibility("hidden")));

#ifndef _LGPL_SOURCE
static inline void lttng_ust_notrace
__tracepoint__init_urcu_sym(void);
static inline void
__tracepoint__init_urcu_sym(void)
{
	if (!tracepoint_dlopen_ptr)
		tracepoint_dlopen_ptr = &tracepoint_dlopen;
	/*
	 * Symbols below are needed by tracepoint call sites and probe
	 * providers.
	 */
	if (!tracepoint_dlopen_ptr->rcu_read_lock_sym_bp)
		tracepoint_dlopen_ptr->rcu_read_lock_sym_bp =
			URCU_FORCE_CAST(void (*)(void),
				dlsym(tracepoint_dlopen_ptr->liblttngust_handle,
					"tp_rcu_read_lock_bp"));
	if (!tracepoint_dlopen_ptr->rcu_read_unlock_sym_bp)
		tracepoint_dlopen_ptr->rcu_read_unlock_sym_bp =
			URCU_FORCE_CAST(void (*)(void),
				dlsym(tracepoint_dlopen_ptr->liblttngust_handle,
					"tp_rcu_read_unlock_bp"));
	if (!tracepoint_dlopen_ptr->rcu_dereference_sym_bp)
		tracepoint_dlopen_ptr->rcu_dereference_sym_bp =
			URCU_FORCE_CAST(void *(*)(void *p),
				dlsym(tracepoint_dlopen_ptr->liblttngust_handle,
					"tp_rcu_dereference_sym_bp"));
}
#else
static inline void lttng_ust_notrace
__tracepoint__init_urcu_sym(void);
static inline void
__tracepoint__init_urcu_sym(void)
{
}
#endif

static void lttng_ust_notrace __attribute__((constructor))
__tracepoints__init(void);
static void
__tracepoints__init(void)
{
	if (__tracepoint_registered++) {
		if (!tracepoint_dlopen_ptr->liblttngust_handle)
			return;
		__tracepoint__init_urcu_sym();
		return;
	}

	if (!tracepoint_dlopen_ptr)
		tracepoint_dlopen_ptr = &tracepoint_dlopen;
	if (!tracepoint_dlopen_ptr->liblttngust_handle)
		tracepoint_dlopen_ptr->liblttngust_handle =
			dlopen("liblttng-ust-tracepoint.so.0", RTLD_NOW | RTLD_GLOBAL);
	if (!tracepoint_dlopen_ptr->liblttngust_handle)
		return;
	__tracepoint__init_urcu_sym();
}

static void lttng_ust_notrace __attribute__((destructor))
__tracepoints__destroy(void);
static void
__tracepoints__destroy(void)
{
	int ret;

	if (--__tracepoint_registered)
		return;
	if (!tracepoint_dlopen_ptr)
		tracepoint_dlopen_ptr = &tracepoint_dlopen;
	if (!__tracepoints__disable_destructors
			&& tracepoint_dlopen_ptr->liblttngust_handle
			&& !__tracepoint_ptrs_registered) {
		ret = dlclose(tracepoint_dlopen_ptr->liblttngust_handle);
		if (ret) {
			fprintf(stderr, "Error (%d) in dlclose\n", ret);
			abort();
		}
		memset(tracepoint_dlopen_ptr, 0, sizeof(*tracepoint_dlopen_ptr));
	}
}

#ifdef TRACEPOINT_DEFINE

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the tracepoints per shared-ojbect
 * (or for the whole main program).
 */
extern struct lttng_ust_tracepoint * const __start___tracepoints_ptrs[]
	__attribute__((weak, visibility("hidden")));
extern struct lttng_ust_tracepoint * const __stop___tracepoints_ptrs[]
	__attribute__((weak, visibility("hidden")));

/*
 * When TRACEPOINT_PROBE_DYNAMIC_LINKAGE is defined, we do not emit a
 * unresolved symbol that requires the provider to be linked in. When
 * TRACEPOINT_PROBE_DYNAMIC_LINKAGE is not defined, we emit an
 * unresolved symbol that depends on having the provider linked in,
 * otherwise the linker complains. This deals with use of static
 * libraries, ensuring that the linker does not remove the provider
 * object from the executable.
 */
#ifdef TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#define _TRACEPOINT_UNDEFINED_REF(provider)	NULL
#else	/* TRACEPOINT_PROBE_DYNAMIC_LINKAGE */
#define _TRACEPOINT_UNDEFINED_REF(provider)	&__tracepoint_provider_##provider
#endif /* TRACEPOINT_PROBE_DYNAMIC_LINKAGE */

/*
 * Note: to allow PIC code, we need to allow the linker to update the pointers
 * in the __tracepoints_ptrs section.
 * Therefore, this section is _not_ const (read-only).
 */
#define _TP_EXTRACT_STRING(...)	#__VA_ARGS__

#define _DEFINE_TRACEPOINT(_provider, _name, _args)				\
	extern int __tracepoint_provider_##_provider; 				\
	static const char __tp_strtab_##_provider##___##_name[]			\
		__attribute__((section("__tracepoints_strings"))) =		\
			#_provider ":" #_name;					\
	struct lttng_ust_tracepoint __tracepoint_##_provider##___##_name	\
		__attribute__((section("__tracepoints"))) =			\
		{								\
			__tp_strtab_##_provider##___##_name,			\
			0,							\
			NULL,							\
			_TRACEPOINT_UNDEFINED_REF(_provider), 			\
			_TP_EXTRACT_STRING(_args),				\
			{ },							\
		};								\
	static struct lttng_ust_tracepoint *					\
		__tracepoint_ptr_##_provider##___##_name			\
		__attribute__((used, section("__tracepoints_ptrs"))) =		\
			&__tracepoint_##_provider##___##_name;

static void lttng_ust_notrace __attribute__((constructor))
__tracepoints__ptrs_init(void);
static void
__tracepoints__ptrs_init(void)
{
	if (__tracepoint_ptrs_registered++)
		return;
	if (!tracepoint_dlopen_ptr)
		tracepoint_dlopen_ptr = &tracepoint_dlopen;
	if (!tracepoint_dlopen_ptr->liblttngust_handle)
		tracepoint_dlopen_ptr->liblttngust_handle =
			dlopen("liblttng-ust-tracepoint.so.0", RTLD_NOW | RTLD_GLOBAL);
	if (!tracepoint_dlopen_ptr->liblttngust_handle)
		return;
	tracepoint_dlopen_ptr->tracepoint_register_lib =
		URCU_FORCE_CAST(int (*)(struct lttng_ust_tracepoint * const *, int),
				dlsym(tracepoint_dlopen_ptr->liblttngust_handle,
					"tracepoint_register_lib"));
	tracepoint_dlopen_ptr->tracepoint_unregister_lib =
		URCU_FORCE_CAST(int (*)(struct lttng_ust_tracepoint * const *),
				dlsym(tracepoint_dlopen_ptr->liblttngust_handle,
					"tracepoint_unregister_lib"));
	__tracepoint__init_urcu_sym();
	if (tracepoint_dlopen_ptr->tracepoint_register_lib) {
		tracepoint_dlopen_ptr->tracepoint_register_lib(__start___tracepoints_ptrs,
				__stop___tracepoints_ptrs -
				__start___tracepoints_ptrs);
	}
}

static void lttng_ust_notrace __attribute__((destructor))
__tracepoints__ptrs_destroy(void);
static void
__tracepoints__ptrs_destroy(void)
{
	int ret;

	if (--__tracepoint_ptrs_registered)
		return;
	if (!tracepoint_dlopen_ptr)
		tracepoint_dlopen_ptr = &tracepoint_dlopen;
	if (tracepoint_dlopen_ptr->tracepoint_unregister_lib)
		tracepoint_dlopen_ptr->tracepoint_unregister_lib(__start___tracepoints_ptrs);
	if (!__tracepoints__disable_destructors
			&& tracepoint_dlopen_ptr->liblttngust_handle
			&& !__tracepoint_ptrs_registered) {
		ret = dlclose(tracepoint_dlopen_ptr->liblttngust_handle);
		if (ret) {
			fprintf(stderr, "Error (%d) in dlclose\n", ret);
			abort();
		}
		memset(tracepoint_dlopen_ptr, 0, sizeof(*tracepoint_dlopen_ptr));
	}
}

#else /* TRACEPOINT_DEFINE */

#define _DEFINE_TRACEPOINT(_provider, _name, _args)

#endif /* #else TRACEPOINT_DEFINE */

#ifdef __cplusplus
}
#endif

#endif /* _LTTNG_TRACEPOINT_H */

/* The following declarations must be outside re-inclusion protection. */

#ifndef TRACEPOINT_ENUM

/*
 * Tracepoint Enumerations
 *
 * The enumeration is a mapping between an integer, or range of integers, and
 * a string. It can be used to have a more compact trace in cases where the
 * possible values for a field are limited:
 *
 * An example:
 *
 * TRACEPOINT_ENUM(someproject_component, enumname,
 *	TP_ENUM_VALUES(
 *		ctf_enum_value("even", 0)
 *		ctf_enum_value("uneven", 1)
 *		ctf_enum_range("twoto4", 2, 4)
 *		ctf_enum_value("five", 5)
 *	)
 * )
 *
 * Where "someproject_component" is the name of the component this enumeration
 * belongs to and "enumname" identifies this enumeration. Inside the
 * TP_ENUM_VALUES macro is the actual mapping. Each string value can map
 * to either a single value with ctf_enum_value or a range of values
 * with ctf_enum_range.
 *
 * Enumeration ranges may overlap, but the behavior is implementation-defined,
 * each trace reader will handle overlapping as it wishes.
 *
 * That enumeration can then be used in a field inside the TP_FIELD macro using
 * the following line:
 *
 * ctf_enum(someproject_component, enumname, enumtype, enumfield, enumval)
 *
 * Where "someproject_component" and "enumname" match those in the
 * TRACEPOINT_ENUM, "enumtype" is a signed or unsigned integer type
 * backing the enumeration, "enumfield" is the name of the field and
 * "enumval" is the value.
 */

#define TRACEPOINT_ENUM(provider, name, values)

#endif /* #ifndef TRACEPOINT_ENUM */

#ifndef TRACEPOINT_EVENT

/*
 * How to use the TRACEPOINT_EVENT macro:
 *
 * An example:
 * 
 * TRACEPOINT_EVENT(someproject_component, event_name,
 *
 *     * TP_ARGS takes from 0 to 10 "type, field_name" pairs *
 *
 *     TP_ARGS(int, arg0, void *, arg1, char *, string, size_t, strlen,
 *             long *, arg4, size_t, arg4_len),
 *
 *	* TP_FIELDS describes the event payload layout in the trace *
 *
 *     TP_FIELDS(
 *         * Integer, printed in base 10 * 
 *         ctf_integer(int, field_a, arg0)
 *
 *         * Integer, printed with 0x base 16 * 
 *         ctf_integer_hex(unsigned long, field_d, arg1)
 *
 *         * Enumeration *
 *         ctf_enum(someproject_component, enum_name, int, field_e, arg0)
 *
 *         * Array Sequence, printed as UTF8-encoded array of bytes * 
 *         ctf_array_text(char, field_b, string, FIXED_LEN)
 *         ctf_sequence_text(char, field_c, string, size_t, strlen)
 *
 *         * String, printed as UTF8-encoded string * 
 *         ctf_string(field_e, string)
 *
 *         * Array sequence of signed integer values * 
 *         ctf_array(long, field_f, arg4, FIXED_LEN4)
 *         ctf_sequence(long, field_g, arg4, size_t, arg4_len)
 *     )
 * )
 *
 * More detailed explanation:
 *
 * The name of the tracepoint is expressed as a tuple with the provider
 * and name arguments.
 *
 * The provider and name should be a proper C99 identifier.
 * The "provider" and "name" MUST follow these rules to ensure no
 * namespace clash occurs:
 *
 * For projects (applications and libraries) for which an entity
 * specific to the project controls the source code and thus its
 * tracepoints (typically with a scope larger than a single company):
 *
 * either:
 *   project_component, event
 * or:
 *   project, event
 *
 * Where "project" is the name of the project,
 *       "component" is the name of the project component (which may
 *       include several levels of sub-components, e.g.
 *       ...component_subcomponent_...) where the tracepoint is located
 *       (optional),
 *       "event" is the name of the tracepoint event.
 *
 * For projects issued from a single company wishing to advertise that
 * the company controls the source code and thus the tracepoints, the
 * "com_" prefix should be used:
 *
 * either:
 *   com_company_project_component, event
 * or:
 *   com_company_project, event
 *
 * Where "company" is the name of the company,
 *       "project" is the name of the project,
 *       "component" is the name of the project component (which may
 *       include several levels of sub-components, e.g.
 *       ...component_subcomponent_...) where the tracepoint is located
 *       (optional),
 *       "event" is the name of the tracepoint event.
 *
 * the provider:event identifier is limited to 127 characters.
 */

#define TRACEPOINT_EVENT(provider, name, args, fields)			\
	_DECLARE_TRACEPOINT(provider, name, _TP_PARAMS(args))		\
	_DEFINE_TRACEPOINT(provider, name, _TP_PARAMS(args))

#define TRACEPOINT_EVENT_CLASS(provider, name, args, fields)

#define TRACEPOINT_EVENT_INSTANCE(provider, _template, name, args)	\
	_DECLARE_TRACEPOINT(provider, name, _TP_PARAMS(args))		\
	_DEFINE_TRACEPOINT(provider, name, _TP_PARAMS(args))

#endif /* #ifndef TRACEPOINT_EVENT */

#ifndef TRACEPOINT_LOGLEVEL

/*
 * Tracepoint Loglevels
 *
 * Typical use of these loglevels:
 *
 * The loglevels go from 0 to 14. Higher numbers imply the most
 * verbosity (higher event throughput expected.
 *
 * Loglevels 0 through 6, and loglevel 14, match syslog(3) loglevels
 * semantic. Loglevels 7 through 13 offer more fine-grained selection of
 * debug information.
 *
 * TRACE_EMERG           0
 * system is unusable
 *
 * TRACE_ALERT           1
 * action must be taken immediately
 *
 * TRACE_CRIT            2
 * critical conditions
 *
 * TRACE_ERR             3
 * error conditions
 *
 * TRACE_WARNING         4
 * warning conditions
 *
 * TRACE_NOTICE          5
 * normal, but significant, condition
 *
 * TRACE_INFO            6
 * informational message
 *
 * TRACE_DEBUG_SYSTEM    7
 * debug information with system-level scope (set of programs)
 *
 * TRACE_DEBUG_PROGRAM   8
 * debug information with program-level scope (set of processes)
 *
 * TRACE_DEBUG_PROCESS   9
 * debug information with process-level scope (set of modules)
 *
 * TRACE_DEBUG_MODULE    10
 * debug information with module (executable/library) scope (set of units)
 *
 * TRACE_DEBUG_UNIT      11
 * debug information with compilation unit scope (set of functions)
 *
 * TRACE_DEBUG_FUNCTION  12
 * debug information with function-level scope
 *
 * TRACE_DEBUG_LINE      13
 * debug information with line-level scope (TRACEPOINT_EVENT default)
 *
 * TRACE_DEBUG           14
 * debug-level message
 *
 * Declare tracepoint loglevels for tracepoints. A TRACEPOINT_EVENT
 * should be declared prior to the the TRACEPOINT_LOGLEVEL for a given
 * tracepoint name. The first field is the provider name, the second
 * field is the name of the tracepoint, the third field is the loglevel
 * name.
 *
 *      TRACEPOINT_LOGLEVEL(< [com_company_]project[_component] >, < event >,
 *              < loglevel_name >)
 *
 * The TRACEPOINT_PROVIDER must be already declared before declaring a
 * TRACEPOINT_LOGLEVEL.
 */

enum {
	TRACE_EMERG		= 0,
	TRACE_ALERT		= 1,
	TRACE_CRIT		= 2,
	TRACE_ERR		= 3,
	TRACE_WARNING		= 4,
	TRACE_NOTICE		= 5,
	TRACE_INFO		= 6,
	TRACE_DEBUG_SYSTEM	= 7,
	TRACE_DEBUG_PROGRAM	= 8,
	TRACE_DEBUG_PROCESS	= 9,
	TRACE_DEBUG_MODULE	= 10,
	TRACE_DEBUG_UNIT	= 11,
	TRACE_DEBUG_FUNCTION	= 12,
	TRACE_DEBUG_LINE	= 13,
	TRACE_DEBUG		= 14,
};

#define TRACEPOINT_LOGLEVEL(provider, name, loglevel)

#endif /* #ifndef TRACEPOINT_LOGLEVEL */

#ifndef TRACEPOINT_MODEL_EMF_URI

#define TRACEPOINT_MODEL_EMF_URI(provider, name, uri)

#endif /* #ifndef TRACEPOINT_MODEL_EMF_URI */
