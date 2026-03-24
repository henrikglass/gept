
/**
 * LICENSE:
 *
 * MIT License
 *
 * Copyright (c) 2025 Henrik A. Glass
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * MIT License
 *
 *
 * ABOUT:
 *
 * hgl_string.h implements a dynamic string builder and string view.
 *
 *
 * USAGE:
 *
 * Include hgl_string.h file like this:
 *
 *     #define HGL_STRING_IMPLEMENTATION
 *     #include "hgl_memdbg.h"
 *
 * HGL_STRING_IMPLEMENTATION must only be defined once, in a single compilation unit.
 *
 * hgl_string.h implements two types: HglStringBuilder and HglStringView. HglStringBuilder
 * is a mutable null-terminated string type. HglStringView is an immutable optionally
 * null-terminated string type (although the "immutable" part should be taking with a
 * grain of salt - this is still C).
 *
 * hgl_string.h allows the default allocator, reallocator and free function to be
 * overridden by redefining the following defines before including hgl_string.h:
 *
 *     #define HGL_STRING_ALLOC    malloc
 *     #define HGL_STRING_REALLOC  realloc
 *     #define HGL_STRING_FREE     free
 *
 * Allocator functions may also be overridden per-object in the case of HglStringBuilder
 * by passing them as arguments in the hgl_sb_make() macro, as such:
 *
 *     HglStringBuilder sb = hgl_sb_make(.mem_alloc   = my_alloc,
 *                                       .mem_realloc = my_realloc,
 *                                       .mem_free    = my_free);
 *
 * Allocators not specified in the arguments to `hgl_sb_make` inhertit from the respective
 * HGL_STRING_ALLOC, HGL_STRING_REALLOC, and HGL_STRING_FREE macros. It's is completely
 * legal to specify only one or two of these functions, but some operations may break.
 *
 * EXAMPLE:
 *
 * In this example, we create a string builder, append the contents of a file to the
 * string builder, split the contents by line, and print out the number of occurances
 * of the word "glass" on each line:
 *
 *     HglStringBuilder sb = hgl_sb_make(.initial_capacity = 4096); // alt. only `hgl_sb_make()` is enough.
 *     hgl_sb_append_file(&sb, "assets/glassigt_lyrics.txt");
 *     HglStringView sv = hgl_sv_from_sb(&sb);
 *
 *     int line_nr = 1;
 *     hgl_sv_op_begin(&sv);
 *     HglStringView line = hgl_sv_split_next(&sv, '\n');
 *     while (line.start != NULL) {
 *         int occurances = 0;
 *         hgl_sv_op_begin(&line);
 *         while (hgl_sv_find_next(&line, "glassigt").start != NULL) {
 *             occurances++;
 *         }
 *         printf("line #%d has %d occurances of glassigt.\n", line_nr++, occurances);
 *         line = hgl_sv_split_next(&sv, '\n');
 *     }
 *
 *     hgl_sb_destroy(&sb);
 *
 * See examples/test_string.c for more examples.
 *
 *
 * TODO:
 *     * Don't call hgl_sb_grow_by_policy when not necessary. Check length before.
 *     * sb prepend? Or maybe not?
 *     * sb lchop & rchop? Or maybe not?
 *
 * AUTHOR: Henrik A. Glass
 *
 */

#ifndef HGL_STRING_H
#define HGL_STRING_H

/*--- Include files ---------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <stdlib.h>

/*--- Public macros ---------------------------------------------------------------------*/

#define HGL_SV_LIT(literal) (HglStringView){ .start = literal, .length = sizeof(literal) - 1}
#define HGL_SV_FMT "%.*s"
#define HGL_SV_ARG(sv) (int) (sv).length, (sv).start

#define HGL_SB_FMT "%.*s"
#define HGL_SB_ARG(sb) (int) (sb).length, (sb).cstr

#if !defined(HGL_STRING_ALLOC) &&   \
    !defined(HGL_STRING_REALLOC) && \
    !defined(HGL_STRING_FREE)
#define HGL_STRING_ALLOC malloc
#define HGL_STRING_REALLOC realloc
#define HGL_STRING_FREE free
#endif

/*--- Public type definitions -----------------------------------------------------------*/

typedef enum {
    HGL_SB_GROWTH_POLICY_DOUBLE,
    HGL_SB_GROWTH_POLICY_TO_FIT,
} HglStringGrowthPolicy;

typedef struct {
    size_t initial_capacity;
    void *(*mem_alloc)(size_t);
    void *(*mem_realloc)(void *, size_t);
    void (*mem_free)(void *);
} HglStringBuilderConfig;

/* mutable string type. Owns the underlying `cstr`. */
typedef struct {
    char *cstr;                       /* null-terminated string */
    size_t length;                    /* length of `cstr` excluding null terminator */
    size_t capacity;                  /* total capacity, including null terminator */
    void *(*mem_alloc)(size_t);           /* optional allocator function (Overrides HGL_STRING_ALLOC) */
    void *(*mem_realloc)(void *, size_t); /* optional reallocator function (Overrides HGL_STRING_REALLOC) */
    void (*mem_free)(void *);             /* optional allocator function (Overrides HGL_STRING_FREE) */
} HglStringBuilder;

/* immutable (const) string type. Does not own the underlying `start`. */
typedef struct {
    const char *start;   /* optionally null-terminated string */
    size_t length;       /* length of string view, excluding null terminator */
    size_t it_;          /* gen. purpose iterator for reentrant string view ops. */
} HglStringView;

