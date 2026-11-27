#pragma once
#include <limits.h>
#if defined(__GNUC__) && defined(__LONG_LONG_MAX__)
#ifndef LLONG_MAX
#define LLONG_MAX __LONG_LONG_MAX__
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1LL)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX (2ULL * LLONG_MAX + 1ULL)
#endif
#endif
