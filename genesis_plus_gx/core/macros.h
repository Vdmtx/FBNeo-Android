#ifndef _MACROS_H_
#define _MACROS_H_

#ifdef LSB_FIRST

#define READ_BYTE(BASE, ADDR) (BASE)[(ADDR)^1]

#define READ_WORD(BASE, ADDR) (((BASE)[ADDR]<<8) | (BASE)[(ADDR)+1])

#define READ_WORD_LONG(BASE, ADDR) (((BASE)[(ADDR)+1]<<24) |      \
                                    ((BASE)[(ADDR)]<<16) |  \
                                    ((BASE)[(ADDR)+3]<<8) |   \
                                    (BASE)[(ADDR)+2])

#define WRITE_BYTE(BASE, ADDR, VAL) (BASE)[(ADDR)^1] = (VAL)&0xff

#define WRITE_WORD(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>8) & 0xff; \
                                      (BASE)[(ADDR)+1] = (VAL)&0xff

#define WRITE_WORD_LONG(BASE, ADDR, VAL) (BASE)[(ADDR+1)] = ((VAL)>>24) & 0xff;    \
                                          (BASE)[(ADDR)] = ((VAL)>>16)&0xff;  \
                                          (BASE)[(ADDR+3)] = ((VAL)>>8)&0xff;   \
                                          (BASE)[(ADDR+2)] = (VAL)&0xff

#else

#define READ_BYTE(BASE, ADDR) (BASE)[ADDR]
#define READ_WORD(BASE, ADDR) *(uint16 *)((BASE) + (ADDR))
#define READ_WORD_LONG(BASE, ADDR) *(uint32 *)((BASE) + (ADDR))
#define WRITE_BYTE(BASE, ADDR, VAL) (BASE)[ADDR] = VAL & 0xff
#define WRITE_WORD(BASE, ADDR, VAL) *(uint16 *)((BASE) + (ADDR)) = VAL & 0xffff
#define WRITE_WORD_LONG(BASE, ADDR, VAL) *(uint32 *)((BASE) + (ADDR)) = VAL & 0xffffffff
#endif

/* C89 compatibility */
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327f
#endif /* M_PI */

/* Set to your compiler's static inline keyword to enable it, or
 * set it to blank to disable it.
 * If you define INLINE in makefile or osd.h, it will override this value.
 * NOTE: not enabling inline functions will SEVERELY slow down emulation.
 */
#ifndef INLINE
#define INLINE static __inline__
#endif /* INLINE */

/* Alignment macros for cross compiler compatibility */
#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#elif defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif

/* Provide the compiler with branch prediction information */
#if defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) x
#define UNLIKELY(x) x
#endif

/* Default CD image file access (read-only) functions */
/* If you need to override default stdio.h functions with custom filesystem API,
   redefine following macros in platform specific include file (osd.h) or Makefile
*/
#ifndef cdStream
#define cdStream            FILE
#define cdStreamOpen(fname) fopen(fname, "rb")
#define cdStreamClose       fclose
#define cdStreamRead        fread
#define cdStreamSeek        fseek
#define cdStreamTell        ftell
#define cdStreamGets        fgets
#endif

#endif /* _MACROS_H_ */