/*=======================================================================================*/
/*--- String View function prototypes ---------------------------------------------------*/
/*=======================================================================================*/

/**
 * Create a string view of `cstr` with length `length`
 */
HglStringView hgl_sv_from(const char *cstr, size_t length);

/**
 * Create a string view from a string builder `sb`.
 */
HglStringView hgl_sv_from_sb(HglStringBuilder *sb);

/**
 * Create a string view of `cstr` with length `strlen(cstr)`
 */
HglStringView hgl_sv_from_cstr(const char *cstr);

/**
 * Create NULL-terminated `cstr` from `sv`, wrapped inside a HglStringView object. 
 * If `mem_alloc` is not NULL, then it is used to allocate memory for the cstr copy, 
 * otherwise HGL_STRING_ALLOC is used. The caller should ensure that the returned 
 * string is freed correctly after use.
 */
HglStringView hgl_sv_make_copy(HglStringView sv, void *(*mem_alloc)(size_t));

/**
 * Create NULL-terminated `cstr` from `sv`. If `mem_alloc` is not NULL, then
 * it is used to allocate memory for the cstr copy, otherwise HGL_STRING_ALLOC
 * is used. The caller should ensure that the returned string is freed
 * correctly after use.
 */
char *hgl_sv_make_cstr_copy(HglStringView sv, void *(*mem_alloc)(size_t));

/**
 * (Re-)Start a reentrant string view operation (I.e. functions that have "next"
 * functionality, e.g. find, split, find_next_regex).
 */
void hgl_sv_op_begin(HglStringView *sv);

/**
 * Find the next substring when splitting by `delim`. Is reentrant. Restart
 * operation from the beginning by calling `hgl_sv_op_begin(sv)`.
 */
HglStringView hgl_sv_split_next(HglStringView *sv, char delim);

/**
 * Find the next substring that matches `substr`. Is reentrant. Restart
 * operation from the beginning by calling `hgl_sv_op_begin(sv)`.
 */
HglStringView hgl_sv_find_next(HglStringView *sv, const char *substr);

/**
 * Find the next substring that matches the regex `regex`. Is reentrant. Restart
 * operation from the beginning by calling `hgl_sv_op_begin(sv)`.
 */
HglStringView hgl_sv_find_next_regex_match(HglStringView *sv, const char *regex);

/**
 * Returns the substring of `sv` of length `n` starting at offset `offset`
 */
HglStringView hgl_sv_substr(HglStringView sv, size_t offset, size_t n);

/**
 * Chops `n` bytes from the beginning of `sv`.
 */
HglStringView hgl_sv_lchop(HglStringView *sv, size_t n);

/**
 * Chops `n` bytes from the end of `sv`.
 */
HglStringView hgl_sv_rchop(HglStringView *sv, size_t n);

/**
 * Find the next substring when splitting by `delim`. Similar to
 * hgl_sv_split_next, but chops up the original string view. The part of the
 * string to the left of `delim` is returned, and `sv` is modified to contain
 * the part to the right of `delim`. If `sv` does not contain `delim`, the
 * and identical string view to `sv` is returned.
 */
HglStringView hgl_sv_lchop_until(HglStringView *sv, char delim);

/**
 * Find the next substring when splitting by `delim`, from the right. Similar to
 * hgl_sv_split_next, but chops up the original string view. The part of the
 * string to the right of `delim` is returned, and `sv` is modified to contain
 * the part to the right of `delim`. If `sv` does not contain `delim`, the
 * and identical string view to `sv` is returned.
 */
HglStringView hgl_sv_rchop_until(HglStringView *sv, char delim);

/**
 * Find the next substring, splitting by any characted which satisifes `predicate`, from 
 * the left. E.g. `hgl_sv_lchop_until_predicate(&sv, isspace)`.
 */
HglStringView hgl_sv_lchop_until_predicate(HglStringView *sv, int (*predicate)(int c));

/**
 * Find the next substring, splitting by any characted which satisifes `predicate`, from 
 * the right. E.g. `hgl_sv_rchop_until_predicate(&sv, isspace)`.
 */
HglStringView hgl_sv_rchop_until_predicate(HglStringView *sv, int (*predicate)(int c));

/**
 * Chops `n` bytes from the beginning of `sv` where `n` is given by a user supplied
 * function `f` when applied to `sv`. `f` should typcially define a lexer rule for some
 * i user-defined token. If some prefix of `sv` with non-zero length satisfies the lexer
 * rule, then `f` should return the length of that prefix. If no prefix of `sv` satisfies
 * the lexer rule, then `f` should return 0. `f` must always return a number in the range
 * [0, `sv->length`].
 *
 * Example:
 *
 *     size_t lexer_rule_greeting(HglStringView sv) {
 *         if (hgl_sv_starts_with(&sv, "Hello")) return 5;
 *         if (hgl_sv_starts_with(&sv, "Hi")) return 2;
 *         return 0;
 *     }
 *
 *           /.../
 *
 *     HglStringView greeting = hgl_sv_lchop_lexeme(&sv, lexer_rule_greeting);
 */
HglStringView hgl_sv_lchop_lexeme(HglStringView *sv, size_t (*f)(HglStringView));

/**
 * Trims all whitespace from the left.
 */
HglStringView hgl_sv_ltrim(HglStringView sv);

/**
 * Trims all whitespace from the right.
 */
