#ifndef ASSERTION_CUSTOM
#define ASSERTION_CUSTOM

#include <stddef.h>

#define FSIZE(fixed) (sizeof((fixed)) - 1)
#define FSTRING_SIZE_PARAMS(fixed) (fixed),FSIZE((fixed))

#if defined(__GNUC__) || defined(__clang__)
	#define PRINTF_LIKE(fmt_index, first_arg) \
		__attribute__((format(printf, fmt_index, first_arg)))
#else
	#define PRINTF_LIKE(fmt_index, first_arg)
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
	#define change_assert(assertion, fmt, ...) \
		change_assert_impl((assertion), __FILE__, __LINE__, __func__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#else
	#define change_assert(assertion, fmt, ...) \
		change_assert_impl((assertion), __FILE__, __LINE__, __func__, (fmt), ##__VA_ARGS__)
#endif


void dump_to_file(const char *path, const char *data, size_t len);
_Bool massert(_Bool assertion, const char* message); // message assert
void cassert(_Bool assertion, const char* message); // critical assert
						    //
void change_assert_impl(
	_Bool assertion,
	const char* file,
	int line,
	const char* func,
	const char* fmt,
	...
) PRINTF_LIKE(5, 6);

#endif
