#ifndef _INLINE_H_
#define _INLINE_H_

// C90 and C99 interprets external linkability of inline function differently.
// Use INLINE and INLINE_IMPL macros to get the same behavior regardless of
// of the C standard used.
// Note: Probably works only with gcc
#ifdef __GNUC_STDC_INLINE__ // C99
#define INLINE inline
#define INLINE_IMPL extern inline
#else // C90
#define INLINE static inline
#define INLINE_IMPL inline
#endif

#endif