HglStringView hgl_sv_rtrim(HglStringView sv);

/**
 * Trims all whitespace from both the right and the left.
 */
HglStringView hgl_sv_trim(HglStringView sv);

/**
 * Parse the first part of `sv` as an unsigned 64 bit integer.
 */
uint64_t hgl_sv_to_u64(HglStringView sv);

/**
 * Parse the first part of `sv` as a signed 64 bit integer.
 */
int64_t hgl_sv_to_i64(HglStringView sv);

/**
 * Parse the first part of `sv` as a 64 bit float.
 */
double hgl_sv_to_f64(HglStringView sv);

/**
 * Parse the first part of `sv` as an unsigned 64 bit integer, then
 * chop it out.
 */
uint64_t hgl_sv_lchop_u64(HglStringView *sv);

/**
 * Parse the first part of `sv` as a signed 64 bit integer, then
 * chop it out.
 */
int64_t hgl_sv_lchop_i64(HglStringView *sv);

/**
 * Parse the first part of `sv` as a 64 bit float, then chop it out.
 */
double hgl_sv_lchop_f64(HglStringView *sv);

/**
 * Returns true if string view `sv` starts with the substring `substr` and
 * subsequently lchops `strlen(substr)` off of `sv`. If `sv` does not start
 * with `substr`, false is returned and `sv` is left unmodifed.
 */
bool hgl_sv_lchop_if_starts_with(HglStringView *sv, const char *substr);

/**
 * Returns true if `sv` starts with a prefix that satisfies a user-provided
 * lexer rule function `f`. See `hgl_sv_lchop_lexeme` for more information.
 */
bool hgl_sv_starts_with_lexeme(HglStringView *sv, size_t (*f)(HglStringView));

/**
 * Returns true if string view `sv` contains the substring `substr`.
 */
bool hgl_sv_contains(HglStringView *sv, const char *substr);

/**
 * Returns true if string view `sv` starts with the substring `substr`.
 */
bool hgl_sv_starts_with(HglStringView *sv, const char *substr);

/**
 * Returns true if string view `sv` ends with the substring `substr`.
 */
bool hgl_sv_ends_with(HglStringView *sv, const char *substr);

/**
 * Returns 0 if the string views `a` and `b` are equal, -1 if `a` is "less
 * than" `b`, and 1 if `a` is "greater than" `b`. See `man 3 strncmp` for the
 * definition of "less than" and "greater than".
 */
int hgl_sv_compare(HglStringView a, HglStringView b);

/**
 * Returns 0 if the string views `sv` and `cstr` are equal, -1 if `sv` is "less
 * than" `cstr`, and 1 if `sv` is "greater than" `cstr`. See `man 3 strncmp` for the
 * definition of "less than" and "greater than".
 */
int hgl_sv_compare_cstr(HglStringView sv, const char *cstr);

/**
 * Returns true if `a` and `b` are equal.
 */
bool hgl_sv_equals(HglStringView a, HglStringView b);

/**
 * Returns true if `sv` and `cstr` are equal.
 */
bool hgl_sv_equals_cstr(HglStringView sv, const char *cstr);

/*=======================================================================================*/
/*--- String Builder function prototypes ------------------------------------------------*/
/*=======================================================================================*/

/**
 * Creates a new stringbuilder object.
 *
 * With the default configuration (i.e. if no arguments are passed) this creates
 * a stringbuilder with an initial capacity of 64 characters; the allocator
 * functions are inherited from HGL_STRING_ALLOC, HGL_STRING_REALLOC, and
 * HGL_STRING_FREE. The caller may override none, parts of, or all of default
 * configuration by passing them as variadic arguments, e.g.:
 *
 *     HglStringBuilder sb = hgl_sb_make(.initial_capacity = 1024,
 *                                       .mem_alloc        = my_alloc,
 *                                       .mem_realloc      = my_realloc,
 *                                       .mem_free         = my_free);
 */
#define hgl_sb_make(...) hgl_sb_make_((HglStringBuilderConfig){.initial_capacity = 64,                 \
                                                               .mem_alloc        = HGL_STRING_ALLOC,   \
                                                               .mem_realloc      = HGL_STRING_REALLOC, \
                                                               .mem_free         = HGL_STRING_FREE,    \
                                                               __VA_ARGS__})
HglStringBuilder hgl_sb_make_(HglStringBuilderConfig config);

/**
 * Makes a new copy of an existing string builder `sb`.
 */
HglStringBuilder hgl_sb_make_copy(HglStringBuilder *sb);

/**
 * Destroys the string builder `sb`.
 */
void hgl_sb_destroy(HglStringBuilder *sb);

/**
 * Erases the contents of string builder `sb` but keeps the current capacity.
 */
void hgl_sb_clear(HglStringBuilder *sb);

/**
 * Grows `sb` to `new_capacity`. If `new_capacity` is less than the current
 * capacity `sb` is left unchanged.
 */
void hgl_sb_grow(HglStringBuilder *sb, size_t new_capacity);

/**
 * Grows `sb` to at least `needed_capacity`. New size depends on `policy`.
 */
void hgl_sb_grow_by_policy(HglStringBuilder *sb,
                           size_t needed_capacity,
                           HglStringGrowthPolicy policy);

/**
 * Shrinks `sb` to fit, such that `sb->capacity` == `sb->length + 1` (to fit
 * the extra null terminator).
 */
void hgl_sb_shrink_to_fit(HglStringBuilder *sb);

/**
 * Appends `length` bytes of `src` to `sb`.
 */
void hgl_sb_append(HglStringBuilder *sb, const char *src, size_t length);

/**
 * Appends character `c` to `sb`.
 */
void hgl_sb_append_char(HglStringBuilder *sb, char c);

/**
 * Appends string view `sv` to `sb`.
 */
void hgl_sb_append_sv(HglStringBuilder *sb, HglStringView *sv);

/**
 * Appends `strlen(cstr)` bytes of `cstr` to `sb`.
 */
void hgl_sb_append_cstr(HglStringBuilder *sb, const char *cstr);

/**
 * Appends a printf-style formatted string to `sb`:
 */
void hgl_sb_append_fmt(HglStringBuilder *sb, const char *fmt, ...);

/**
 * Appends contents of file at `path` to `sb`.
 */
int hgl_sb_append_file(HglStringBuilder *sb, const char *path);

/**
 * Replaces the section of text specified by `offset` and `length` with `replacement`
 * in string builder `sb`.
 */
void hgl_sb_replace_section(HglStringBuilder *sb,
                            size_t offset,
                            size_t length,
                            const char *replacement);

/**
 * Replaces all instances of `substr` with `replacement`.
 */
void hgl_sb_replace(HglStringBuilder *sb, const char *substr, const char *replacement);

/**
 * Replaces all substrings matching `regex` with `replacement`.
 */
void hgl_sb_replace_regex(HglStringBuilder *sb, const char *regex, const char *replacement);

/**
 * Trims all whitespace from the right.
 */
void hgl_sb_rtrim(HglStringBuilder *sb);

/**
 * Trims all whitespace from the left.
 */
void hgl_sb_ltrim(HglStringBuilder *sb);

/**
 * Trims all whitespace from both the right and the left.
 */
void hgl_sb_trim(HglStringBuilder *sb);

/**
 * Remove's `n` characters from the end of the string.
 */
void hgl_sb_rchop(HglStringBuilder *sb, size_t n);

#endif /* HGL_STRING_H */

/*--- macros ----------------------------------------------------------------------------*/

#ifdef HGL_STRING_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>

/*--- impl. macros ----------------------------------------------------------------------*/

/* CONFIGURABLE: HGL_SB_DEFAULT_GROWTH_POLICY */
#ifndef HGL_SB_DEFAULT_GROWTH_POLICY
#define HGL_SB_DEFAULT_GROWTH_POLICY HGL_SB_GROWTH_POLICY_DOUBLE
#endif

HglStringView hgl_sv_from(const char *cstr, size_t length)
{
    return (HglStringView) {
        .start = cstr,
        .length = length
    };
}

HglStringView hgl_sv_from_sb(HglStringBuilder *sb)
{
    return (HglStringView) {
        .start = sb->cstr,
        .length = sb->length
    };
}

HglStringView hgl_sv_from_cstr(const char *cstr)
{
    return (HglStringView) {
        .start  = cstr,
        .length = strlen(cstr)
    };
}

HglStringView hgl_sv_make_copy(HglStringView sv, void *(*mem_alloc)(size_t))
{
    return (HglStringView) {
        .start  = hgl_sv_make_cstr_copy(sv, mem_alloc),
        .length = sv.length,
    };
}

char *hgl_sv_make_cstr_copy(HglStringView sv, void *(*mem_alloc)(size_t))
{
    char *cstr = (mem_alloc != NULL) ? mem_alloc(sv.length + 1)
                                     : HGL_STRING_ALLOC(sv.length + 1);
    memcpy(cstr, sv.start, sv.length);
    cstr[sv.length] = '\0';
    return cstr;
}

void hgl_sv_op_begin(HglStringView *sv)
{
    sv->it_ = 0;
}

HglStringView hgl_sv_split_next(HglStringView *sv, char delim)
{
    size_t it_at_start = sv->it_;

    /* check if last call walked past end */
    if (sv->it_ >= sv->length) {
        return (HglStringView) {
            .start  = NULL,
            .length = 0
        };
    }

    /* walk along string till we find the next delimiter or we reach the end of the string. */
    while (sv->it_ < sv->length && sv->start[sv->it_] != delim) {
        sv->it_++;
    }

    HglStringView split = {
        .start = &sv->start[it_at_start],
        .length = sv->it_ - it_at_start
    };
    sv->it_++;

    return split;
}

HglStringView hgl_sv_find_next(HglStringView *sv, const char *substr)
{
    size_t len = strlen(substr);

    while (sv->it_ < sv->length) {
        /* walk along string till we find the next character matching the first char in substr. */
        while (sv->it_ < sv->length && sv->start[sv->it_] != substr[0]) {
            sv->it_++;
        }

        /* Check if entire substr matches */
        bool is_match = true;
        size_t i;
        for (i = 0; i < len; i++) {
            /* no more chars in sv */
            if (((sv->it_ + i) >= sv->length)) {
                is_match = false;
                break;
            }

            /* char doesn't match */
            if (sv->start[sv->it_ + i] != substr[i]) {
                is_match = false;
                break;
            }
        }

        sv->it_++;
        if (is_match) {
            return (HglStringView) {
                .start  = &sv->start[sv->it_ - 1],
                .length = len
            };
        }
    }

    /* Walked past end */
    return (HglStringView) {
        .start  = NULL,
        .length = 0
    };
}

HglStringView hgl_sv_find_next_regex_match(HglStringView *sv, const char *regex)
{
    HglStringView match = {0};
    regex_t re;
    regmatch_t rmatch;

    /* assert sv is a view to a null terminated string */
    if (sv->start[sv->length] != '\0') {
        fprintf(stderr, "[hgl_string] ERROR: regex operations require string "
                "view to be null-terminated \"%s\"\n", regex);
        return match;
    }

    /* compile regex */
    /* TODO: cache compiled regexes */
    int err = regcomp(&re, regex, REG_EXTENDED);
    if (err != 0) {
        fprintf(stderr, "[hgl_string] ERROR: Could not compile regex \"%s\"\n", regex);
        return match;
    }

    int ret = regexec(&re, sv->start + sv->it_, 1, &rmatch, 0);
    if (ret != 0) {
        if (ret != REG_NOMATCH) {
            fprintf(stderr, "[hgl_string] ERROR: regexec failed\n");
        }
        regfree(&re);
        return match;
    }
    match.start = sv->start + sv->it_ + rmatch.rm_so;
    match.length = rmatch.rm_eo - rmatch.rm_so;

    regfree(&re);
    sv->it_ += rmatch.rm_so + match.length;
    return match;
}

HglStringView hgl_sv_substr(HglStringView sv, size_t offset, size_t n)
{
    if ((offset + n) > sv.length) {
        n = sv.length - offset;
    }

    return (HglStringView) {
        .start  = &sv.start[offset],
        .length = n,
        .it_    = 0,
    };
}

HglStringView hgl_sv_lchop(HglStringView *sv, size_t n)
{
    if (n > sv->length) {
        n = sv->length;
    }

    HglStringView left_part = (HglStringView) {
        .start  = sv->start,
        .length = n,
        .it_    = 0,
    };

    sv->start  += n;
    sv->length -= n;
    sv->it_     = 0;

    return left_part;
}

HglStringView hgl_sv_rchop(HglStringView *sv, size_t n)
{
    if (n > sv->length) {
        n = sv->length;
    }

    HglStringView right_part = (HglStringView) {
        .start  = sv->start + sv->length - n,
        .length = n,
        .it_    = 0,
    };

    sv->length -= n;
    sv->it_     = 0;

    return right_part;
}

HglStringView hgl_sv_lchop_until(HglStringView *sv, char delim)
{
    size_t i;
    for (i = 0; i < sv->length; i++) {
        if (sv->start[i] == delim) {
            break;
        }
    }

    HglStringView left_part = (HglStringView) {
        .start  = sv->start,
        .length = i,
        .it_    = 0,
    };

    sv->start  += i + 1;
    sv->length -= (i + 1 > sv->length) ? sv->length : i + 1;
    sv->it_     = 0;

    return left_part;
}

HglStringView hgl_sv_rchop_until(HglStringView *sv, char delim)
{
    size_t i;
    for (i = sv->length - 1; i > 0; i--) {
        if (sv->start[i] == delim) {
            break;
        }
    }

    HglStringView right_part = (HglStringView) {
        .start  = sv->start + i + 1,
        .length = sv->length - i - 1,
        .it_    = 0,
    };

    sv->length = i;
    sv->it_    = 0;

    return right_part;
}

HglStringView hgl_sv_lchop_until_predicate(HglStringView *sv, int (*predicate)(int c))
{
    size_t i;
    for (i = 0; i < sv->length; i++) {
        if (predicate(sv->start[i]) != 0) {
            break;
        }
    }

    HglStringView left_part = (HglStringView) {
        .start  = sv->start,
        .length = i,
        .it_    = 0,
    };

    sv->start  += i + 1;
    sv->length -= (i + 1 > sv->length) ? sv->length : i + 1;
    sv->it_     = 0;

    return left_part;
}

HglStringView hgl_sv_rchop_until_predicate(HglStringView *sv, int (*predicate)(int c))
{
    size_t i;
    for (i = sv->length - 1; i > 0; i--) {
        if (predicate(sv->start[i])) {
            break;
        }
    }

    HglStringView right_part = (HglStringView) {
        .start  = sv->start + i + 1,
        .length = sv->length - i - 1,
        .it_    = 0,
    };

    sv->length = i;
    sv->it_    = 0;

    return right_part;
}

HglStringView hgl_sv_lchop_lexeme(HglStringView *sv, size_t (*f)(HglStringView))
{
    size_t n = f(*sv);
    return hgl_sv_lchop(sv, n);
}

HglStringView hgl_sv_ltrim(HglStringView sv)
{
    size_t i;
    for (i = 0; i < sv.length; i++) {
        if (!isspace(sv.start[i])) {
            break;
        }
    }

    return (HglStringView) {
        .start = sv.start + i,
        .length = sv.length - i,
    };
}

HglStringView hgl_sv_rtrim(HglStringView sv)
{
    size_t i;
    for (i = sv.length - 1; i > 0; i--) {
        if (!isspace(sv.start[i])) {
            break;
        }
    }

    return (HglStringView) {
        .start = sv.start,
        .length = i + 1,
    };
}

HglStringView hgl_sv_trim(HglStringView sv)
{
    return hgl_sv_rtrim(hgl_sv_ltrim(sv));
}

uint64_t hgl_sv_to_u64(HglStringView sv)
{
    char temp[64];
    size_t n = (sv.length < 63) ? sv.length : 63;
    memcpy(temp, sv.start, n);
    temp[n] = '\0';
    return strtoul(temp, NULL, 0);
}

int64_t hgl_sv_to_i64(HglStringView sv)
{
    char temp[64];
    size_t n = (sv.length < 63) ? sv.length : 63;
    memcpy(temp, sv.start, n);
    temp[n] = '\0';
    return strtol(temp, NULL, 0);
}

double hgl_sv_to_f64(HglStringView sv)
{
    char temp[64];
    size_t n = (sv.length < 63) ? sv.length : 63;
    memcpy(temp, sv.start, n);
    temp[n] = '\0';
    return strtod(temp, NULL);
}

uint64_t hgl_sv_lchop_u64(HglStringView *sv)
{
    char temp[64];
    char *end;
    size_t n = (sv->length < 63) ? sv->length : 63;
    memcpy(temp, sv->start, n);
    temp[n] = '\0';
    uint64_t value = strtoul(temp, &end, 0);
    hgl_sv_lchop(sv, end - temp);
    return value;
}

int64_t hgl_sv_lchop_i64(HglStringView *sv)
{
    char temp[64];
    char *end;
    size_t n = (sv->length < 63) ? sv->length : 63;
    memcpy(temp, sv->start, n);
    temp[n] = '\0';
    int64_t value = strtol(temp, &end, 0);
    hgl_sv_lchop(sv, end - temp);
    return value;
}

double hgl_sv_lchop_f64(HglStringView *sv)
{
    char temp[64];
    char *end;
    size_t n = (sv->length < 63) ? sv->length : 63;
    memcpy(temp, sv->start, n);
    temp[n] = '\0';
    double value = strtod(temp, &end);
    hgl_sv_lchop(sv, end - temp);
    return value;
}

bool hgl_sv_lchop_if_starts_with(HglStringView *sv, const char *substr)
{
    if (hgl_sv_starts_with(sv, substr)) {
        hgl_sv_lchop(sv, strlen(substr));
        return true;
    }

    return false;
}

bool hgl_sv_starts_with_lexeme(HglStringView *sv, size_t (*f)(HglStringView))
{
    return 0 != f(*sv);
}

bool hgl_sv_contains(HglStringView *sv, const char *substr)
{
    hgl_sv_op_begin(sv);
    HglStringView match = hgl_sv_find_next(sv, substr);
    return (match.length != 0);
}

bool hgl_sv_starts_with(HglStringView *sv, const char *substr)
{
    hgl_sv_op_begin(sv);
    HglStringView match = hgl_sv_find_next(sv, substr);
    return (match.length != 0) && (match.start == sv->start);
}

bool hgl_sv_ends_with(HglStringView *sv, const char *substr)
{
    hgl_sv_op_begin(sv);
    HglStringView match = hgl_sv_find_next(sv, substr);
    return (match.length != 0) && ((match.start + match.length) == (sv->start + sv->length));
}

int hgl_sv_compare(HglStringView a, HglStringView b)
{
    if (a.length < b.length) return -1;
    if (a.length > b.length) return  1;
    return strncmp(a.start, b.start, a.length);
}

int hgl_sv_compare_cstr(HglStringView sv, const char *cstr)
{
    HglStringView other = hgl_sv_from_cstr(cstr);
    return hgl_sv_compare(sv, other);
}

bool hgl_sv_equals(HglStringView a, HglStringView b)
{
    return 0 == hgl_sv_compare(a, b);
}

bool hgl_sv_equals_cstr(HglStringView sv, const char *cstr)
{
    return 0 == hgl_sv_compare_cstr(sv, cstr);
}

HglStringBuilder hgl_sb_make_(HglStringBuilderConfig config)
{
    assert(config.initial_capacity > 0);

    HglStringBuilder sb = {
        .cstr         = config.mem_alloc(config.initial_capacity),
        .length       = 0,
        .capacity     = config.initial_capacity,
        .mem_alloc    = config.mem_alloc,
        .mem_realloc  = config.mem_realloc,
        .mem_free     = config.mem_free,
    };

    sb.cstr[0] = '\0';
    return sb;
}

HglStringBuilder hgl_sb_make_copy(HglStringBuilder *sb)
{
    HglStringBuilder copy;
    copy.length       = sb->length,
    copy.capacity     = sb->capacity,
    copy.mem_alloc    = sb->mem_alloc;
    copy.mem_realloc  = sb->mem_realloc;
    copy.mem_free     = sb->mem_free;
    copy.cstr         = sb->mem_alloc(sb->capacity),
    memcpy(copy.cstr, sb->cstr, sb->length);
    return copy;
}

void hgl_sb_destroy(HglStringBuilder *sb)
{
    sb->mem_free(sb->cstr);
    sb->cstr     = NULL;
    sb->length   = 0;
    sb->capacity = 0;
}

void hgl_sb_clear(HglStringBuilder *sb)
{
    sb->length = 0;
    sb->cstr[0] = '\0';
}

void hgl_sb_grow(HglStringBuilder *sb, size_t new_capacity)
{
    if (new_capacity < sb->capacity) {
        return;
    }

    sb->cstr = sb->mem_realloc(sb->cstr, new_capacity);
    sb->capacity = new_capacity;
}

void hgl_sb_grow_by_policy(HglStringBuilder *sb,
                           size_t needed_capacity,
                           HglStringGrowthPolicy policy)
{
    if (needed_capacity < sb->capacity) {
        return;
    }

    size_t new_capacity = 0;
    switch (policy) {
        case HGL_SB_GROWTH_POLICY_TO_FIT: {
            new_capacity = needed_capacity;
        } break;
        case HGL_SB_GROWTH_POLICY_DOUBLE: {
            new_capacity = 2*sb->capacity;
            while (new_capacity < needed_capacity) {
                new_capacity *= 2;
            }
        } break;
        default: assert(0 && "unreachable"); break;
    }

    sb->cstr = sb->mem_realloc(sb->cstr, new_capacity);
    sb->capacity = new_capacity;
}

void hgl_sb_shrink_to_fit(HglStringBuilder *sb)
{
    if ((sb->length + 1) == sb->capacity) {
        return;
    }

    sb->capacity = sb->length + 1;
    sb->cstr = sb->mem_realloc(sb->cstr, sb->capacity);
}

void hgl_sb_append(HglStringBuilder *sb, const char *src, size_t length)
{
    if (length == 0) {
        return;
    }

    hgl_sb_grow_by_policy(sb, sb->length + length + 1,
                          HGL_SB_DEFAULT_GROWTH_POLICY);

    if ((src > sb->cstr) && (src < sb->cstr + sb->length)) {
        memmove(sb->cstr + sb->length, src, length);
    } else {
        memcpy(sb->cstr + sb->length, src, length);
    }
    sb->length = sb->length + length;
    sb->cstr[sb->length] = '\0';
}

void hgl_sb_append_char(HglStringBuilder *sb, char c)
{
    hgl_sb_grow_by_policy(sb, sb->length + 2,
                          HGL_SB_DEFAULT_GROWTH_POLICY);

    sb->length++;
    sb->cstr[sb->length - 1] = c;
    sb->cstr[sb->length] = '\0';
}

void hgl_sb_append_sv(HglStringBuilder *sb, HglStringView *sv)
{
    hgl_sb_append(sb, sv->start, sv->length);
}

void hgl_sb_append_cstr(HglStringBuilder *sb, const char *cstr)
{
    hgl_sb_append(sb, cstr, strlen(cstr));
}

void hgl_sb_append_fmt(HglStringBuilder *sb, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int length = vsnprintf(NULL, 0, fmt, args);
    if (length == 0) {
        va_end(args);
        return;
    }

    hgl_sb_grow_by_policy(sb, sb->length + length + 1,
                          HGL_SB_DEFAULT_GROWTH_POLICY);

    va_start(args, fmt);
    vsnprintf(&sb->cstr[sb->length], length + 1, fmt, args);
    sb->length = sb->length + length;
    sb->cstr[sb->length] = '\0'; // not really necessary

    va_end(args);
}

int hgl_sb_append_file(HglStringBuilder *sb, const char *path)
{
    /* open file */
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[hgl_string] ERROR: Could not open file %s. errno=%s\n",
                path, strerror(errno));
        return -1;
    }

    /* get file size */
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
    assert(fsize >= 0);

    /* grow sb if necessary */
    size_t needed_capacity = sb->length + fsize + 1;
    hgl_sb_grow_by_policy(sb, needed_capacity,
                          HGL_SB_DEFAULT_GROWTH_POLICY);

    /* read file into sb, update length, write null byte */
    assert(fread(sb->cstr + sb->length, fsize, 1, fp) == 1);
    sb->length += fsize;
    sb->cstr[sb->length] = '\0';

    fclose(fp);
    return 0;
}

void hgl_sb_replace_section(HglStringBuilder *sb,
                            size_t offset,
                            size_t length,
                            const char *replacement)
{
    size_t repl_length = strlen(replacement);
    int len_diff = repl_length - length;

    if (len_diff < 0 ) {
        /* replacement is smaller */
        /* copy replacement into substr */
        memcpy(sb->cstr + offset, replacement, repl_length);

        /* shift remaining string "left" */
        memmove(sb->cstr + offset + repl_length,
                sb->cstr + offset + length,
                sb->length - offset - length);

    } else if (len_diff > 0) {
        /* replacement is bigger */
        /* grow if necessary */
        hgl_sb_grow_by_policy(sb, sb->length + len_diff + 1,
                              HGL_SB_DEFAULT_GROWTH_POLICY);

        /* shift remaining string "right". */
        memmove(sb->cstr + offset + repl_length,
                sb->cstr + offset + length,
                sb->length - offset - length);

        /* copy replacement into substr */
        memcpy(sb->cstr + offset, replacement, repl_length);

    } else {
        /* replacement is of same length, just overwrite */
        memcpy(sb->cstr + offset, replacement, repl_length);
    }

    sb->length += len_diff;
    sb->cstr[sb->length] = '\0';
}

void hgl_sb_replace(HglStringBuilder *sb, const char *substr, const char *replacement)
{
    const size_t repl_len = strlen(replacement);

    HglStringView sv = hgl_sv_from(sb->cstr, sb->length);
    hgl_sv_op_begin(&sv);
    HglStringView match = hgl_sv_find_next(&sv, substr);

    while (match.length != 0 && match.start != NULL) {
        hgl_sb_replace_section(sb, match.start - sb->cstr, match.length, replacement);
        sv.start = sb->cstr; // in case replacement invalidates sv pointer.
        sv.length = sb->length;
        sv.it_ += repl_len - 1; // move iterator to after replament to avoid infinite
                                // replacement loops whenever `replacement` itself is
                                // a substring of `substr`.

        /* find next match */
        match = hgl_sv_find_next(&sv, substr);
    }
}

void hgl_sb_replace_regex(HglStringBuilder *sb, const char *regex, const char *replacement)
{
    const size_t repl_len = strlen(replacement);

    HglStringView sv = hgl_sv_from(sb->cstr, sb->length);
    hgl_sv_op_begin(&sv);
    HglStringView match = hgl_sv_find_next_regex_match(&sv, regex);

    while (match.length != 0 && match.start != NULL) {
        hgl_sb_replace_section(sb, match.start - sb->cstr, match.length, replacement);
        sv.start = sb->cstr; // in case replacement invalidates sv pointer.
        sv.length = sb->length;
        sv.it_ += repl_len - match.length; // move iterator to after replament to avoid infinite
                                           // replacement loops whenever `replacement` itself is
                                           // a substring of `substr`.

        /* find next match */
        match = hgl_sv_find_next_regex_match(&sv, regex);
    }
}

void hgl_sb_rtrim(HglStringBuilder *sb)
{
    size_t last = sb->length - 1;
    while ((last > 0) && isspace(sb->cstr[last])) {
        last--;
    }
    size_t new_len = last + 1;
    sb->cstr[new_len] = '\0';
    sb->length = new_len;
}

void hgl_sb_ltrim(HglStringBuilder *sb)
{
    size_t i = 0;
    while ((i < sb->length) && isspace(sb->cstr[i])) {
        i++;
    }
    size_t new_len = sb->length - i;
    if (sb->cstr != &sb->cstr[i]) {
        memmove(sb->cstr, &sb->cstr[i], new_len);
        sb->cstr[new_len] = '\0';
        sb->length = new_len;
    }
}

void hgl_sb_trim(HglStringBuilder *sb)
{
    hgl_sb_rtrim(sb);
    hgl_sb_ltrim(sb);
}

void hgl_sb_rchop(HglStringBuilder *sb, size_t n)
{
    if (n > sb->length) {
        n = sb->length;
    }
    sb->length -= n;
    sb->cstr[sb->length + 1] = '\0';
}

#endif

#ifdef HGL_STRING_STRIP_PREFIX

typedef HglStringView StringView;
typedef HglStringBuilder StringBuilder;

#define SV_LIT HGL_SV_LIT
#define SV_FMT HGL_SV_FMT
#define SV_ARG HGL_SV_ARG
#define SB_FMT HGL_SB_FMT
#define SB_ARG HGL_SB_ARG

#define sv_from                  hgl_sv_from
#define sv_from_sb               hgl_sv_from_sb
#define sv_from_cstr             hgl_sv_from_cstr
#define sv_make_copy             hgl_sv_make_copy
#define sv_make_cstr_copy        hgl_sv_make_cstr_copy
#define sv_op_begin              hgl_sv_op_begin
#define sv_split_next            hgl_sv_split_next
#define sv_find_next             hgl_sv_find_next
#define sv_find_next_regex_match hgl_sv_find_next_regex_match
#define sv_substr                hgl_sv_substr
#define sv_lchop                 hgl_sv_lchop
#define sv_rchop                 hgl_sv_rchop
#define sv_lchop_until           hgl_sv_lchop_until
#define sv_rchop_until           hgl_sv_rchop_until
#define sv_lchop_lexeme          hgl_sv_lchop_lexeme
#define sv_ltrim                 hgl_sv_ltrim
#define sv_rtrim                 hgl_sv_rtrim
#define sv_trim                  hgl_sv_trim
#define sv_to_u64                hgl_sv_to_u64
#define sv_to_i64                hgl_sv_to_i64
#define sv_to_f64                hgl_sv_to_f64
#define sv_lchop_u64             hgl_sv_lchop_u64
#define sv_lchop_i64             hgl_sv_lchop_i64
#define sv_lchop_f64             hgl_sv_lchop_f64
#define sv_lchop_if_starts_with  hgl_sv_lchop_if_starts_with
#define sv_starts_with_lexeme    hgl_sv_starts_with_lexeme
#define sv_contains              hgl_sv_contains
#define sv_starts_with           hgl_sv_starts_with
#define sv_ends_with             hgl_sv_ends_with
#define sv_compare               hgl_sv_compare
#define sv_compare_cstr          hgl_sv_compare_cstr
#define sv_equals                hgl_sv_equals
#define sv_equals_cstr           hgl_sv_equals_cstr

#define sb_make                  hgl_sb_make
#define sb_make_copy             hgl_sb_make_copy
#define sb_destroy               hgl_sb_destroy
#define sb_clear                 hgl_sb_clear
#define sb_grow                  hgl_sb_grow
#define sb_grow_by_policy        hgl_sb_grow_by_policy
#define sb_shrink_to_fit         hgl_sb_shrink_to_fit
#define sb_append                hgl_sb_append
#define sb_append_char           hgl_sb_append_char
#define sb_append_sv             hgl_sb_append_sv
#define sb_append_cstr           hgl_sb_append_cstr
#define sb_append_fmt            hgl_sb_append_fmt
#define sb_append_file           hgl_sb_append_file
#define sb_replace_section       hgl_sb_replace_section
#define sb_replace               hgl_sb_replace
#define sb_replace_regex         hgl_sb_replace_regex
#define sb_rtrim                 hgl_sb_rtrim
#define sb_ltrim                 hgl_sb_ltrim
#define sb_trim                  hgl_sb_trim
#define sb_rchop                 hgl_sb_rchop

#endif

