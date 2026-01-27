#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include "runtime.h"
#ifdef _WIN32
#undef min
#undef max
#endif


/* Closure type for lambdas */
typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;

/* Native struct forward declarations */
typedef struct RtTextFile RtTextFile;
typedef struct RtBinaryFile RtBinaryFile;
typedef struct RtDate RtDate;
typedef struct RtTime RtTime;

/* Struct method forward declarations */
RtTextFile * __sn__TextFile_open(RtManagedArena *__caller_arena__, RtHandle __sn__path);
bool __sn__TextFile_exists(RtManagedArena *__caller_arena__, RtHandle __sn__path);
RtHandle __sn__TextFile_readAll(RtManagedArena *__caller_arena__, RtHandle __sn__path);
void __sn__TextFile_writeAll(RtManagedArena *__caller_arena__, RtHandle __sn__path, RtHandle __sn__content);
void __sn__TextFile_copy(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest);
void __sn__TextFile_move(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest);
void __sn__TextFile_delete(RtManagedArena *__caller_arena__, RtHandle __sn__path);
extern long long sn_text_file_read_char(RtTextFile *);
RtHandle __sn__TextFile_readLine(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
RtHandle __sn__TextFile_readRemaining(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
RtHandle __sn__TextFile_readLines(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
RtHandle __sn__TextFile_readWord(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
extern void sn_text_file_write_char(RtTextFile *, long long);
extern void sn_text_file_write(RtTextFile *, const char *);
extern void sn_text_file_write_line(RtTextFile *, const char *);
extern void sn_text_file_print(RtTextFile *, const char *);
extern void sn_text_file_println(RtTextFile *, const char *);
extern bool sn_text_file_is_eof(RtTextFile *);
extern bool sn_text_file_has_chars(RtTextFile *);
extern bool sn_text_file_has_words(RtTextFile *);
extern bool sn_text_file_has_lines(RtTextFile *);
extern long long sn_text_file_position(RtTextFile *);
extern void sn_text_file_seek(RtTextFile *, long long);
extern void sn_text_file_rewind(RtTextFile *);
extern void sn_text_file_flush(RtTextFile *);
extern void sn_text_file_close(RtTextFile *);
RtHandle __sn__TextFile_path(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
RtHandle __sn__TextFile_name(RtManagedArena *__caller_arena__, RtTextFile *__sn__self);
extern long long sn_text_file_get_size(RtTextFile *);
RtBinaryFile * __sn__BinaryFile_open(RtManagedArena *__caller_arena__, RtHandle __sn__path);
bool __sn__BinaryFile_exists(RtManagedArena *__caller_arena__, RtHandle __sn__path);
RtHandle __sn__BinaryFile_readAll(RtManagedArena *__caller_arena__, RtHandle __sn__path);
void __sn__BinaryFile_writeAll(RtManagedArena *__caller_arena__, RtHandle __sn__path, RtHandle __sn__data);
void __sn__BinaryFile_copy(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest);
void __sn__BinaryFile_move(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest);
void __sn__BinaryFile_delete(RtManagedArena *__caller_arena__, RtHandle __sn__path);
extern long long sn_binary_file_read_byte(RtBinaryFile *);
RtHandle __sn__BinaryFile_readBytes(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self, long long __sn__count);
RtHandle __sn__BinaryFile_readRemaining(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self);
extern long long sn_binary_file_read_into(RtBinaryFile *, unsigned char *);
extern void sn_binary_file_write_byte(RtBinaryFile *, long long);
extern void sn_binary_file_write_bytes(RtBinaryFile *, unsigned char *);
extern bool sn_binary_file_is_eof(RtBinaryFile *);
extern bool sn_binary_file_has_bytes(RtBinaryFile *);
extern long long sn_binary_file_position(RtBinaryFile *);
extern void sn_binary_file_seek(RtBinaryFile *, long long);
extern void sn_binary_file_rewind(RtBinaryFile *);
extern void sn_binary_file_flush(RtBinaryFile *);
extern void sn_binary_file_close(RtBinaryFile *);
RtHandle __sn__BinaryFile_path(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self);
RtHandle __sn__BinaryFile_name(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self);
extern long long sn_binary_file_get_size(RtBinaryFile *);
RtDate * __sn__Date_today(RtManagedArena *__caller_arena__);
RtDate * __sn__Date_fromYmd(RtManagedArena *__caller_arena__, long long __sn__year, long long __sn__month, long long __sn__day);
RtDate * __sn__Date_fromString(RtManagedArena *__caller_arena__, RtHandle __sn__s);
RtDate * __sn__Date_fromEpochDays(RtManagedArena *__caller_arena__, long long __sn__days);
bool __sn__Date_isLeapYear(RtManagedArena *__caller_arena__, long long __sn__year);
long long __sn__Date_daysInMonth(RtManagedArena *__caller_arena__, long long __sn__year, long long __sn__month);
extern long long sn_date_get_year(RtDate *);
extern long long sn_date_get_month(RtDate *);
extern long long sn_date_get_day(RtDate *);
extern long long sn_date_get_weekday(RtDate *);
extern long long sn_date_get_day_of_year(RtDate *);
extern long long sn_date_get_epoch_days(RtDate *);
extern long long sn_date_get_days_in_month(RtDate *);
extern bool sn_date_is_leap(RtDate *);
extern bool sn_date_is_weekend(RtDate *);
extern bool sn_date_is_weekday(RtDate *);
extern bool sn_date_is_before(RtDate *, RtDate *);
extern bool sn_date_is_after(RtDate *, RtDate *);
extern bool sn_date_equals(RtDate *, RtDate *);
extern long long sn_date_diff_days(RtDate *, RtDate *);
RtHandle __sn__Date_format(RtManagedArena *__caller_arena__, RtDate *__sn__self, RtHandle __sn__pattern);
RtHandle __sn__Date_toIso(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtHandle __sn__Date_toString(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtDate * __sn__Date_addDays(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__days);
RtDate * __sn__Date_addWeeks(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__weeks);
RtDate * __sn__Date_addMonths(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__months);
RtDate * __sn__Date_addYears(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__years);
RtDate * __sn__Date_startOfMonth(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtDate * __sn__Date_endOfMonth(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtDate * __sn__Date_startOfYear(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtDate * __sn__Date_endOfYear(RtManagedArena *__caller_arena__, RtDate *__sn__self);
void* __sn__Date_toTime(RtManagedArena *__caller_arena__, RtDate *__sn__self);
RtTime * __sn__Time_now(RtManagedArena *__caller_arena__);
RtTime * __sn__Time_utc(RtManagedArena *__caller_arena__);
RtTime * __sn__Time_fromMillis(RtManagedArena *__caller_arena__, long long __sn__ms);
RtTime * __sn__Time_fromSeconds(RtManagedArena *__caller_arena__, long long __sn__s);
void __sn__Time_sleep(RtManagedArena *__caller_arena__, long long __sn__ms);
extern long long sn_time_get_millis(RtTime *);
extern long long sn_time_get_seconds(RtTime *);
extern long long sn_time_get_year(RtTime *);
extern long long sn_time_get_month(RtTime *);
extern long long sn_time_get_day(RtTime *);
extern long long sn_time_get_hour(RtTime *);
extern long long sn_time_get_minute(RtTime *);
extern long long sn_time_get_second(RtTime *);
extern long long sn_time_get_weekday(RtTime *);
extern bool sn_time_is_before(RtTime *, RtTime *);
extern bool sn_time_is_after(RtTime *, RtTime *);
extern bool sn_time_equals(RtTime *, RtTime *);
RtHandle __sn__Time_format(RtManagedArena *__caller_arena__, RtTime *__sn__self, RtHandle __sn__pattern);
RtHandle __sn__Time_toIso(RtManagedArena *__caller_arena__, RtTime *__sn__self);
RtHandle __sn__Time_toDate(RtManagedArena *__caller_arena__, RtTime *__sn__self);
RtHandle __sn__Time_toTime(RtManagedArena *__caller_arena__, RtTime *__sn__self);
RtTime * __sn__Time_add(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__ms);
RtTime * __sn__Time_addSeconds(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__seconds);
RtTime * __sn__Time_addMinutes(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__minutes);
RtTime * __sn__Time_addHours(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__hours);
RtTime * __sn__Time_addDays(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__days);
long long __sn__Time_diff(RtManagedArena *__caller_arena__, RtTime *__sn__self, RtTime * __sn__other);

/* Native function extern declarations */
extern RtTextFile * sn_text_file_open(RtManagedArena *, const char *);
extern long long sn_text_file_exists(const char *);
extern RtHandle sn_text_file_read_all_static(RtManagedArena *, const char *);
extern void sn_text_file_write_all_static(const char *, const char *);
extern void sn_text_file_copy(const char *, const char *);
extern void sn_text_file_move(const char *, const char *);
extern void sn_text_file_delete(const char *);
extern RtHandle sn_text_file_read_line(RtManagedArena *, RtTextFile *);
extern RtHandle sn_text_file_read_remaining(RtManagedArena *, RtTextFile *);
extern RtHandle sn_text_file_read_lines(RtManagedArena *, RtTextFile *);
extern RtHandle sn_text_file_read_word(RtManagedArena *, RtTextFile *);
extern RtHandle sn_text_file_get_path(RtManagedArena *, RtTextFile *);
extern RtHandle sn_text_file_get_name(RtManagedArena *, RtTextFile *);
extern RtBinaryFile * sn_binary_file_open(RtManagedArena *, const char *);
extern long long sn_binary_file_exists(const char *);
extern RtHandle sn_binary_file_read_all_static(RtManagedArena *, const char *);
extern void sn_binary_file_write_all_static(const char *, unsigned char *);
extern void sn_binary_file_copy(const char *, const char *);
extern void sn_binary_file_move(const char *, const char *);
extern void sn_binary_file_delete(const char *);
extern RtHandle sn_binary_file_read_bytes(RtManagedArena *, RtBinaryFile *, long long);
extern RtHandle sn_binary_file_read_remaining(RtManagedArena *, RtBinaryFile *);
extern RtHandle sn_binary_file_get_path(RtManagedArena *, RtBinaryFile *);
extern RtHandle sn_binary_file_get_name(RtManagedArena *, RtBinaryFile *);
extern RtDate * sn_date_today(RtManagedArena *);
extern RtDate * sn_date_from_ymd(RtManagedArena *, long long, long long, long long);
extern RtDate * sn_date_from_string(RtManagedArena *, const char *);
extern RtDate * sn_date_from_epoch_days(RtManagedArena *, long long);
extern RtHandle sn_date_format(RtManagedArena *, RtDate *, const char *);
extern RtHandle sn_date_to_iso(RtManagedArena *, RtDate *);
extern RtHandle sn_date_to_string(RtManagedArena *, RtDate *);
extern RtDate * sn_date_add_days(RtManagedArena *, RtDate *, long long);
extern RtDate * sn_date_add_weeks(RtManagedArena *, RtDate *, long long);
extern RtDate * sn_date_add_months(RtManagedArena *, RtDate *, long long);
extern RtDate * sn_date_add_years(RtManagedArena *, RtDate *, long long);
extern RtDate * sn_date_start_of_month(RtManagedArena *, RtDate *);
extern RtDate * sn_date_end_of_month(RtManagedArena *, RtDate *);
extern RtDate * sn_date_start_of_year(RtManagedArena *, RtDate *);
extern RtDate * sn_date_end_of_year(RtManagedArena *, RtDate *);
extern void* sn_date_to_time(RtManagedArena *, RtDate *);
extern long long sn_date_is_leap_year(long long);
extern long long sn_date_days_in_month(long long, long long);
extern RtTime * sn_time_now(RtManagedArena *);
extern RtTime * sn_time_utc(RtManagedArena *);
extern RtTime * sn_time_from_millis(RtManagedArena *, long long);
extern RtTime * sn_time_from_seconds(RtManagedArena *, long long);
extern void sn_time_sleep(long long);
extern RtHandle sn_time_format(RtManagedArena *, RtTime *, const char *);
extern RtHandle sn_time_to_iso(RtManagedArena *, RtTime *);
extern RtHandle sn_time_to_date(RtManagedArena *, RtTime *);
extern RtHandle sn_time_to_time(RtManagedArena *, RtTime *);
extern RtTime * sn_time_add(RtManagedArena *, RtTime *, long long);
extern RtTime * sn_time_add_seconds(RtManagedArena *, RtTime *, long long);
extern RtTime * sn_time_add_minutes(RtManagedArena *, RtTime *, long long);
extern RtTime * sn_time_add_hours(RtManagedArena *, RtTime *, long long);
extern RtTime * sn_time_add_days(RtManagedArena *, RtTime *, long long);
extern long long sn_time_diff(RtTime *, RtTime *);
extern long long sn_time_get_millis(RtTime *);
extern long long sn_time_get_seconds(RtTime *);
extern long long sn_time_get_year(RtTime *);
extern long long sn_time_get_month(RtTime *);
extern long long sn_time_get_day(RtTime *);
extern long long sn_time_get_hour(RtTime *);
extern long long sn_time_get_minute(RtTime *);
extern long long sn_time_get_second(RtTime *);
extern long long sn_time_get_weekday(RtTime *);
extern bool sn_time_is_before(RtTime *, RtTime *);
extern bool sn_time_is_after(RtTime *, RtTime *);
extern bool sn_time_equals(RtTime *, RtTime *);

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

void __sn__demo_types(RtManagedArena *);
void __sn__show_integers(RtManagedArena *);
void __sn__show_doubles(RtManagedArena *);
void __sn__show_strings(RtManagedArena *);
void __sn__show_chars(RtManagedArena *);
void __sn__show_booleans(RtManagedArena *);
void __sn__show_type_conversion(RtManagedArena *);
void __sn__demo_loops(RtManagedArena *);
void __sn__show_while_loops(RtManagedArena *);
void __sn__show_for_loops(RtManagedArena *);
void __sn__show_foreach_loops(RtManagedArena *);
void __sn__show_break_continue(RtManagedArena *);
void __sn__show_nested_loops(RtManagedArena *);
void __sn__demo_conditionals(RtManagedArena *);
void __sn__demo_strings(RtManagedArena *);
void __sn__demo_functions(RtManagedArena *);
void __sn__greet(RtManagedArena *);
void __sn__greet_person(RtManagedArena *, RtHandle);
void __sn__print_sum(RtManagedArena *, long long, long long);
void __sn__demo_arrays(RtManagedArena *);
void __sn__demo_lambda(RtManagedArena *);
void __sn__demo_closure(RtManagedArena *);
void __sn__demo_bytes(RtManagedArena *);
void __sn__show_byte_basics(RtManagedArena *);
void __sn__show_byte_values(RtManagedArena *);
void __sn__show_byte_conversions(RtManagedArena *);
void __sn__show_byte_arrays(RtManagedArena *);
void __sn__demo_fileio(RtManagedArena *);
void __sn__demo_textfile(RtManagedArena *);
void __sn__demo_binaryfile(RtManagedArena *);
void __sn__demo_file_utilities(RtManagedArena *);
void __sn__demo_date(RtManagedArena *);
void __sn__demo_time(RtManagedArena *);

/* Lambda forward declarations */
static long long __lambda_0__(void *__closure__, long long __sn__x);
static long long __lambda_1__(void *__closure__, long long __sn__a, long long __sn__b);
static long long __lambda_2__(void *__closure__, long long __sn__x);
static long long __lambda_3__(void *__closure__, long long __sn__a, long long __sn__b);
static long long __lambda_4__(void *__closure__, long long __sn__x);
static long long __lambda_5__(void *__closure__, long long __sn__x);
static long long __lambda_6__(void *__closure__, long long __sn__x);
typedef struct __closure_7__ {
    void *fn;
    RtArena *arena;
    size_t size;
    long long *multiplier;
} __closure_7__;
static long long __lambda_7__(void *__closure__, long long __sn__x);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);
static RtAny __thunk_3(void);
static RtAny __thunk_4(void);
static RtAny __thunk_5(void);
static RtAny __thunk_6(void);
static RtAny __thunk_7(void);
static RtAny __thunk_8(void);
static RtAny __thunk_9(void);
static RtAny __thunk_10(void);
static RtAny __thunk_11(void);
static RtAny __thunk_12(void);
static RtAny __thunk_13(void);
static RtAny __thunk_14(void);
static RtAny __thunk_15(void);
static RtAny __thunk_16(void);
static RtAny __thunk_17(void);
static RtAny __thunk_18(void);
static RtAny __thunk_19(void);
static RtAny __thunk_20(void);
static RtAny __thunk_21(void);
static RtAny __thunk_22(void);
static RtAny __thunk_23(void);
static RtAny __thunk_24(void);
static RtAny __thunk_25(void);
static RtAny __thunk_26(void);
static RtAny __thunk_27(void);
static RtAny __thunk_28(void);
static RtAny __thunk_29(void);
static RtAny __thunk_30(void);
static RtAny __thunk_31(void);
static RtAny __thunk_32(void);
static RtAny __thunk_33(void);
static RtAny __thunk_34(void);

void __sn__demo_types(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // types.sn - Basic Types in Sindarin
    // ============================================================================
    // This file demonstrates all primitive types available in Sindarin.
    //
    // Types covered:
    //   1. int - Integer numbers
    //   2. double - Floating-point numbers
    //   3. str - String text
    //   4. char - Single characters
    //   5. bool - Boolean true/false
    //   6. void - No return value
    // ============================================================================
    // Entry point for type demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Type System                        │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_integers", __args, 0, __thunk_0);
    } else {
        __sn__show_integers(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_doubles", __args, 0, __thunk_1);
    } else {
        __sn__show_doubles(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_strings", __args, 0, __thunk_2);
    } else {
        __sn__show_strings(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_chars", __args, 0, __thunk_3);
    } else {
        __sn__show_chars(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_booleans", __args, 0, __thunk_4);
    } else {
        __sn__show_booleans(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_type_conversion", __args, 0, __thunk_5);
    } else {
        __sn__show_type_conversion(__local_arena__);
    }
    (void)0;
});
    goto __sn__demo_types_return;
__sn__demo_types_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_integers(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 1. Integers (int)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 1. Integer Type (int) ---\n");
    // Declaration and initialization
    long long __sn__a = 42LL;
    long long __sn__b = -17LL;
    long long __sn__c = 0LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__a);
        char *_r = rt_str_concat(__local_arena__, "a = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__b);
        char *_r = rt_str_concat(__local_arena__, "b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__c);
        char *_r = rt_str_concat(__local_arena__, "c = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Arithmetic operations
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_add_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "  a + b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_sub_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "  a - b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_mul_long(__sn__a, 2LL));
        char *_r = rt_str_concat(__local_arena__, "  a * 2 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_div_long(__sn__a, 5LL));
        char *_r = rt_str_concat(__local_arena__, "  a / 5 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_mod_long(__sn__a, 5LL));
        char *_r = rt_str_concat(__local_arena__, "  a % 5 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Increment/decrement
    rt_print_string("\nIncrement/Decrement:\n");
    long long __sn__x = 5LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_r = rt_str_concat(__local_arena__, "  x = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_inc_long(&__sn__x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_r = rt_str_concat(__local_arena__, "  After x++: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_dec_long(&__sn__x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_r = rt_str_concat(__local_arena__, "  After x--: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Comparisons
    rt_print_string("\nComparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 == 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 != 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 > 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 0LL);
        char *_r = rt_str_concat(__local_arena__, "  10 < 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 >= 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 <= 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_integers_return;
__sn__show_integers_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_doubles(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 2. Doubles (double)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 2. Double Type (double) ---\n");
    // Declaration
    double __sn__pi = 3.1415899999999999;
    double __sn__e = 2.71828;
    double __sn__negative = -1.5;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, __sn__pi);
        char *_r = rt_str_concat(__local_arena__, "pi = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, __sn__e);
        char *_r = rt_str_concat(__local_arena__, "e = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, __sn__negative);
        char *_r = rt_str_concat(__local_arena__, "negative = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Arithmetic
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, rt_add_double(__sn__pi, __sn__e));
        char *_r = rt_str_concat(__local_arena__, "  pi + e = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, rt_mul_double(__sn__pi, 2.0));
        char *_r = rt_str_concat(__local_arena__, "  pi * 2.0 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, 3.3333333333333335);
        char *_r = rt_str_concat(__local_arena__, "  10.0 / 3.0 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Mixed with int (int promotes to double)
    rt_print_string("\nMixed operations:\n");
    double __sn__radius = 5.0;
    double __sn__area = rt_mul_double(rt_mul_double(__sn__pi, __sn__radius), __sn__radius);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, __sn__area);
        char *_r = rt_str_concat(__local_arena__, "  Circle area (r=5): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_doubles_return;
__sn__show_doubles_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_strings(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 3. Strings (str)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 3. String Type (str) ---\n");
    // Declaration
    RtHandle __sn__greeting = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello");
    RtHandle __sn__name = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "World");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "greeting = \"", (char *)rt_managed_pin(__local_arena__, __sn__greeting));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "name = \"", (char *)rt_managed_pin(__local_arena__, __sn__name));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Concatenation
    RtHandle __sn__message = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__greeting), ", ")), (char *)rt_managed_pin(__local_arena__, __sn__name))), "!");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Concatenated: ", (char *)rt_managed_pin(__local_arena__, __sn__message));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // String interpolation
    long long __sn__age = 25LL;
    double __sn__height = 5.9000000000000004;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__age);
        char *_p1 = rt_to_string_double(__local_arena__, __sn__height);
        char *_r = rt_str_concat(__local_arena__, "Interpolation: Age is ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", height is ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Empty string
    RtHandle __sn__empty = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Empty string: \"", (char *)rt_managed_pin(__local_arena__, __sn__empty));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // String comparisons
    rt_print_string("\nString comparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_string("abc", "abc"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" == \"abc\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_ne_string("abc", "xyz"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" != \"xyz\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_lt_string("abc", "abd"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" < \"abd\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_strings_return;
__sn__show_strings_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_chars(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 4. Characters (char)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 4. Character Type (char) ---\n");
    // Declaration
    char __sn__letter = 'A';
    char __sn__digit = '7';
    char __sn__symbol = '@';
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, __sn__letter);
        char *_r = rt_str_concat(__local_arena__, "letter = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, __sn__digit);
        char *_r = rt_str_concat(__local_arena__, "digit = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, __sn__symbol);
        char *_r = rt_str_concat(__local_arena__, "symbol = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char __sn__tab = '\t';
    rt_print_string("\nEscape sequences:\n");
    rt_print_string("  Tab:");
    rt_print_char(__sn__tab);
    rt_print_string("between\n");
    // Char in strings
    char __sn__first = 'S';
    RtHandle __sn__rest = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "indarin");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, __sn__first);
        char *_r = rt_str_concat(__local_arena__, "  Combined: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, (char *)rt_managed_pin(__local_arena__, __sn__rest));
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_chars_return;
__sn__show_chars_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_booleans(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 5. Booleans (bool)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 5. Boolean Type (bool) ---\n");
    // Declaration
    bool __sn__is_active = 1L;
    bool __sn__is_complete = 0L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__is_active);
        char *_r = rt_str_concat(__local_arena__, "is_active = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__is_complete);
        char *_r = rt_str_concat(__local_arena__, "is_complete = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Boolean from comparisons
    long long __sn__x = 10LL;
    long long __sn__y = 5LL;
    bool __sn__greater = rt_gt_long(__sn__x, __sn__y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__y);
        char *_p2 = rt_to_string_bool(__local_arena__, __sn__greater);
        char *_r = rt_str_concat(__local_arena__, "\n", _p0);
        _r = rt_str_concat(__local_arena__, _r, " > ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // NOT operator (!)
    rt_print_string("\nNOT operator (!):\n");
    bool __sn__flag = 0L;
    if (rt_not_bool(__sn__flag)) {
        {
            rt_print_string("  !false = true\n");
        }
    }
    (__sn__flag = 1L);
    if (rt_not_bool(__sn__flag)) {
        {
            rt_print_string("  never printed\n");
        }
    }
    else {
        {
            rt_print_string("  !true = false\n\n");
        }
    }
    goto __sn__show_booleans_return;
__sn__show_booleans_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_type_conversion(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 6. Type Conversion (implicit via interpolation)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 6. Type Display in Strings ---\n");
    long long __sn__i = 42LL;
    double __sn__d = 3.1400000000000001;
    RtHandle __sn__s = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "hello");
    char __sn__c = 'X';
    bool __sn__b = 1L;
    // All types can be displayed via interpolation
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_r = rt_str_concat(__local_arena__, "int: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, __sn__d);
        char *_r = rt_str_concat(__local_arena__, "double: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "str: ", (char *)rt_managed_pin(__local_arena__, __sn__s));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, __sn__c);
        char *_r = rt_str_concat(__local_arena__, "char: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__b);
        char *_r = rt_str_concat(__local_arena__, "bool: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Mixed in single string
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_p1 = rt_to_string_double(__local_arena__, __sn__d);
        char *_p2 = rt_to_string_char(__local_arena__, __sn__c);
        char *_p3 = rt_to_string_bool(__local_arena__, __sn__b);
        char *_r = rt_str_concat(__local_arena__, "\nMixed: i=", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", d=");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ", s=");
        _r = rt_str_concat(__local_arena__, _r, (char *)rt_managed_pin(__local_arena__, __sn__s));
        _r = rt_str_concat(__local_arena__, _r, ", c=");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, ", b=");
        _r = rt_str_concat(__local_arena__, _r, _p3);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_type_conversion_return;
__sn__show_type_conversion_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_loops(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // loops.sn - Loop Constructs in Sindarin
    // ============================================================================
    // This file demonstrates all loop constructs available in Sindarin.
    //
    // Topics covered:
    //   1. While loops
    //   2. For loops (C-style)
    //   3. For-each loops (array iteration)
    //   4. Break and continue
    //   5. Nested loops
    // ============================================================================
    // Entry point for loop demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Loop Features                      │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_while_loops", __args, 0, __thunk_6);
    } else {
        __sn__show_while_loops(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_for_loops", __args, 0, __thunk_7);
    } else {
        __sn__show_for_loops(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_foreach_loops", __args, 0, __thunk_8);
    } else {
        __sn__show_foreach_loops(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_break_continue", __args, 0, __thunk_9);
    } else {
        __sn__show_break_continue(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_nested_loops", __args, 0, __thunk_10);
    } else {
        __sn__show_nested_loops(__local_arena__);
    }
    (void)0;
});
    goto __sn__demo_loops_return;
__sn__demo_loops_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_while_loops(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 1. While Loops
    // ----------------------------------------------------------------------------
    rt_print_string("--- 1. While Loops ---\n");
    // Basic while loop
    rt_print_string("Counting 1 to 5:\n");
    long long __sn__i = 1LL;
    while (rt_le_long(__sn__i, 5LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("\nFinding first power of 2 >= 100:\n");
    long long __sn__power = 1LL;
    while (rt_lt_long(__sn__power, 100LL)) {
        {
            (__sn__power = rt_mul_long(__sn__power, 2LL));
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__power);
        char *_r = rt_str_concat(__local_arena__, "  Result: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Countdown
    rt_print_string("\nCountdown:\n");
    long long __sn__count = 5LL;
    while (rt_gt_long(__sn__count, 0LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__count);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "...");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            (__sn__count = rt_sub_long(__sn__count, 1LL));
        }
    }
    rt_print_string("  Liftoff!\n\n");
    goto __sn__show_while_loops_return;
__sn__show_while_loops_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_for_loops(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 2. For Loops (C-style)
    // ----------------------------------------------------------------------------
    rt_print_string("--- 2. For Loops ---\n");
    // Basic for loop
    rt_print_string("For loop 0 to 4:\n");
    {
        long long __sn__i = 0LL;
        while (rt_lt_long(__sn__i, 5LL)) {
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_r = rt_str_concat(__local_arena__, "  i = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_0__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    rt_print_string("\nFor loop 5 down to 1:\n");
    {
        long long __sn__j = 5LL;
        while (rt_ge_long(__sn__j, 1LL)) {
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__j);
        char *_r = rt_str_concat(__local_arena__, "  j = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_1__:;
            rt_post_dec_long(&__sn__j);
        }
    }
    rt_print_string("\nFor loop with step of 2:\n");
    {
        long long __sn__k = 0LL;
        while (rt_le_long(__sn__k, 10LL)) {
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__k);
        char *_r = rt_str_concat(__local_arena__, "  k = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_2__:;
            (__sn__k = rt_add_long(__sn__k, 2LL));
        }
    }
    rt_print_string("\nSum of 1 to 10:\n");
    long long __sn__sum = 0LL;
    {
        long long __sn__n = 1LL;
        while (rt_le_long(__sn__n, 10LL)) {
            {
                (__sn__sum = rt_add_long(__sn__sum, __sn__n));
            }
        __for_continue_3__:;
            rt_post_inc_long(&__sn__n);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__sum);
        char *_r = rt_str_concat(__local_arena__, "  Sum = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_for_loops_return;
__sn__show_for_loops_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_foreach_loops(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 3. For-Each Loops
    // ----------------------------------------------------------------------------
    rt_print_string("--- 3. For-Each Loops ---\n");
    // Iterate over int array
    RtHandle __sn__numbers = rt_array_create_long_h(__local_arena__, 5, (long long[]){10LL, 20LL, 30LL, 40LL, 50LL});
    rt_print_string("Iterating over int array:\n");
    {
        long long * __arr_0__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers));
        long __len_0__ = rt_array_length(__arr_0__);
        for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            long long __sn__num = __arr_0__[__idx_0__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__num);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    RtHandle __sn__fruits = rt_array_create_string_h(__local_arena__, 3, (char *[]){"apple", "banana", "cherry"});
    rt_print_string("\nIterating over string array:\n");
    {
        RtHandle * __arr_1__ = ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__fruits));
        long __len_1__ = rt_array_length(__arr_1__);
        for (long __idx_1__ = 0; __idx_1__ < __len_1__; __idx_1__++) {
            RtHandle __sn__fruit = __arr_1__[__idx_1__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  ", (char *)rt_managed_pin(__local_arena__, __sn__fruit));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\nSum with for-each:\n");
    long long __sn__total = 0LL;
    {
        long long * __arr_2__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers));
        long __len_2__ = rt_array_length(__arr_2__);
        for (long __idx_2__ = 0; __idx_2__ < __len_2__; __idx_2__++) {
            long long __sn__n = __arr_2__[__idx_2__];
            {
                (__sn__total = rt_add_long(__sn__total, __sn__n));
            }
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__total);
        char *_r = rt_str_concat(__local_arena__, "  Total = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__show_foreach_loops_return;
__sn__show_foreach_loops_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_break_continue(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 4. Break and Continue
    // ----------------------------------------------------------------------------
    rt_print_string("--- 4. Break and Continue ---\n");
    // Break - exit loop early
    rt_print_string("Break at 5:\n");
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 10LL)) {
            {
                if (rt_eq_long(__sn__i, 5LL)) {
                    {
                        rt_print_string("  (breaking)\n");
                        break;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_r = rt_str_concat(__local_arena__, "  i = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_4__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    rt_print_string("\nContinue (skip evens):\n");
    {
        long long __sn__j = 1LL;
        while (rt_le_long(__sn__j, 6LL)) {
            {
                if (rt_eq_long(rt_mod_long(__sn__j, 2LL), 0LL)) {
                    {
                        goto __for_continue_5__;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__j);
        char *_r = rt_str_concat(__local_arena__, "  j = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_5__:;
            rt_post_inc_long(&__sn__j);
        }
    }
    rt_print_string("\nBreak in while (find first > 50 divisible by 7):\n");
    long long __sn__n = 50LL;
    while (rt_lt_long(__sn__n, 100LL)) {
        {
            rt_post_inc_long(&__sn__n);
            if (rt_eq_long(rt_mod_long(__sn__n, 7LL), 0LL)) {
                {
                    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        char *_r = rt_str_concat(__local_arena__, "  Found: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                    break;
                }
            }
        }
    }
    rt_print_string("\nContinue in for-each (skip 'banana'):\n");
    RtHandle __sn__fruits = rt_array_create_string_h(__local_arena__, 4, (char *[]){"apple", "banana", "cherry", "date"});
    {
        RtHandle * __arr_3__ = ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__fruits));
        long __len_3__ = rt_array_length(__arr_3__);
        for (long __idx_3__ = 0; __idx_3__ < __len_3__; __idx_3__++) {
            RtHandle __sn__fruit = __arr_3__[__idx_3__];
            {
                if (rt_eq_string((char *)rt_managed_pin(__local_arena__, __sn__fruit), "banana")) {
                    {
                        continue;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  ", (char *)rt_managed_pin(__local_arena__, __sn__fruit));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n");
    goto __sn__show_break_continue_return;
__sn__show_break_continue_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_nested_loops(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 5. Nested Loops
    // ----------------------------------------------------------------------------
    rt_print_string("--- 5. Nested Loops ---\n");
    // Multiplication table
    rt_print_string("Multiplication table (1-3):\n");
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 3LL)) {
            {
                {
                    long long __sn__j = 1LL;
                    while (rt_le_long(__sn__j, 3LL)) {
                        {
                            long long __sn__product = rt_mul_long(__sn__i, __sn__j);
                            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__j);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__product);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " x ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                        }
                    __for_continue_7__:;
                        rt_post_inc_long(&__sn__j);
                    }
                }
                rt_print_string("\n");
            }
        __for_continue_6__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    rt_print_string("Triangle pattern:\n");
    {
        long long __sn__row = 1LL;
        while (rt_le_long(__sn__row, 5LL)) {
            {
                rt_print_string("  ");
                {
                    long long __sn__col = 1LL;
                    while (rt_le_long(__sn__col, __sn__row)) {
                        {
                            rt_print_string("*");
                        }
                    __for_continue_9__:;
                        rt_post_inc_long(&__sn__col);
                    }
                }
                rt_print_string("\n");
            }
        __for_continue_8__:;
            rt_post_inc_long(&__sn__row);
        }
    }
    rt_print_string("\nNested for-each (pairs):\n");
    RtHandle __sn__a = rt_array_create_long_h(__local_arena__, 2, (long long[]){1LL, 2LL});
    RtHandle __sn__b = rt_array_create_long_h(__local_arena__, 2, (long long[]){10LL, 20LL});
    {
        long long * __arr_4__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__a));
        long __len_4__ = rt_array_length(__arr_4__);
        for (long __idx_4__ = 0; __idx_4__ < __len_4__; __idx_4__++) {
            long long __sn__x = __arr_4__[__idx_4__];
            {
                {
                    long long * __arr_5__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__b));
                    long __len_5__ = rt_array_length(__arr_5__);
                    for (long __idx_5__ = 0; __idx_5__ < __len_5__; __idx_5__++) {
                        long long __sn__y = __arr_5__[__idx_5__];
                        {
                            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__y);
        char *_r = rt_str_concat(__local_arena__, "  (", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                        }
                    }
                }
            }
        }
    }
    goto __sn__show_nested_loops_return;
__sn__show_nested_loops_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_conditionals(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // conditionals.sn - Conditional Statements in Sindarin
    // ============================================================================
    // Demonstrates: if, if-else, comparison operators, NOT, AND (&&), OR (||)
    // ============================================================================
    // Entry point for conditional demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Conditionals                       │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. Basic if
    rt_print_string("--- If Statements ---\n");
    long long __sn__x = 10LL;
    if (rt_gt_long(__sn__x, 5LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        rt_str_concat(__local_arena__, _p0, " is greater than 5\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    if (rt_eq_long(__sn__x, 10LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        rt_str_concat(__local_arena__, _p0, " equals 10\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- If-Else ---\n");
    long long __sn__age = 20LL;
    if (rt_ge_long(__sn__age, 18LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__age);
        char *_r = rt_str_concat(__local_arena__, "Age ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ": Adult\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__age);
        char *_r = rt_str_concat(__local_arena__, "Age ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ": Minor\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    long long __sn__score = 75LL;
    if (rt_ge_long(__sn__score, 60LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__score);
        char *_r = rt_str_concat(__local_arena__, "Score ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ": Pass\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__score);
        char *_r = rt_str_concat(__local_arena__, "Score ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ": Fail\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- NOT Operator ---\n");
    bool __sn__flag = 0L;
    if (rt_not_bool(__sn__flag)) {
        {
            rt_print_string("!false = true\n");
        }
    }
    rt_print_string("\n--- AND (&&) and OR (||) ---\n");
    bool __sn__hasTicket = 1L;
    bool __sn__hasID = 1L;
    bool __sn__isVIP = 0L;
    if (((__sn__hasTicket != 0 && __sn__hasID != 0) ? 1L : 0L)) {
        {
            // Both must be true
            rt_print_string("Entry allowed (has ticket AND ID)\n");
        }
    }
    if (((__sn__hasTicket != 0 || __sn__isVIP != 0) ? 1L : 0L)) {
        {
            rt_print_string("Can enter (has ticket OR is VIP)\n");
        }
    }
    long long __sn__temperature = 25LL;
    if (((rt_gt_long(__sn__temperature, 20LL) != 0 && rt_lt_long(__sn__temperature, 30LL) != 0) ? 1L : 0L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__temperature);
        char *_r = rt_str_concat(__local_arena__, "Temperature ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "C is comfortable\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    if (((rt_lt_long(__sn__temperature, 10LL) != 0 || rt_gt_long(__sn__temperature, 35LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Extreme temperature!\n");
        }
    }
    else {
        {
            rt_print_string("Temperature is moderate\n");
        }
    }
    bool __sn__loggedIn = 1L;
    bool __sn__isAdmin = 0L;
    bool __sn__isModerator = 1L;
    if (((__sn__loggedIn != 0 && ((__sn__isAdmin != 0 || __sn__isModerator != 0) ? 1L : 0L) != 0) ? 1L : 0L)) {
        {
            rt_print_string("User can moderate content\n");
        }
    }
    rt_print_string("\n--- Comparisons ---\n");
    long long __sn__a = 10LL;
    long long __sn__b = 20LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__a);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__b);
        char *_r = rt_str_concat(__local_arena__, "a = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", b = ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "a == b: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_ne_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "a != b: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_lt_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "a < b: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_gt_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "a > b: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 6. Even/odd check (inline)
    rt_print_string("\n--- Even/Odd Check ---\n");
    long long __sn__n = 7LL;
    if (rt_eq_long(rt_mod_long(__sn__n, 2LL), 0LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        rt_str_concat(__local_arena__, _p0, " is even\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        rt_str_concat(__local_arena__, _p0, " is odd\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    (__sn__n = 12LL);
    if (rt_eq_long(rt_mod_long(__sn__n, 2LL), 0LL)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        rt_str_concat(__local_arena__, _p0, " is even\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        rt_str_concat(__local_arena__, _p0, " is odd\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- Max Example ---\n");
    long long __sn__p = 5LL;
    long long __sn__q = 12LL;
    long long __sn__m = __sn__p;
    if (rt_gt_long(__sn__q, __sn__p)) {
        {
            (__sn__m = __sn__q);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__p);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__q);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__m);
        char *_r = rt_str_concat(__local_arena__, "max(", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ") = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__demo_conditionals_return;
__sn__demo_conditionals_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_strings(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // strings.sn - String Features in Sindarin
    // ============================================================================
    // Demonstrates: string literals, length, concatenation, interpolation,
    //               escape sequences, comparisons, and string methods
    // ============================================================================
    // Entry point for string demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Strings                          │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. String literals
    rt_print_string("--- String Literals ---\n");
    RtHandle __sn__hello = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello, World!");
    rt_print_string((char *)rt_managed_pin(__local_arena__, __sn__hello));
    rt_print_string("\n");
    RtHandle __sn__empty = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Empty string: \"", (char *)rt_managed_pin(__local_arena__, __sn__empty));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 2. String length (both len() and .length)
    rt_print_string("\n--- String Length ---\n");
    RtHandle __sn__greeting = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, (long)strlen((char *)rt_managed_pin(__local_arena__, __sn__greeting)));
        char *_r = rt_str_concat(__local_arena__, "len(\"", (char *)rt_managed_pin(__local_arena__, __sn__greeting));
        _r = rt_str_concat(__local_arena__, _r, "\") = ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_str_length((char *)rt_managed_pin(__local_arena__, __sn__greeting)));
        char *_r = rt_str_concat(__local_arena__, "\"", (char *)rt_managed_pin(__local_arena__, __sn__greeting));
        _r = rt_str_concat(__local_arena__, _r, "\".length = ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__sentence = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "The quick brown fox");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, (long)strlen((char *)rt_managed_pin(__local_arena__, __sn__sentence)));
        char *_r = rt_str_concat(__local_arena__, "len(\"", (char *)rt_managed_pin(__local_arena__, __sn__sentence));
        _r = rt_str_concat(__local_arena__, _r, "\") = ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 3. Concatenation
    rt_print_string("\n--- Concatenation ---\n");
    RtHandle __sn__first = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello");
    RtHandle __sn__second = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "World");
    RtHandle __sn__combined = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__first), " ")), (char *)rt_managed_pin(__local_arena__, __sn__second));
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Combined: \"", (char *)rt_managed_pin(__local_arena__, __sn__combined));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 4. Basic Interpolation
    rt_print_string("\n--- Basic Interpolation ---\n");
    RtHandle __sn__name = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Alice");
    long long __sn__age = 30LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__age);
        char *_r = rt_str_concat(__local_arena__, "Name: ", (char *)rt_managed_pin(__local_arena__, __sn__name));
        _r = rt_str_concat(__local_arena__, _r, ", Age: ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long __sn__x = 5LL;
    long long __sn__y = 3LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__y);
        char *_p2 = rt_to_string_long(__local_arena__, rt_add_long(__sn__x, __sn__y));
        char *_r = rt_str_concat(__local_arena__, _p0, " + ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__y);
        char *_p2 = rt_to_string_long(__local_arena__, rt_mul_long(__sn__x, __sn__y));
        char *_r = rt_str_concat(__local_arena__, _p0, " * ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 4a. Escaped quotes in interpolation
    rt_print_string("\n--- Escaped Quotes in Interpolation ---\n");
    RtHandle __sn__item = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "widget");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Item name: \"", (char *)rt_managed_pin(__local_arena__, __sn__item));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = "Nested quotes: \"She said \\\"hello\\\"\"\n";
        rt_print_string(_str_arg0);
    });
    // 4b. Format specifiers
    rt_print_string("\n--- Format Specifiers ---\n");
    double __sn__pi = 3.1415926535900001;
    double __sn__price = 42.5;
    long long __sn__num = 255LL;
    long long __sn__count = 7LL;
    // Floating point precision
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__local_arena__, __sn__pi, ".2f");
        char *_r = rt_str_concat(__local_arena__, "Pi (2 decimals): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__local_arena__, __sn__pi, ".4f");
        char *_r = rt_str_concat(__local_arena__, "Pi (4 decimals): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__local_arena__, __sn__price, ".2f");
        char *_r = rt_str_concat(__local_arena__, "Price: $", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Hexadecimal formatting
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__local_arena__, __sn__num, "x");
        char *_r = rt_str_concat(__local_arena__, "255 in hex (lower): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__local_arena__, __sn__num, "X");
        char *_r = rt_str_concat(__local_arena__, "255 in hex (upper): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Zero-padded integers
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__local_arena__, __sn__count, "03d");
        char *_r = rt_str_concat(__local_arena__, "Count (3 digits): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__local_arena__, __sn__count, "05d");
        char *_r = rt_str_concat(__local_arena__, "Count (5 digits): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 4c. Multi-line interpolated strings
    rt_print_string("\n--- Multi-line Interpolation ---\n");
    RtHandle __sn__user = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Bob");
    long long __sn__score = 95LL;
    RtHandle __sn__profile = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__score);
        char *_r = rt_str_concat(__local_arena__, "User Profile:\n  Name: ", (char *)rt_managed_pin(__local_arena__, __sn__user));
        _r = rt_str_concat(__local_arena__, _r, "\n  Score: ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n  Grade: A");
        rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, _r);
    });
    rt_print_string((char *)rt_managed_pin(__local_arena__, __sn__profile));
    rt_print_string("\n");
    // Multi-line with indentation
    long long __sn__a = 10LL;
    long long __sn__b = 20LL;
    RtHandle __sn__report = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__a);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__b);
        char *_p2 = rt_to_string_long(__local_arena__, rt_add_long(__sn__a, __sn__b));
        char *_p3 = rt_to_string_long(__local_arena__, rt_mul_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "Calculation Report:\n    Value A: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n    Value B: ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n    Sum: ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n    Product: ");
        _r = rt_str_concat(__local_arena__, _r, _p3);
        rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, _r);
    });
    rt_print_string((char *)rt_managed_pin(__local_arena__, __sn__report));
    rt_print_string("\n");
    // 4d. Nested interpolation
    rt_print_string("\n--- Nested Interpolation ---\n");
    long long __sn__inner_val = 42LL;
    RtHandle __sn__outer = ({
        char *_p0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__inner_val);
        rt_str_concat(__local_arena__, "inner value is ", _p0);
    });
        rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, "Outer contains: ", _p0);
    });
    rt_print_string((char *)rt_managed_pin(__local_arena__, __sn__outer));
    rt_print_string("\n");
    // Deeper nesting
    long long __sn__level = 3LL;
    RtHandle __sn__deep = ({
        char *_p0 = ({
        char *_p0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__level);
        rt_str_concat(__local_arena__, "L3: ", _p0);
    });
        rt_str_concat(__local_arena__, "L2: ", _p0);
    });
        rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, "L1: ", _p0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Deep nesting: ", (char *)rt_managed_pin(__local_arena__, __sn__deep));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 5. Escape sequences
    rt_print_string("\n--- Escape Sequences ---\n");
    rt_print_string("Line 1\nLine 2\nLine 3\n");
    rt_print_string("Tab:\tValue\n");
    rt_print_string("Quote: \"Hello\"\n");
    // 6. String comparisons
    rt_print_string("\n--- Comparisons ---\n");
    RtHandle __sn__s1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "apple");
    RtHandle __sn__s2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "apple");
    RtHandle __sn__s3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "banana");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_string((char *)rt_managed_pin(__local_arena__, __sn__s1), (char *)rt_managed_pin(__local_arena__, __sn__s2)));
        char *_r = rt_str_concat(__local_arena__, "apple == apple: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_string((char *)rt_managed_pin(__local_arena__, __sn__s1), (char *)rt_managed_pin(__local_arena__, __sn__s3)));
        char *_r = rt_str_concat(__local_arena__, "apple == banana: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 7. Case conversion: toUpper and toLower
    rt_print_string("\n--- Case Conversion ---\n");
    RtHandle __sn__text = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello World");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__text));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toUpper(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__text));
        char *_r = rt_str_concat(__local_arena__, "toUpper(): \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toLower(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__text));
        char *_r = rt_str_concat(__local_arena__, "toLower(): \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Method chaining on literals
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toUpper(__local_arena__, "sindarin");
        char *_r = rt_str_concat(__local_arena__, "\"sindarin\".toUpper() = \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 8. Trim whitespace
    rt_print_string("\n--- Trim ---\n");
    RtHandle __sn__padded = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "   hello world   ");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__padded));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_trim(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__padded));
        char *_r = rt_str_concat(__local_arena__, "trim(): \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 9. Substring extraction
    rt_print_string("\n--- Substring ---\n");
    RtHandle __sn__phrase = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello, World!");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__phrase));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_substring(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__phrase), 0LL, 5LL);
        char *_r = rt_str_concat(__local_arena__, "substring(0, 5): \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_substring(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__phrase), 7LL, 12LL);
        char *_r = rt_str_concat(__local_arena__, "substring(7, 12): \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 10. indexOf - find substring position
    rt_print_string("\n--- indexOf ---\n");
    RtHandle __sn__haystack = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "the quick brown fox");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "String: \"", (char *)rt_managed_pin(__local_arena__, __sn__haystack));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__search1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "quick");
    RtHandle __sn__search2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "fox");
    RtHandle __sn__search3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "cat");
    long long __sn__idx1 = rt_str_indexOf((char *)rt_managed_pin(__local_arena__, __sn__haystack), (char *)rt_managed_pin(__local_arena__, __sn__search1));
    long long __sn__idx2 = rt_str_indexOf((char *)rt_managed_pin(__local_arena__, __sn__haystack), (char *)rt_managed_pin(__local_arena__, __sn__search2));
    long long __sn__idx3 = rt_str_indexOf((char *)rt_managed_pin(__local_arena__, __sn__haystack), (char *)rt_managed_pin(__local_arena__, __sn__search3));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__idx1);
        char *_r = rt_str_concat(__local_arena__, "indexOf(\"quick\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__idx2);
        char *_r = rt_str_concat(__local_arena__, "indexOf(\"fox\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__idx3);
        char *_r = rt_str_concat(__local_arena__, "indexOf(\"cat\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 11. String search: startsWith, endsWith, contains
    rt_print_string("\n--- String Search ---\n");
    RtHandle __sn__filename = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "document.txt");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "String: \"", (char *)rt_managed_pin(__local_arena__, __sn__filename));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__prefix1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "doc");
    RtHandle __sn__prefix2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "file");
    RtHandle __sn__suffix1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, ".txt");
    RtHandle __sn__suffix2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, ".pdf");
    RtHandle __sn__sub1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "ment");
    RtHandle __sn__sub2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "xyz");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_startsWith((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__prefix1)));
        char *_r = rt_str_concat(__local_arena__, "startsWith(\"doc\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_startsWith((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__prefix2)));
        char *_r = rt_str_concat(__local_arena__, "startsWith(\"file\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_endsWith((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__suffix1)));
        char *_r = rt_str_concat(__local_arena__, "endsWith(\".txt\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_endsWith((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__suffix2)));
        char *_r = rt_str_concat(__local_arena__, "endsWith(\".pdf\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_contains((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__sub1)));
        char *_r = rt_str_concat(__local_arena__, "contains(\"ment\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_contains((char *)rt_managed_pin(__local_arena__, __sn__filename), (char *)rt_managed_pin(__local_arena__, __sn__sub2)));
        char *_r = rt_str_concat(__local_arena__, "contains(\"xyz\"): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 12. Replace substrings
    rt_print_string("\n--- Replace ---\n");
    RtHandle __sn__original = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "hello world");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__original));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__oldStr = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "world");
    RtHandle __sn__newStr = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Sindarin");
    RtHandle __sn__replaced = rt_str_replace_h(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__original), (char *)rt_managed_pin(__local_arena__, __sn__oldStr), (char *)rt_managed_pin(__local_arena__, __sn__newStr));
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "replace(\"world\", \"Sindarin\"): \"", (char *)rt_managed_pin(__local_arena__, __sn__replaced));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 13. Split into array
    rt_print_string("\n--- Split ---\n");
    RtHandle __sn__csv = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "apple,banana,cherry");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "String: \"", (char *)rt_managed_pin(__local_arena__, __sn__csv));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__delim = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, ",");
    RtHandle __sn__parts = rt_str_split_h(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__csv), (char *)rt_managed_pin(__local_arena__, __sn__delim));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__parts))));
        char *_r = rt_str_concat(__local_arena__, "split(\",\") -> ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " parts:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        RtHandle * __arr_6__ = ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__parts));
        long __len_6__ = rt_array_length(__arr_6__);
        for (long __idx_6__ = 0; __idx_6__ < __len_6__; __idx_6__++) {
            RtHandle __sn__part = __arr_6__[__idx_6__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  - \"", (char *)rt_managed_pin(__local_arena__, __sn__part));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n--- Method Chaining ---\n");
    RtHandle __sn__messy = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "  HELLO WORLD  ");
    RtHandle __sn__clean = rt_str_toLower_h(__local_arena__, rt_str_trim(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__messy)));
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__messy));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "trim().toLower(): \"", (char *)rt_managed_pin(__local_arena__, __sn__clean));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Chaining on literal
    RtHandle __sn__chainTest = rt_str_toUpper_h(__local_arena__, rt_str_trim(__local_arena__, "  TEST  "));
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Chained on literal: \"", (char *)rt_managed_pin(__local_arena__, __sn__chainTest));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 15. splitWhitespace - Split on any whitespace
    rt_print_string("\n--- splitWhitespace ---\n");
    RtHandle __sn__wsText = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "hello   world\tfoo\nbar");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Original: \"", (char *)rt_managed_pin(__local_arena__, __sn__wsText));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__wsWords = rt_array_from_raw_strings_h(__local_arena__, RT_HANDLE_NULL, rt_str_split_whitespace(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__wsText)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__wsWords))));
        char *_r = rt_str_concat(__local_arena__, "splitWhitespace() -> ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " words:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        RtHandle * __arr_7__ = ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__wsWords));
        long __len_7__ = rt_array_length(__arr_7__);
        for (long __idx_7__ = 0; __idx_7__ < __len_7__; __idx_7__++) {
            RtHandle __sn__wsWord = __arr_7__[__idx_7__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  - \"", (char *)rt_managed_pin(__local_arena__, __sn__wsWord));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n--- splitLines ---\n");
    RtHandle __sn__multiLine = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Line 1\nLine 2\nLine 3");
    rt_print_string("Original (3 lines with \\n):\n");
    RtHandle __sn__lineArr = rt_array_from_raw_strings_h(__local_arena__, RT_HANDLE_NULL, rt_str_split_lines(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__multiLine)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__lineArr))));
        char *_r = rt_str_concat(__local_arena__, "splitLines() -> ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " lines:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        RtHandle * __arr_8__ = ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__lineArr));
        long __len_8__ = rt_array_length(__arr_8__);
        for (long __idx_8__ = 0; __idx_8__ < __len_8__; __idx_8__++) {
            RtHandle __sn__ln = __arr_8__[__idx_8__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  \"", (char *)rt_managed_pin(__local_arena__, __sn__ln));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n--- isBlank ---\n");
    RtHandle __sn__blankEmpty = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    RtHandle __sn__blankSpaces = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "   ");
    RtHandle __sn__blankTabs = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "\t\t");
    RtHandle __sn__notBlank = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "hello");
    RtHandle __sn__notBlank2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "  hi  ");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_is_blank((char *)rt_managed_pin(__local_arena__, __sn__blankEmpty)));
        char *_r = rt_str_concat(__local_arena__, "\"\" isBlank: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_is_blank((char *)rt_managed_pin(__local_arena__, __sn__blankSpaces)));
        char *_r = rt_str_concat(__local_arena__, "\"   \" isBlank: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_is_blank((char *)rt_managed_pin(__local_arena__, __sn__blankTabs)));
        char *_r = rt_str_concat(__local_arena__, "\"\\t\\t\" isBlank: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_is_blank((char *)rt_managed_pin(__local_arena__, __sn__notBlank)));
        char *_r = rt_str_concat(__local_arena__, "\"hello\" isBlank: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_str_is_blank((char *)rt_managed_pin(__local_arena__, __sn__notBlank2)));
        char *_r = rt_str_concat(__local_arena__, "\"  hi  \" isBlank: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__demo_strings_return;
__sn__demo_strings_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_functions(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // functions.sn - Function Features in Sindarin
    // ============================================================================
    // Demonstrates: function definition, parameters, return values, recursion
    // ============================================================================
    // Entry point for function demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin Functions                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. Basic functions - void functions work fine
    rt_print_string("--- Basic Functions ---\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("greet", __args, 0, __thunk_11);
    } else {
        __sn__greet(__local_arena__);
    }
    (void)0;
});
    // 2. Parameters - void functions with params work
    rt_print_string("\n--- Parameters ---\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_string((char *)rt_managed_pin(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Alice")));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("greet_person", __args, 1, __thunk_12);
    } else {
        __sn__greet_person(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Alice"));
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_string((char *)rt_managed_pin(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Bob")));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("greet_person", __args, 1, __thunk_13);
    } else {
        __sn__greet_person(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Bob"));
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(5LL);
        __args[1] = rt_box_int(3LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("print_sum", __args, 2, __thunk_14);
    } else {
        __sn__print_sum(__local_arena__, 5LL, 3LL);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(10LL);
        __args[1] = rt_box_int(20LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("print_sum", __args, 2, __thunk_15);
    } else {
        __sn__print_sum(__local_arena__, 10LL, 20LL);
    }
    (void)0;
});
    // 3. Return values - demonstrated inline
    rt_print_string("\n--- Return Values ---\n");
    // In Sindarin, functions can return values:
    // fn add(a: int, b: int): int => return a + b
    // fn factorial(n: int): int => if n <= 1 => return 1; return n * factorial(n-1)
    //
    // These work correctly but have C compilation issues with forward declarations.
    // See samples/main.sn for examples of functions with return values.
    rt_print_string("See main.sn for return value examples\n");
    // 4. Recursion example (inline)
    rt_print_string("\n--- Recursion Example ---\n");
    rt_print_string("factorial(5) = 120\n");
    rt_print_string("fibonacci sequence: 0, 1, 1, 2, 3, 5, 8...\n");
    goto __sn__demo_functions_return;
__sn__demo_functions_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__greet(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("Hello from greet()!\n");
    goto __sn__greet_return;
__sn__greet_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__greet_person(RtManagedArena *__caller_arena__, RtHandle __sn__name) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__name = rt_managed_clone_any(__local_arena__, __caller_arena__, __sn__name);
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Hello, ", (char *)rt_managed_pin(__local_arena__, __sn__name));
        _r = rt_str_concat(__local_arena__, _r, "!\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__greet_person_return;
__sn__greet_person_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__print_sum(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long __sn__sum = rt_add_long(__sn__a, __sn__b);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__a);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__b);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__sum);
        char *_r = rt_str_concat(__local_arena__, _p0, " + ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__print_sum_return;
__sn__print_sum_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_arrays(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // arrays.sn - Array Features in Sindarin
    // ============================================================================
    // Demonstrates: declaration, methods, slicing, negative indexing, for-each,
    //               range literals, spread operator
    // ============================================================================
    // Entry point for array demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Arrays                           │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. Declaration and initialization
    rt_print_string("--- Declaration ---\n");
    RtHandle __sn__numbers = rt_array_create_long_h(__local_arena__, 5, (long long[]){10LL, 20LL, 30LL, 40LL, 50LL});
    rt_print_string("numbers = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers)));
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers))));
        char *_r = rt_str_concat(__local_arena__, "len(numbers) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers))));
        char *_r = rt_str_concat(__local_arena__, "numbers.length = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers))[0LL]);
        char *_r = rt_str_concat(__local_arena__, "numbers[0] = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__numbers))[2LL]);
        char *_r = rt_str_concat(__local_arena__, "numbers[2] = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 2. Push and Pop
    rt_print_string("\n--- Push and Pop ---\n");
    RtHandle __sn__arr = rt_array_create_long_h(__local_arena__, 0, (long long[]){});
    rt_print_string("Starting with empty array: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr))));
        char *_r = rt_str_concat(__local_arena__, " (length = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    (__sn__arr = rt_array_push_long_h(__local_arena__, __sn__arr, 10LL));
    (__sn__arr = rt_array_push_long_h(__local_arena__, __sn__arr, 20LL));
    (__sn__arr = rt_array_push_long_h(__local_arena__, __sn__arr, 30LL));
    rt_print_string("After push(10), push(20), push(30): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)));
    rt_print_string("\n");
    long long __sn__popped = rt_array_pop_long_h(__local_arena__, __sn__arr);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__popped);
        char *_r = rt_str_concat(__local_arena__, "pop() returned: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("After pop: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)));
    rt_print_string("\n");
    // 3. Insert and Remove
    rt_print_string("\n--- Insert and Remove ---\n");
    RtHandle __sn__items = rt_array_create_long_h(__local_arena__, 5, (long long[]){1LL, 2LL, 3LL, 4LL, 5LL});
    rt_print_string("Starting: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__items)));
    rt_print_string("\n");
    (__sn__items = rt_array_ins_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__items)), 99LL, 2LL));
    rt_print_string("After insert(99, 2): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__items)));
    rt_print_string("\n");
    (__sn__items = rt_array_rem_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__items)), 2LL));
    rt_print_string("After remove(2): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__items)));
    rt_print_string("\n");
    // 4. Reverse
    rt_print_string("\n--- Reverse ---\n");
    RtHandle __sn__nums = rt_array_create_long_h(__local_arena__, 5, (long long[]){1LL, 2LL, 3LL, 4LL, 5LL});
    rt_print_string("Before reverse: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__nums)));
    rt_print_string("\n");
    (__sn__nums = rt_array_rev_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__nums))));
    rt_print_string("After reverse(): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__nums)));
    rt_print_string("\n");
    // 5. Clone
    rt_print_string("\n--- Clone ---\n");
    RtHandle __sn__original = rt_array_create_long_h(__local_arena__, 3, (long long[]){10LL, 20LL, 30LL});
    RtHandle __sn__copy = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, ((long long *)rt_managed_pin_array(__local_arena__, __sn__original)));
    rt_print_string("Original: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__original)));
    rt_print_string("\n");
    rt_print_string("Clone: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__copy)));
    rt_print_string("\n");
    (__sn__copy = rt_array_push_long_h(__local_arena__, __sn__copy, 40LL));
    rt_print_string("After pushing 40 to clone:\n");
    rt_print_string("  Original: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__original)));
    rt_print_string("\n");
    rt_print_string("  Clone: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__copy)));
    rt_print_string("\n");
    // 6. Concat (returns a NEW array, doesn't modify originals)
    rt_print_string("\n--- Concat ---\n");
    RtHandle __sn__first = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 3LL});
    RtHandle __sn__second = rt_array_create_long_h(__local_arena__, 3, (long long[]){4LL, 5LL, 6LL});
    rt_print_string("First: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__first)));
    rt_print_string("\n");
    rt_print_string("Second: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__second)));
    rt_print_string("\n");
    RtHandle __sn__combined = rt_array_concat_long_h(__local_arena__, RT_HANDLE_NULL, ((long long *)rt_managed_pin_array(__local_arena__, __sn__first)), ((long long *)rt_managed_pin_array(__local_arena__, __sn__second)));
    rt_print_string("first.concat(second): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__combined)));
    rt_print_string("\n");
    rt_print_string("First after concat: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__first)));
    rt_print_string(" (unchanged)\n");
    // 7. IndexOf and Contains
    rt_print_string("\n--- IndexOf and Contains ---\n");
    RtHandle __sn__search = rt_array_create_long_h(__local_arena__, 5, (long long[]){10LL, 20LL, 30LL, 40LL, 50LL});
    rt_print_string("Array: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__search)));
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_indexOf_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__search)), 30LL));
        char *_r = rt_str_concat(__local_arena__, "indexOf(30) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_indexOf_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__search)), 99LL));
        char *_r = rt_str_concat(__local_arena__, "indexOf(99) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_array_contains_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__search)), 30LL));
        char *_r = rt_str_concat(__local_arena__, "contains(30) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_array_contains_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__search)), 99LL));
        char *_r = rt_str_concat(__local_arena__, "contains(99) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 8. Join
    rt_print_string("\n--- Join ---\n");
    RtHandle __sn__words = rt_array_create_string_h(__local_arena__, 3, (char *[]){"apple", "banana", "cherry"});
    rt_print_string("Array: ");
    rt_print_array_string_h(__local_arena__, (RtHandle *)rt_managed_pin_array(__local_arena__, __sn__words));
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_string_h(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__words)), ", ");
        char *_r = rt_str_concat(__local_arena__, "join(\", \") = \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_string_h(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__words)), " - ");
        char *_r = rt_str_concat(__local_arena__, "join(\" - \") = \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__digits = rt_array_create_long_h(__local_arena__, 5, (long long[]){1LL, 2LL, 3LL, 4LL, 5LL});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__digits)), "-");
        char *_r = rt_str_concat(__local_arena__, "Int array joined: \"", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 9. Clear
    rt_print_string("\n--- Clear ---\n");
    RtHandle __sn__toclear = rt_array_create_long_h(__local_arena__, 5, (long long[]){1LL, 2LL, 3LL, 4LL, 5LL});
    rt_print_string("Before clear: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__toclear)));
    rt_print_string("\n");
    rt_array_clear(((long long *)rt_managed_pin_array(__local_arena__, __sn__toclear)));
    rt_print_string("After clear(): ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__toclear)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__toclear))));
        char *_r = rt_str_concat(__local_arena__, " (length = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 10. Slicing
    rt_print_string("\n--- Slicing ---\n");
    RtHandle __sn__slicetest = rt_array_create_long_h(__local_arena__, 5, (long long[]){10LL, 20LL, 30LL, 40LL, 50LL});
    rt_print_string("Array: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__slicetest)));
    rt_print_string("\n");
    RtHandle __sn__s1 = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__slicetest)), 1LL, 4LL, LONG_MIN);
    rt_print_string("arr[1..4] = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__s1)));
    rt_print_string("\n");
    RtHandle __sn__s2 = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__slicetest)), LONG_MIN, 3LL, LONG_MIN);
    rt_print_string("arr[..3] = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__s2)));
    rt_print_string("\n");
    RtHandle __sn__s3 = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__slicetest)), 2LL, LONG_MIN, LONG_MIN);
    rt_print_string("arr[2..] = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__s3)));
    rt_print_string("\n");
    RtHandle __sn__s4 = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__slicetest)), LONG_MIN, LONG_MIN, LONG_MIN);
    rt_print_string("arr[..] (full copy) = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__s4)));
    rt_print_string("\n");
    // 11. Step Slicing
    rt_print_string("\n--- Step Slicing ---\n");
    RtHandle __sn__steptest = rt_array_create_long_h(__local_arena__, 10, (long long[]){0LL, 1LL, 2LL, 3LL, 4LL, 5LL, 6LL, 7LL, 8LL, 9LL});
    rt_print_string("Array: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__steptest)));
    rt_print_string("\n");
    RtHandle __sn__evens = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__steptest)), LONG_MIN, LONG_MIN, 2LL);
    rt_print_string("arr[..:2] (every 2nd) = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__evens)));
    rt_print_string("\n");
    RtHandle __sn__odds = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__steptest)), 1LL, LONG_MIN, 2LL);
    rt_print_string("arr[1..:2] (odds) = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__odds)));
    rt_print_string("\n");
    RtHandle __sn__thirds = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__steptest)), LONG_MIN, LONG_MIN, 3LL);
    rt_print_string("arr[..:3] (every 3rd) = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__thirds)));
    rt_print_string("\n");
    // 12. Negative Indexing
    rt_print_string("\n--- Negative Indexing ---\n");
    RtHandle __sn__negtest = rt_array_create_long_h(__local_arena__, 5, (long long[]){10LL, 20LL, 30LL, 40LL, 50LL});
    rt_print_string("Array: ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest)));
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))[(-1LL) < 0 ? rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))) + (-1LL) : (-1LL)]);
        char *_r = rt_str_concat(__local_arena__, "arr[-1] = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))[(-2LL) < 0 ? rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))) + (-2LL) : (-2LL)]);
        char *_r = rt_str_concat(__local_arena__, "arr[-2] = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))[(-3LL) < 0 ? rt_array_length(((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest))) + (-3LL) : (-3LL)]);
        char *_r = rt_str_concat(__local_arena__, "arr[-3] = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtHandle __sn__lasttwo = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest)), -2LL, LONG_MIN, LONG_MIN);
    rt_print_string("arr[-2..] = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__lasttwo)));
    rt_print_string("\n");
    RtHandle __sn__notlast = rt_array_slice_long_h(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__negtest)), LONG_MIN, -1LL, LONG_MIN);
    rt_print_string("arr[..-1] = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__notlast)));
    rt_print_string("\n");
    // 13. For-Each Iteration
    rt_print_string("\n--- For-Each Iteration ---\n");
    RtHandle __sn__iterate = rt_array_create_long_h(__local_arena__, 3, (long long[]){10LL, 20LL, 30LL});
    rt_print_string("Iterating over ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__iterate)));
    rt_print_string(":\n");
    {
        long long * __arr_9__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__iterate));
        long __len_9__ = rt_array_length(__arr_9__);
        for (long __idx_9__ = 0; __idx_9__ < __len_9__; __idx_9__++) {
            long long __sn__x = __arr_9__[__idx_9__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        char *_r = rt_str_concat(__local_arena__, "  value: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    long long __sn__sum = 0LL;
    {
        long long * __arr_10__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__iterate));
        long __len_10__ = rt_array_length(__arr_10__);
        for (long __idx_10__ = 0; __idx_10__ < __len_10__; __idx_10__++) {
            long long __sn__n = __arr_10__[__idx_10__];
            {
                (__sn__sum = rt_add_long(__sn__sum, __sn__n));
            }
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__sum);
        char *_r = rt_str_concat(__local_arena__, "Sum = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 14. Array Equality
    rt_print_string("\n--- Array Equality ---\n");
    RtHandle __sn__eq1 = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 3LL});
    RtHandle __sn__eq2 = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 3LL});
    RtHandle __sn__eq3 = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 4LL});
    RtHandle __sn__eq4 = rt_array_create_long_h(__local_arena__, 2, (long long[]){1LL, 2LL});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_array_eq_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__eq1)), ((long long *)rt_managed_pin_array(__local_arena__, __sn__eq2))));
        char *_r = rt_str_concat(__local_arena__, "{1,2,3} == {1,2,3}: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_array_eq_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__eq1)), ((long long *)rt_managed_pin_array(__local_arena__, __sn__eq3))));
        char *_r = rt_str_concat(__local_arena__, "{1,2,3} == {1,2,4}: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_array_eq_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__eq1)), ((long long *)rt_managed_pin_array(__local_arena__, __sn__eq4))));
        char *_r = rt_str_concat(__local_arena__, "{1,2,3} == {1,2}: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, (!rt_array_eq_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__eq1)), ((long long *)rt_managed_pin_array(__local_arena__, __sn__eq3)))));
        char *_r = rt_str_concat(__local_arena__, "{1,2,3} != {1,2,4}: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 15. Range Literals
    rt_print_string("\n--- Range Literals ---\n");
    RtHandle __sn__range1 = rt_array_range_h(__local_arena__, 1LL, 6LL);
    rt_print_string("1..6 = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__range1)));
    rt_print_string("\n");
    RtHandle __sn__range2 = rt_array_range_h(__local_arena__, 0LL, 10LL);
    rt_print_string("0..10 = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__range2)));
    rt_print_string("\n");
    // Range in array literal
    RtHandle __sn__withRange = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_concat_long(__local_arena__, rt_array_concat_long(__local_arena__, rt_array_create_long(__local_arena__, 1, (long long[]){0LL}), rt_array_range(__local_arena__, 1LL, 4LL)), rt_array_create_long(__local_arena__, 1, (long long[]){10LL})));
    rt_print_string("{0, 1..4, 10} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__withRange)));
    rt_print_string("\n");
    // Combining multiple ranges
    RtHandle __sn__multiRange = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_concat_long(__local_arena__, rt_array_range(__local_arena__, 1LL, 3LL), rt_array_range(__local_arena__, 10LL, 13LL)));
    rt_print_string("{1..3, 10..13} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__multiRange)));
    rt_print_string("\n");
    // 16. Spread Operator
    rt_print_string("\n--- Spread Operator ---\n");
    RtHandle __sn__source = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 3LL});
    rt_print_string("source = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__source)));
    rt_print_string("\n");
    // Clone with spread
    RtHandle __sn__spreadCopy = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_clone_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__source))));
    rt_print_string("{...source} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__spreadCopy)));
    rt_print_string("\n");
    // Prepend and append
    RtHandle __sn__extended = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_concat_long(__local_arena__, rt_array_concat_long(__local_arena__, rt_array_concat_long(__local_arena__, rt_array_create_long(__local_arena__, 1, (long long[]){0LL}), rt_array_clone_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__source)))), rt_array_create_long(__local_arena__, 1, (long long[]){4LL})), rt_array_create_long(__local_arena__, 1, (long long[]){5LL})));
    rt_print_string("{0, ...source, 4, 5} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__extended)));
    rt_print_string("\n");
    // Combine arrays
    RtHandle __sn__arr_a = rt_array_create_long_h(__local_arena__, 2, (long long[]){1LL, 2LL});
    RtHandle __sn__arr_b = rt_array_create_long_h(__local_arena__, 2, (long long[]){3LL, 4LL});
    RtHandle __sn__merged = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_concat_long(__local_arena__, rt_array_clone_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__arr_a))), rt_array_clone_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__arr_b)))));
    rt_print_string("{...{1,2}, ...{3,4}} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__merged)));
    rt_print_string("\n");
    // Mix spread and range
    RtHandle __sn__mixed = rt_array_clone_long_h(__local_arena__, RT_HANDLE_NULL, rt_array_concat_long(__local_arena__, rt_array_clone_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__source))), rt_array_range(__local_arena__, 10LL, 13LL)));
    rt_print_string("{...source, 10..13} = ");
    rt_print_array_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__mixed)));
    rt_print_string("\n");
    // 17. Different Array Types
    rt_print_string("\n--- Different Array Types ---\n");
    RtHandle __sn__doubles = rt_array_create_double_h(__local_arena__, 3, (double[]){1.5, 2.5, 3.5});
    rt_print_string("double[]: ");
    rt_print_array_double(((double *)rt_managed_pin_array(__local_arena__, __sn__doubles)));
    rt_print_string("\n");
    RtHandle __sn__chars = rt_array_create_char_h(__local_arena__, 5, (char[]){'H', 'e', 'l', 'l', 'o'});
    rt_print_string("char[]: ");
    rt_print_array_char(((char *)rt_managed_pin_array(__local_arena__, __sn__chars)));
    rt_print_string("\n");
    RtHandle __sn__bools = rt_array_create_bool_h(__local_arena__, 3, (int[]){1L, 0L, 1L});
    rt_print_string("bool[]: ");
    rt_print_array_bool(((int *)rt_managed_pin_array(__local_arena__, __sn__bools)));
    rt_print_string("\n");
    RtHandle __sn__strings = rt_array_create_string_h(__local_arena__, 2, (char *[]){"hello", "world"});
    rt_print_string("str[]: ");
    rt_print_array_string_h(__local_arena__, (RtHandle *)rt_managed_pin_array(__local_arena__, __sn__strings));
    rt_print_string("\n");
    goto __sn__demo_arrays_return;
__sn__demo_arrays_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_lambda(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                     Sindarin Lambda Expressions                  │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // =========================================
    // Explicit Type Annotations (verbose style)
    // =========================================
    rt_print_string("Explicit type annotations:\n");
    // Test 1: Basic lambda with explicit types
    __Closure__ * __sn__double_it = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_0__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    long long __sn__result = ((long long (*)(void *, long long))__sn__double_it->fn)(__sn__double_it, 5LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__result);
        char *_r = rt_str_concat(__local_arena__, "  double_it(5) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Test 2: Lambda with multiple parameters (explicit)
    __Closure__ * __sn__add = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_1__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long, long long))__sn__add->fn)(__sn__add, 3LL, 4LL));
        char *_r = rt_str_concat(__local_arena__, "  add(3, 4) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // =========================================
    // Type Inference (concise style)
    // =========================================
    rt_print_string("\nType inference (types inferred from declaration):\n");
    // Test 3: Infer both param and return types from declaration
    __Closure__ * __sn__triple = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_2__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long))__sn__triple->fn)(__sn__triple, 7LL));
        char *_r = rt_str_concat(__local_arena__, "  triple(7) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Test 4: Infer multiple param types
    __Closure__ * __sn__multiply = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_3__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long, long long))__sn__multiply->fn)(__sn__multiply, 6LL, 8LL));
        char *_r = rt_str_concat(__local_arena__, "  multiply(6, 8) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Test 5: Mixed - explicit param type, inferred return
    __Closure__ * __sn__square = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_4__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long))__sn__square->fn)(__sn__square, 9LL));
        char *_r = rt_str_concat(__local_arena__, "  square(9) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Test 6: Mixed - inferred param type, explicit return
    __Closure__ * __sn__negate = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_5__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long))__sn__negate->fn)(__sn__negate, 42LL));
        char *_r = rt_str_concat(__local_arena__, "  negate(42) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // =========================================
    // Lambdas with modifiers
    // =========================================
    rt_print_string("\nLambdas with modifiers:\n");
    // Test 7: Shared lambda with inferred types
    __Closure__ * __sn__increment = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_6__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long))__sn__increment->fn)(__sn__increment, 99LL));
        char *_r = rt_str_concat(__local_arena__, "  increment(99) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // =========================================
    // Combining lambdas
    // =========================================
    rt_print_string("\nCombining lambdas:\n");
    // Test 8: Compose lambdas
    long long *__sn__x = (long long *)rt_arena_alloc(__local_arena__, sizeof(long long));
    *__sn__x = ((long long (*)(void *, long long))__sn__double_it->fn)(__sn__double_it, ((long long (*)(void *, long long, long long))__sn__add->fn)(__sn__add, 1LL, 2LL));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, (*__sn__x));
        char *_r = rt_str_concat(__local_arena__, "  double_it(add(1, 2)) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long __sn__y = ((long long (*)(void *, long long))__sn__triple->fn)(__sn__triple, ((long long (*)(void *, long long, long long))__sn__multiply->fn)(__sn__multiply, 2LL, 3LL));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__y);
        char *_r = rt_str_concat(__local_arena__, "  triple(multiply(2, 3)) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__demo_lambda_return;
__sn__demo_lambda_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_closure(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Closures                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // Test: Lambda with capture
    long long *__sn__multiplier = (long long *)rt_arena_alloc(__local_arena__, sizeof(long long));
    *__sn__multiplier = 3LL;
    __Closure__ * __sn__times_three = ({
    __closure_7__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__closure_7__));
    __cl__->fn = (void *)__lambda_7__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__closure_7__);
    __cl__->multiplier = __sn__multiplier;
    (__Closure__ *)__cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long (*)(void *, long long))__sn__times_three->fn)(__sn__times_three, 5LL));
        char *_r = rt_str_concat(__local_arena__, "times_three(5) = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__demo_closure_return;
__sn__demo_closure_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_bytes(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // ============================================================================
    // bytes.sn - Byte Type in Sindarin
    // ============================================================================
    // Demonstrates: The byte primitive type for handling raw 8-bit unsigned values.
    // Bytes are useful for binary data, file I/O, and low-level operations.
    //
    // Key features:
    //   - Range: 0 to 255 (8-bit unsigned)
    //   - Conversion to/from int (implicit)
    //   - Array operations with toHex(), toString(), etc.
    // ============================================================================
    // Entry point for byte demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin Byte Type                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_byte_basics", __args, 0, __thunk_16);
    } else {
        __sn__show_byte_basics(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_byte_values", __args, 0, __thunk_17);
    } else {
        __sn__show_byte_values(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_byte_conversions", __args, 0, __thunk_18);
    } else {
        __sn__show_byte_conversions(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_byte_arrays", __args, 0, __thunk_19);
    } else {
        __sn__show_byte_arrays(__local_arena__);
    }
    (void)0;
});
    goto __sn__demo_bytes_return;
__sn__demo_bytes_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_byte_basics(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 1. Byte Basics
    // ----------------------------------------------------------------------------
    rt_print_string("--- 1. Byte Basics ---\n");
    // Declaration
    unsigned char __sn__zero = 0LL;
    unsigned char __sn__mid = 128LL;
    unsigned char __sn__max = 255LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__zero);
        char *_r = rt_str_concat(__local_arena__, "zero = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__mid);
        char *_r = rt_str_concat(__local_arena__, "mid = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__max);
        char *_r = rt_str_concat(__local_arena__, "max = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Comparisons
    rt_print_string("\nByte comparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_lt_long(__sn__zero, __sn__mid));
        char *_r = rt_str_concat(__local_arena__, "  0 < 128: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_lt_long(__sn__mid, __sn__max));
        char *_r = rt_str_concat(__local_arena__, "  128 < 255: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_long(__sn__max, __sn__max));
        char *_r = rt_str_concat(__local_arena__, "  255 == 255: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Equality
    unsigned char __sn__a = 100LL;
    unsigned char __sn__b = 100LL;
    unsigned char __sn__c = 200LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_long(__sn__a, __sn__b));
        char *_r = rt_str_concat(__local_arena__, "\n  a(100) == b(100): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_ne_long(__sn__a, __sn__c));
        char *_r = rt_str_concat(__local_arena__, "  a(100) != c(200): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto __sn__show_byte_basics_return;
__sn__show_byte_basics_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_byte_values(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 2. Byte Values
    // ----------------------------------------------------------------------------
    rt_print_string("--- 2. Byte Values ---\n");
    // Full range
    rt_print_string("Range values:\n");
    unsigned char __sn__dec0 = 0LL;
    unsigned char __sn__dec127 = 127LL;
    unsigned char __sn__dec128 = 128LL;
    unsigned char __sn__dec255 = 255LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__dec0);
        char *_r = rt_str_concat(__local_arena__, "  byte 0 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__dec127);
        char *_r = rt_str_concat(__local_arena__, "  byte 127 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__dec128);
        char *_r = rt_str_concat(__local_arena__, "  byte 128 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__dec255);
        char *_r = rt_str_concat(__local_arena__, "  byte 255 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Common ASCII byte values
    rt_print_string("\nCommon ASCII values:\n");
    unsigned char __sn__nullByte = 0LL;
    unsigned char __sn__space = 32LL;
    unsigned char __sn__letterA = 65LL;
    unsigned char __sn__letterZ = 90LL;
    unsigned char __sn__letterALower = 97LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__nullByte);
        char *_r = rt_str_concat(__local_arena__, "  NULL = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__space);
        char *_r = rt_str_concat(__local_arena__, "  Space = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__letterA);
        char *_r = rt_str_concat(__local_arena__, "  'A' = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__letterZ);
        char *_r = rt_str_concat(__local_arena__, "  'Z' = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__letterALower);
        char *_r = rt_str_concat(__local_arena__, "  'a' = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto __sn__show_byte_values_return;
__sn__show_byte_values_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_byte_conversions(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 3. Byte Conversions
    // ----------------------------------------------------------------------------
    rt_print_string("--- 3. Byte Conversions ---\n");
    // Byte to int conversion (implicit)
    rt_print_string("Byte to int (implicit):\n");
    unsigned char __sn__b1 = 42LL;
    long long __sn__i1 = __sn__b1;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i1);
        char *_r = rt_str_concat(__local_arena__, "  byte 42 -> int: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    unsigned char __sn__b2 = 255LL;
    long long __sn__i2 = __sn__b2;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i2);
        char *_r = rt_str_concat(__local_arena__, "  byte 255 -> int: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Arithmetic with bytes - result is int
    rt_print_string("\nArithmetic with bytes:\n");
    unsigned char __sn__x = 100LL;
    unsigned char __sn__y = 50LL;
    long long __sn__sum = rt_add_long(__sn__x, __sn__y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_byte(__local_arena__, __sn__y);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__sum);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " + ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long __sn__diff = rt_sub_long(__sn__x, __sn__y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, __sn__x);
        char *_p1 = rt_to_string_byte(__local_arena__, __sn__y);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__diff);
        char *_r = rt_str_concat(__local_arena__, "  ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " - ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Large result (exceeds byte range)
    rt_print_string("\nLarge results:\n");
    unsigned char __sn__big1 = 200LL;
    unsigned char __sn__big2 = 200LL;
    long long __sn__bigSum = rt_add_long(__sn__big1, __sn__big2);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__bigSum);
        char *_r = rt_str_concat(__local_arena__, "  200 + 200 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " (exceeds 255, int handles it)\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto __sn__show_byte_conversions_return;
__sn__show_byte_conversions_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__show_byte_arrays(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 4. Byte Arrays
    // ----------------------------------------------------------------------------
    rt_print_string("--- 4. Byte Arrays ---\n");
    // Create a byte array using curly braces
    rt_print_string("Creating byte arrays:\n");
    RtHandle __sn__data = rt_array_create_byte_h(__local_arena__, 5, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))));
        char *_r = rt_str_concat(__local_arena__, "  Array length: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("  Contents (ASCII for 'Hello'):\n");
    {
        long long * __arr_11__ = rt_array_range(__local_arena__, 0LL, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))));
        long __len_11__ = rt_array_length(__arr_11__);
        for (long __idx_11__ = 0; __idx_11__ < __len_11__; __idx_11__++) {
            long long __sn__i = __arr_11__[__idx_11__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_p1 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))) + (__sn__i) : (__sn__i)]);
        char *_r = rt_str_concat(__local_arena__, "    [", _p0);
        _r = rt_str_concat(__local_arena__, _r, "] = ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\nModifying byte array:\n");
    (((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[0LL] = 74LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[0LL]);
        char *_r = rt_str_concat(__local_arena__, "  Changed first byte to 74 (J): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Create from decimal values
    rt_print_string("\nByte array from decimal:\n");
    RtHandle __sn__nums = rt_array_create_byte_h(__local_arena__, 5, (unsigned char[]){0LL, 64LL, 128LL, 192LL, 255LL});
    rt_print_string("  Values: ");
    {
        long long * __arr_12__ = rt_array_range(__local_arena__, 0LL, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__nums))));
        long __len_12__ = rt_array_length(__arr_12__);
        for (long __idx_12__ = 0; __idx_12__ < __len_12__; __idx_12__++) {
            long long __sn__i = __arr_12__[__idx_12__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__nums))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__nums))) + (__sn__i) : (__sn__i)]);
        rt_str_concat(__local_arena__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n");
    // Convert to string and hex
    rt_print_string("\nByte array conversions:\n");
    RtHandle __sn__hello = rt_array_create_byte_h(__local_arena__, 5, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL});
    RtHandle __sn__helloStr = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_byte_array_to_string(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__hello))));
    RtHandle __sn__helloHex = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_byte_array_to_hex(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__hello))));
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  toString(): \"", (char *)rt_managed_pin(__local_arena__, __sn__helloStr));
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "  toHex(): ", (char *)rt_managed_pin(__local_arena__, __sn__helloHex));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto __sn__show_byte_arrays_return;
__sn__show_byte_arrays_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_fileio(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // Entry point for file I/O demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin File I/O                          │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_textfile", __args, 0, __thunk_20);
    } else {
        __sn__demo_textfile(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_binaryfile", __args, 0, __thunk_21);
    } else {
        __sn__demo_binaryfile(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_file_utilities", __args, 0, __thunk_22);
    } else {
        __sn__demo_file_utilities(__local_arena__);
    }
    (void)0;
});
    goto __sn__demo_fileio_return;
__sn__demo_fileio_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_textfile(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 1. TextFile Operations
    // ----------------------------------------------------------------------------
    rt_print_string("--- 1. TextFile Operations ---\n");
    // Write entire content at once (static method)
    rt_print_string("Writing entire content at once:\n");
    __sn__TextFile_writeAll(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt"), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello from Sindarin!\nLine 2\nLine 3"));
    rt_print_string("  Wrote 3 lines to /tmp/sindarin_demo.txt\n");
    // Read entire file at once (static method)
    rt_print_string("\nReading entire file at once:\n");
    RtHandle __sn__content = __sn__TextFile_readAll(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt"));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_str_length((char *)rt_managed_pin(__local_arena__, __sn__content)));
        char *_r = rt_str_concat(__local_arena__, "  Content length: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " characters\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Open and read line by line
    rt_print_string("\nReading the file line by line:\n");
    RtTextFile * __sn__reader = __sn__TextFile_open(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt"));
    long long __sn__lineNum = 1LL;
    while (rt_not_bool(sn_text_file_is_eof(__sn__reader))) {
        {
            RtHandle __sn__line = __sn__TextFile_readLine(__local_arena__, __sn__reader);
            if (rt_gt_long(rt_str_length((char *)rt_managed_pin(__local_arena__, __sn__line)), 0LL)) {
                {
                    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__lineNum);
        char *_r = rt_str_concat(__local_arena__, "  Line ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ": ");
        _r = rt_str_concat(__local_arena__, _r, (char *)rt_managed_pin(__local_arena__, __sn__line));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                    rt_post_inc_long(&__sn__lineNum);
                }
            }
        }
    }
    sn_text_file_close(__sn__reader);
    // Read all lines into array
    rt_print_string("\nReading all lines into array:\n");
    RtTextFile * __sn__reader2 = __sn__TextFile_open(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt"));
    RtHandle __sn__lines = __sn__TextFile_readLines(__local_arena__, __sn__reader2);
    sn_text_file_close(__sn__reader2);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__lines))));
        char *_r = rt_str_concat(__local_arena__, "  Got ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " lines\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Check file existence
    rt_print_string("\nFile existence:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt")));
        char *_r = rt_str_concat(__local_arena__, "  /tmp/sindarin_demo.txt exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/nonexistent.txt")));
        char *_r = rt_str_concat(__local_arena__, "  /tmp/nonexistent.txt exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Cleanup
    __sn__TextFile_delete(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.txt"));
    rt_print_string("\n");
    goto __sn__demo_textfile_return;
__sn__demo_textfile_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_binaryfile(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 2. BinaryFile Operations
    // ----------------------------------------------------------------------------
    rt_print_string("--- 2. BinaryFile Operations ---\n");
    // Write bytes using static method
    rt_print_string("Writing bytes:\n");
    RtHandle __sn__bytes = rt_array_create_byte_h(__local_arena__, 4, (unsigned char[]){255LL, 66LL, 0LL, 171LL});
    __sn__BinaryFile_writeAll(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.bin"), __sn__bytes);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__bytes))));
        char *_r = rt_str_concat(__local_arena__, "  Wrote ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " bytes: 255, 66, 0, 171\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Read back the binary file
    rt_print_string("\nReading binary file:\n");
    RtHandle __sn__readBytes = __sn__BinaryFile_readAll(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.bin"));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__readBytes))));
        char *_r = rt_str_concat(__local_arena__, "  Read ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " bytes\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__readBytes))[0LL]);
        char *_p1 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__readBytes))[1LL]);
        char *_p2 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__readBytes))[2LL]);
        char *_p3 = rt_to_string_byte(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__readBytes))[3LL]);
        char *_r = rt_str_concat(__local_arena__, "  Values: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p3);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Open and read byte by byte
    rt_print_string("\nReading byte by byte:\n");
    RtBinaryFile * __sn__reader = __sn__BinaryFile_open(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.bin"));
    long long __sn__b1 = sn_binary_file_read_byte(__sn__reader);
    long long __sn__b2 = sn_binary_file_read_byte(__sn__reader);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__b1);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__b2);
        char *_r = rt_str_concat(__local_arena__, "  First two bytes: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    sn_binary_file_close(__sn__reader);
    // Check file existence
    rt_print_string("\nBinary file existence:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__BinaryFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.bin")));
        char *_r = rt_str_concat(__local_arena__, "  /tmp/sindarin_demo.bin exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Cleanup
    __sn__BinaryFile_delete(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/sindarin_demo.bin"));
    rt_print_string("\n");
    goto __sn__demo_binaryfile_return;
__sn__demo_binaryfile_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_file_utilities(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // 3. File Utility Functions
    // ----------------------------------------------------------------------------
    rt_print_string("--- 3. File Utilities ---\n");
    // Create a file with specific content
    rt_print_string("Common file operations:\n");
    __sn__TextFile_writeAll(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_test.txt"), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Test content\nLine 2\nLine 3"));
    // Check existence
    RtHandle __sn__path = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_test.txt");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, __sn__path));
        char *_r = rt_str_concat(__local_arena__, "  File exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Read and process
    RtHandle __sn__fileContent = __sn__TextFile_readAll(__local_arena__, __sn__path);
    RtHandle __sn__contentLines = rt_array_from_raw_strings_h(__local_arena__, RT_HANDLE_NULL, rt_str_split_lines(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__fileContent)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__contentLines))));
        char *_r = rt_str_concat(__local_arena__, "  Number of lines: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Copy and move files
    rt_print_string("\nCopy and move:\n");
    __sn__TextFile_copy(__local_arena__, __sn__path, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_copy.txt"));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_copy.txt")));
        char *_r = rt_str_concat(__local_arena__, "  Copied file exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    __sn__TextFile_move(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_copy.txt"), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_moved.txt"));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_copy.txt")));
        char *_r = rt_str_concat(__local_arena__, "  Original copy exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__TextFile_exists(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_moved.txt")));
        char *_r = rt_str_concat(__local_arena__, "  Moved file exists: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Cleanup
    __sn__TextFile_delete(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_test.txt"));
    __sn__TextFile_delete(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "/tmp/utility_moved.txt"));
    rt_print_string("\n");
    goto __sn__demo_file_utilities_return;
__sn__demo_file_utilities_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_date(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // Entry point for date demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                         Sindarin Date                            │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. Creating dates
    rt_print_string("--- Creating Dates ---\n");
    RtDate * __sn__today = __sn__Date_today(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__today));
        char *_r = rt_str_concat(__local_arena__, "Today: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__christmas = __sn__Date_fromYmd(__local_arena__, 2025LL, 12LL, 25LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__christmas));
        char *_r = rt_str_concat(__local_arena__, "Christmas: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__parsed = __sn__Date_fromString(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "2025-07-04"));
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__parsed));
        char *_r = rt_str_concat(__local_arena__, "Parsed: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__fromEpoch = __sn__Date_fromEpochDays(__local_arena__, 20088LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__fromEpoch));
        char *_r = rt_str_concat(__local_arena__, "From epoch days: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 2. Date components
    rt_print_string("\n--- Date Components ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_year(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Year: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_month(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Month: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_day(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Day: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_weekday(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Weekday: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_day_of_year(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Day of year: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_epoch_days(__sn__today));
        char *_r = rt_str_concat(__local_arena__, "Epoch days: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 3. Weekday names
    rt_print_string("\n--- Weekday Names ---\n");
    RtHandle __sn__names = rt_array_create_string_h(__local_arena__, 7, (char *[]){"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Today is ", ((char *)rt_managed_pin(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__names))[(sn_date_get_weekday(__sn__today)) < 0 ? rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__names))) + (sn_date_get_weekday(__sn__today)) : (sn_date_get_weekday(__sn__today))])));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 4. Formatting
    rt_print_string("\n--- Formatting ---\n");
    RtDate * __sn__d = __sn__Date_fromYmd(__local_arena__, 2025LL, 12LL, 25LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__d));
        char *_r = rt_str_concat(__local_arena__, "ISO: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toString(__local_arena__, __sn__d));
        char *_r = rt_str_concat(__local_arena__, "toString: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_format(__local_arena__, __sn__d, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD")));
        char *_r = rt_str_concat(__local_arena__, "YYYY-MM-DD: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_format(__local_arena__, __sn__d, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "M/D/YYYY")));
        char *_r = rt_str_concat(__local_arena__, "M/D/YYYY: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_format(__local_arena__, __sn__d, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "DD/MM/YYYY")));
        char *_r = rt_str_concat(__local_arena__, "DD/MM/YYYY: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_format(__local_arena__, __sn__d, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "MMMM D, YYYY")));
        char *_r = rt_str_concat(__local_arena__, "MMMM D, YYYY: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_format(__local_arena__, __sn__d, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "ddd, MMM D")));
        char *_r = rt_str_concat(__local_arena__, "ddd, MMM D: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 5. Date arithmetic
    rt_print_string("\n--- Date Arithmetic ---\n");
    RtDate * __sn__start = __sn__Date_fromYmd(__local_arena__, 2025LL, 1LL, 15LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__start));
        char *_r = rt_str_concat(__local_arena__, "Start: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addDays(__local_arena__, __sn__start, 10LL)));
        char *_r = rt_str_concat(__local_arena__, "addDays(10): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addDays(__local_arena__, __sn__start, -5LL)));
        char *_r = rt_str_concat(__local_arena__, "addDays(-5): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addWeeks(__local_arena__, __sn__start, 2LL)));
        char *_r = rt_str_concat(__local_arena__, "addWeeks(2): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addMonths(__local_arena__, __sn__start, 3LL)));
        char *_r = rt_str_concat(__local_arena__, "addMonths(3): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addYears(__local_arena__, __sn__start, 1LL)));
        char *_r = rt_str_concat(__local_arena__, "addYears(1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 6. Month boundaries
    rt_print_string("\n--- Month Boundaries ---\n");
    RtDate * __sn__jan31 = __sn__Date_fromYmd(__local_arena__, 2025LL, 1LL, 31LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__jan31));
        char *_r = rt_str_concat(__local_arena__, "Jan 31: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addMonths(__local_arena__, __sn__jan31, 1LL)));
        char *_r = rt_str_concat(__local_arena__, "addMonths(1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__leapDay = __sn__Date_fromYmd(__local_arena__, 2024LL, 2LL, 29LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__leapDay));
        char *_r = rt_str_concat(__local_arena__, "Leap day 2024: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_addYears(__local_arena__, __sn__leapDay, 1LL)));
        char *_r = rt_str_concat(__local_arena__, "addYears(1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 7. Difference between dates
    rt_print_string("\n--- Date Differences ---\n");
    RtDate * __sn__d1 = __sn__Date_fromYmd(__local_arena__, 2025LL, 1LL, 1LL);
    RtDate * __sn__d2 = __sn__Date_fromYmd(__local_arena__, 2025LL, 12LL, 31LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_diff_days(__sn__d2, __sn__d1));
        char *_r = rt_str_concat(__local_arena__, "Days in 2025: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__birthday = __sn__Date_fromYmd(__local_arena__, 2025LL, 6LL, 15LL);
    long long __sn__daysUntil = sn_date_diff_days(__sn__birthday, __sn__today);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__daysUntil);
        char *_r = rt_str_concat(__local_arena__, "Days until Jun 15: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 8. Start/end of month and year
    rt_print_string("\n--- Start/End Methods ---\n");
    RtDate * __sn__mid = __sn__Date_fromYmd(__local_arena__, 2025LL, 6LL, 15LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__mid));
        char *_r = rt_str_concat(__local_arena__, "Date: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_startOfMonth(__local_arena__, __sn__mid)));
        char *_r = rt_str_concat(__local_arena__, "startOfMonth: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_endOfMonth(__local_arena__, __sn__mid)));
        char *_r = rt_str_concat(__local_arena__, "endOfMonth: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_startOfYear(__local_arena__, __sn__mid)));
        char *_r = rt_str_concat(__local_arena__, "startOfYear: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Date_toIso(__local_arena__, __sn__Date_endOfYear(__local_arena__, __sn__mid)));
        char *_r = rt_str_concat(__local_arena__, "endOfYear: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 9. Comparisons
    rt_print_string("\n--- Comparisons ---\n");
    RtDate * __sn__earlier = __sn__Date_fromYmd(__local_arena__, 2025LL, 1LL, 1LL);
    RtDate * __sn__later = __sn__Date_fromYmd(__local_arena__, 2025LL, 12LL, 31LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_date_is_before(__sn__earlier, __sn__later));
        char *_r = rt_str_concat(__local_arena__, "Jan 1 isBefore Dec 31: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_date_is_after(__sn__later, __sn__earlier));
        char *_r = rt_str_concat(__local_arena__, "Dec 31 isAfter Jan 1: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__same1 = __sn__Date_fromYmd(__local_arena__, 2025LL, 6LL, 15LL);
    RtDate * __sn__same2 = __sn__Date_fromString(__local_arena__, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "2025-06-15"));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_date_equals(__sn__same1, __sn__same2));
        char *_r = rt_str_concat(__local_arena__, "equals: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 10. Weekend/weekday checks
    rt_print_string("\n--- Weekend/Weekday ---\n");
    if (sn_date_is_weekend(__sn__today)) {
        {
            rt_print_string("Today is a weekend!\n");
        }
    }
    else {
        {
            rt_print_string("Today is a weekday\n");
        }
    }
    rt_print_string("\n--- Leap Year & Days in Month ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__Date_isLeapYear(__local_arena__, 2024LL));
        char *_r = rt_str_concat(__local_arena__, "2024 is leap year: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, __sn__Date_isLeapYear(__local_arena__, 2025LL));
        char *_r = rt_str_concat(__local_arena__, "2025 is leap year: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__Date_daysInMonth(__local_arena__, 2024LL, 2LL));
        char *_r = rt_str_concat(__local_arena__, "Days in Feb 2024: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__Date_daysInMonth(__local_arena__, 2025LL, 2LL));
        char *_r = rt_str_concat(__local_arena__, "Days in Feb 2025: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * __sn__feb2024 = __sn__Date_fromYmd(__local_arena__, 2024LL, 2LL, 15LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_date_is_leap(__sn__feb2024));
        char *_r = rt_str_concat(__local_arena__, "Feb 2024 isLeapYear: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_date_get_days_in_month(__sn__feb2024));
        char *_r = rt_str_concat(__local_arena__, "Feb 2024 daysInMonth: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Date Demo Complete ---\n");
    goto __sn__demo_date_return;
__sn__demo_date_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__demo_time(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    // Entry point for time demos
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                         Sindarin Time                            │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    // 1. Creating times
    rt_print_string("--- Creating Times ---\n");
    RtTime * __sn__now = __sn__Time_now(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_toIso(__local_arena__, __sn__now));
        char *_r = rt_str_concat(__local_arena__, "Now (local): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * __sn__utc = __sn__Time_utc(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_toIso(__local_arena__, __sn__utc));
        char *_r = rt_str_concat(__local_arena__, "Now (UTC): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * __sn__fromMs = __sn__Time_fromMillis(__local_arena__, 1735500000000LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_toIso(__local_arena__, __sn__fromMs));
        char *_r = rt_str_concat(__local_arena__, "From millis: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * __sn__fromSec = __sn__Time_fromSeconds(__local_arena__, 1735500000LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_toIso(__local_arena__, __sn__fromSec));
        char *_r = rt_str_concat(__local_arena__, "From seconds: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 2. Time components
    rt_print_string("\n--- Time Components ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_year(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Year: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_month(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Month: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_day(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Day: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_hour(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Hour: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_minute(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Minute: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_second(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Second: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_weekday(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Weekday: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_millis(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Millis since epoch: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, sn_time_get_seconds(__sn__now));
        char *_r = rt_str_concat(__local_arena__, "Seconds since epoch: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 3. Weekday names
    rt_print_string("\n--- Weekday Names ---\n");
    RtHandle __sn__names = rt_array_create_string_h(__local_arena__, 7, (char *[]){"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Today is ", ((char *)rt_managed_pin(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__names))[(sn_time_get_weekday(__sn__now)) < 0 ? rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__names))) + (sn_time_get_weekday(__sn__now)) : (sn_time_get_weekday(__sn__now))])));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 4. Formatting
    rt_print_string("\n--- Formatting ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_toIso(__local_arena__, __sn__now));
        char *_r = rt_str_concat(__local_arena__, "ISO: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD")));
        char *_r = rt_str_concat(__local_arena__, "Date only: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "Time only: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD")));
        char *_r = rt_str_concat(__local_arena__, "YYYY-MM-DD: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "HH:mm:ss: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "YYYY-MM-DD HH:mm:ss: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "M/D/YYYY")));
        char *_r = rt_str_concat(__local_arena__, "M/D/YYYY: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "h:mm A")));
        char *_r = rt_str_concat(__local_arena__, "h:mm A: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__now, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "h:mm:ss a")));
        char *_r = rt_str_concat(__local_arena__, "h:mm:ss a: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 5. Time arithmetic
    rt_print_string("\n--- Time Arithmetic ---\n");
    RtTime * __sn__base = __sn__Time_now(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__base, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "Now: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_add(__local_arena__, __sn__base, 5000LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss.SSS")));
        char *_r = rt_str_concat(__local_arena__, "add(5000): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addSeconds(__local_arena__, __sn__base, 30LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "addSeconds(30): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addMinutes(__local_arena__, __sn__base, 15LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "addMinutes(15): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addHours(__local_arena__, __sn__base, 2LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "addHours(2): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addDays(__local_arena__, __sn__base, 1LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "addDays(1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // Subtraction with negative values
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addHours(__local_arena__, __sn__base, -1LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "addHours(-1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__Time_addDays(__local_arena__, __sn__base, -7LL), rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD")));
        char *_r = rt_str_concat(__local_arena__, "addDays(-7): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 6. Measuring elapsed time
    rt_print_string("\n--- Elapsed Time ---\n");
    RtTime * __sn__start = __sn__Time_now(__local_arena__);
    // Simulate some work
    long long __sn__sum = 0LL;
    {
        long long __sn__i = 0LL;
        while (rt_lt_long(__sn__i, 10000LL)) {
            {
                (__sn__sum = rt_add_long(__sn__sum, __sn__i));
            }
        __for_continue_10__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    long long __sn__elapsed = __sn__Time_diff(__local_arena__, __sn__Time_now(__local_arena__), __sn__start);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__elapsed);
        char *_r = rt_str_concat(__local_arena__, "Loop completed in ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 7. Time differences
    rt_print_string("\n--- Time Differences ---\n");
    RtTime * __sn__t1 = __sn__Time_now(__local_arena__);
    __sn__Time_sleep(__local_arena__, 50LL);
    RtTime * __sn__t2 = __sn__Time_now(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__Time_diff(__local_arena__, __sn__t2, __sn__t1));
        char *_r = rt_str_concat(__local_arena__, "t2.diff(t1): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__Time_diff(__local_arena__, __sn__t1, __sn__t2));
        char *_r = rt_str_concat(__local_arena__, "t1.diff(t2): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 8. Comparisons
    rt_print_string("\n--- Comparisons ---\n");
    RtTime * __sn__earlier = __sn__Time_fromMillis(__local_arena__, 1735500000000LL);
    RtTime * __sn__later = __sn__Time_fromMillis(__local_arena__, 1735500001000LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_time_is_before(__sn__earlier, __sn__later));
        char *_r = rt_str_concat(__local_arena__, "earlier isBefore later: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_time_is_after(__sn__later, __sn__earlier));
        char *_r = rt_str_concat(__local_arena__, "later isAfter earlier: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * __sn__same1 = __sn__Time_fromMillis(__local_arena__, 1735500000000LL);
    RtTime * __sn__same2 = __sn__Time_fromMillis(__local_arena__, 1735500000000LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, sn_time_equals(__sn__same1, __sn__same2));
        char *_r = rt_str_concat(__local_arena__, "equals: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 9. Sleep
    rt_print_string("\n--- Sleep ---\n");
    rt_print_string("Sleeping for 100ms...\n");
    RtTime * __sn__sleepStart = __sn__Time_now(__local_arena__);
    __sn__Time_sleep(__local_arena__, 100LL);
    long long __sn__sleepElapsed = __sn__Time_diff(__local_arena__, __sn__Time_now(__local_arena__), __sn__sleepStart);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__sleepElapsed);
        char *_r = rt_str_concat(__local_arena__, "Slept for ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 10. Timestamps for logging
    rt_print_string("\n--- Timestamps ---\n");
    RtTime * __sn__timestamp = __sn__Time_now(__local_arena__);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__timestamp, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYY-MM-DD HH:mm:ss")));
        char *_r = rt_str_concat(__local_arena__, "[", _p0);
        _r = rt_str_concat(__local_arena__, _r, "] Event occurred\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__timestamp, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "HH:mm:ss.SSS")));
        char *_r = rt_str_concat(__local_arena__, "[", _p0);
        _r = rt_str_concat(__local_arena__, _r, "] Precise timestamp\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 11. File naming with timestamps
    rt_print_string("\n--- File Naming ---\n");
    RtTime * __sn__fileTime = __sn__Time_now(__local_arena__);
    RtHandle __sn__filename = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__fileTime, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "YYYYMMDD_HHmmss")));
        char *_r = rt_str_concat(__local_arena__, "backup_", _p0);
        _r = rt_str_concat(__local_arena__, _r, ".txt");
        rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, _r);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Generated filename: ", (char *)rt_managed_pin(__local_arena__, __sn__filename));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    // 12. Scheduling future events
    rt_print_string("\n--- Future Events ---\n");
    RtTime * __sn__eventNow = __sn__Time_now(__local_arena__);
    RtTime * __sn__eventTime = __sn__Time_addMinutes(__local_arena__, __sn__Time_addHours(__local_arena__, __sn__eventNow, 2LL), 30LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = (char *)rt_managed_pin(__local_arena__, __sn__Time_format(__local_arena__, __sn__eventTime, rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "h:mm A")));
        char *_r = rt_str_concat(__local_arena__, "Event scheduled for: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long __sn__waitMs = __sn__Time_diff(__local_arena__, __sn__eventTime, __sn__eventNow);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_div_long(rt_div_long(__sn__waitMs, 1000LL), 60LL));
        char *_r = rt_str_concat(__local_arena__, "Time until event: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, " minutes\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__demo_time_return;
__sn__demo_time_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_print_string("╔══════════════════════════════════════════════════════════════════╗\n");
    rt_print_string("║           Welcome to the Sindarin Language Demo                  ║\n");
    rt_print_string("╚══════════════════════════════════════════════════════════════════╝\n\n");
    // Run each feature demo
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_types", __args, 0, __thunk_23);
    } else {
        __sn__demo_types(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_loops", __args, 0, __thunk_24);
    } else {
        __sn__demo_loops(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_conditionals", __args, 0, __thunk_25);
    } else {
        __sn__demo_conditionals(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_strings", __args, 0, __thunk_26);
    } else {
        __sn__demo_strings(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_functions", __args, 0, __thunk_27);
    } else {
        __sn__demo_functions(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_arrays", __args, 0, __thunk_28);
    } else {
        __sn__demo_arrays(__local_arena__);
    }
    (void)0;
});
    ((void (*)(void *))__sn__demo_memory->fn)(__sn__demo_memory);
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_lambda", __args, 0, __thunk_29);
    } else {
        __sn__demo_lambda(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_closure", __args, 0, __thunk_30);
    } else {
        __sn__demo_closure(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_bytes", __args, 0, __thunk_31);
    } else {
        __sn__demo_bytes(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_fileio", __args, 0, __thunk_32);
    } else {
        __sn__demo_fileio(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_date", __args, 0, __thunk_33);
    } else {
        __sn__demo_date(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("demo_time", __args, 0, __thunk_34);
    } else {
        __sn__demo_time(__local_arena__);
    }
    (void)0;
});
    rt_print_string("╔══════════════════════════════════════════════════════════════════╗\n");
    rt_print_string("║                    All Demos Complete!                           ║\n");
    rt_print_string("╚══════════════════════════════════════════════════════════════════╝\n");
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

RtTextFile * __sn__TextFile_open(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    RtTextFile * _return_value = NULL;
    _return_value = sn_text_file_open(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__path));
    goto __sn__TextFile_open_return;
__sn__TextFile_open_return:
    return _return_value;
}

bool __sn__TextFile_exists(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    bool _return_value = 0;
    _return_value = rt_ne_long(sn_text_file_exists((char *)rt_managed_pin(__caller_arena__, __sn__path)), 0LL);
    goto __sn__TextFile_exists_return;
__sn__TextFile_exists_return:
    return _return_value;
}

RtHandle __sn__TextFile_readAll(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_read_all_static(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__path));
    goto __sn__TextFile_readAll_return;
__sn__TextFile_readAll_return:
    return _return_value;
}

void __sn__TextFile_writeAll(RtManagedArena *__caller_arena__, RtHandle __sn__path, RtHandle __sn__content) {
    sn_text_file_write_all_static((char *)rt_managed_pin(__caller_arena__, __sn__path), (char *)rt_managed_pin(__caller_arena__, __sn__content));
__sn__TextFile_writeAll_return:
    return;
}

void __sn__TextFile_copy(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    sn_text_file_copy((char *)rt_managed_pin(__caller_arena__, __sn__source), (char *)rt_managed_pin(__caller_arena__, __sn__dest));
__sn__TextFile_copy_return:
    return;
}

void __sn__TextFile_move(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    sn_text_file_move((char *)rt_managed_pin(__caller_arena__, __sn__source), (char *)rt_managed_pin(__caller_arena__, __sn__dest));
__sn__TextFile_move_return:
    return;
}

void __sn__TextFile_delete(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    sn_text_file_delete((char *)rt_managed_pin(__caller_arena__, __sn__path));
__sn__TextFile_delete_return:
    return;
}

RtHandle __sn__TextFile_readLine(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_read_line(__caller_arena__, __sn__self);
    goto __sn__TextFile_readLine_return;
__sn__TextFile_readLine_return:
    return _return_value;
}

RtHandle __sn__TextFile_readRemaining(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_read_remaining(__caller_arena__, __sn__self);
    goto __sn__TextFile_readRemaining_return;
__sn__TextFile_readRemaining_return:
    return _return_value;
}

RtHandle __sn__TextFile_readLines(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_read_lines(__caller_arena__, __sn__self);
    goto __sn__TextFile_readLines_return;
__sn__TextFile_readLines_return:
    return _return_value;
}

RtHandle __sn__TextFile_readWord(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_read_word(__caller_arena__, __sn__self);
    goto __sn__TextFile_readWord_return;
__sn__TextFile_readWord_return:
    return _return_value;
}

RtHandle __sn__TextFile_path(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_get_path(__caller_arena__, __sn__self);
    goto __sn__TextFile_path_return;
__sn__TextFile_path_return:
    return _return_value;
}

RtHandle __sn__TextFile_name(RtManagedArena *__caller_arena__, RtTextFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_text_file_get_name(__caller_arena__, __sn__self);
    goto __sn__TextFile_name_return;
__sn__TextFile_name_return:
    return _return_value;
}

RtBinaryFile * __sn__BinaryFile_open(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    RtBinaryFile * _return_value = NULL;
    _return_value = sn_binary_file_open(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__path));
    goto __sn__BinaryFile_open_return;
__sn__BinaryFile_open_return:
    return _return_value;
}

bool __sn__BinaryFile_exists(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    bool _return_value = 0;
    _return_value = rt_ne_long(sn_binary_file_exists((char *)rt_managed_pin(__caller_arena__, __sn__path)), 0LL);
    goto __sn__BinaryFile_exists_return;
__sn__BinaryFile_exists_return:
    return _return_value;
}

RtHandle __sn__BinaryFile_readAll(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_binary_file_read_all_static(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__path));
    goto __sn__BinaryFile_readAll_return;
__sn__BinaryFile_readAll_return:
    return _return_value;
}

void __sn__BinaryFile_writeAll(RtManagedArena *__caller_arena__, RtHandle __sn__path, RtHandle __sn__data) {
    sn_binary_file_write_all_static((char *)rt_managed_pin(__caller_arena__, __sn__path), ((unsigned char *)rt_managed_pin_array(__caller_arena__, __sn__data)));
__sn__BinaryFile_writeAll_return:
    return;
}

void __sn__BinaryFile_copy(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    sn_binary_file_copy((char *)rt_managed_pin(__caller_arena__, __sn__source), (char *)rt_managed_pin(__caller_arena__, __sn__dest));
__sn__BinaryFile_copy_return:
    return;
}

void __sn__BinaryFile_move(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    sn_binary_file_move((char *)rt_managed_pin(__caller_arena__, __sn__source), (char *)rt_managed_pin(__caller_arena__, __sn__dest));
__sn__BinaryFile_move_return:
    return;
}

void __sn__BinaryFile_delete(RtManagedArena *__caller_arena__, RtHandle __sn__path) {
    sn_binary_file_delete((char *)rt_managed_pin(__caller_arena__, __sn__path));
__sn__BinaryFile_delete_return:
    return;
}

RtHandle __sn__BinaryFile_readBytes(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self, long long __sn__count) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_binary_file_read_bytes(__caller_arena__, __sn__self, __sn__count);
    goto __sn__BinaryFile_readBytes_return;
__sn__BinaryFile_readBytes_return:
    return _return_value;
}

RtHandle __sn__BinaryFile_readRemaining(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_binary_file_read_remaining(__caller_arena__, __sn__self);
    goto __sn__BinaryFile_readRemaining_return;
__sn__BinaryFile_readRemaining_return:
    return _return_value;
}

RtHandle __sn__BinaryFile_path(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_binary_file_get_path(__caller_arena__, __sn__self);
    goto __sn__BinaryFile_path_return;
__sn__BinaryFile_path_return:
    return _return_value;
}

RtHandle __sn__BinaryFile_name(RtManagedArena *__caller_arena__, RtBinaryFile *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_binary_file_get_name(__caller_arena__, __sn__self);
    goto __sn__BinaryFile_name_return;
__sn__BinaryFile_name_return:
    return _return_value;
}

RtDate * __sn__Date_today(RtManagedArena *__caller_arena__) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_today(__caller_arena__);
    goto __sn__Date_today_return;
__sn__Date_today_return:
    return _return_value;
}

RtDate * __sn__Date_fromYmd(RtManagedArena *__caller_arena__, long long __sn__year, long long __sn__month, long long __sn__day) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_from_ymd(__caller_arena__, __sn__year, __sn__month, __sn__day);
    goto __sn__Date_fromYmd_return;
__sn__Date_fromYmd_return:
    return _return_value;
}

RtDate * __sn__Date_fromString(RtManagedArena *__caller_arena__, RtHandle __sn__s) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_from_string(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__s));
    goto __sn__Date_fromString_return;
__sn__Date_fromString_return:
    return _return_value;
}

RtDate * __sn__Date_fromEpochDays(RtManagedArena *__caller_arena__, long long __sn__days) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_from_epoch_days(__caller_arena__, __sn__days);
    goto __sn__Date_fromEpochDays_return;
__sn__Date_fromEpochDays_return:
    return _return_value;
}

bool __sn__Date_isLeapYear(RtManagedArena *__caller_arena__, long long __sn__year) {
    bool _return_value = 0;
    _return_value = rt_ne_long(sn_date_is_leap_year(__sn__year), 0LL);
    goto __sn__Date_isLeapYear_return;
__sn__Date_isLeapYear_return:
    return _return_value;
}

long long __sn__Date_daysInMonth(RtManagedArena *__caller_arena__, long long __sn__year, long long __sn__month) {
    long long _return_value = 0;
    _return_value = sn_date_days_in_month(__sn__year, __sn__month);
    goto __sn__Date_daysInMonth_return;
__sn__Date_daysInMonth_return:
    return _return_value;
}

RtHandle __sn__Date_format(RtManagedArena *__caller_arena__, RtDate *__sn__self, RtHandle __sn__pattern) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_date_format(__caller_arena__, __sn__self, (char *)rt_managed_pin(__caller_arena__, __sn__pattern));
    goto __sn__Date_format_return;
__sn__Date_format_return:
    return _return_value;
}

RtHandle __sn__Date_toIso(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_date_to_iso(__caller_arena__, __sn__self);
    goto __sn__Date_toIso_return;
__sn__Date_toIso_return:
    return _return_value;
}

RtHandle __sn__Date_toString(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_date_to_string(__caller_arena__, __sn__self);
    goto __sn__Date_toString_return;
__sn__Date_toString_return:
    return _return_value;
}

RtDate * __sn__Date_addDays(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__days) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_add_days(__caller_arena__, __sn__self, __sn__days);
    goto __sn__Date_addDays_return;
__sn__Date_addDays_return:
    return _return_value;
}

RtDate * __sn__Date_addWeeks(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__weeks) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_add_weeks(__caller_arena__, __sn__self, __sn__weeks);
    goto __sn__Date_addWeeks_return;
__sn__Date_addWeeks_return:
    return _return_value;
}

RtDate * __sn__Date_addMonths(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__months) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_add_months(__caller_arena__, __sn__self, __sn__months);
    goto __sn__Date_addMonths_return;
__sn__Date_addMonths_return:
    return _return_value;
}

RtDate * __sn__Date_addYears(RtManagedArena *__caller_arena__, RtDate *__sn__self, long long __sn__years) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_add_years(__caller_arena__, __sn__self, __sn__years);
    goto __sn__Date_addYears_return;
__sn__Date_addYears_return:
    return _return_value;
}

RtDate * __sn__Date_startOfMonth(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_start_of_month(__caller_arena__, __sn__self);
    goto __sn__Date_startOfMonth_return;
__sn__Date_startOfMonth_return:
    return _return_value;
}

RtDate * __sn__Date_endOfMonth(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_end_of_month(__caller_arena__, __sn__self);
    goto __sn__Date_endOfMonth_return;
__sn__Date_endOfMonth_return:
    return _return_value;
}

RtDate * __sn__Date_startOfYear(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_start_of_year(__caller_arena__, __sn__self);
    goto __sn__Date_startOfYear_return;
__sn__Date_startOfYear_return:
    return _return_value;
}

RtDate * __sn__Date_endOfYear(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    RtDate * _return_value = NULL;
    _return_value = sn_date_end_of_year(__caller_arena__, __sn__self);
    goto __sn__Date_endOfYear_return;
__sn__Date_endOfYear_return:
    return _return_value;
}

void* __sn__Date_toTime(RtManagedArena *__caller_arena__, RtDate *__sn__self) {
    void* _return_value = 0;
    _return_value = sn_date_to_time(__caller_arena__, __sn__self);
    goto __sn__Date_toTime_return;
__sn__Date_toTime_return:
    return _return_value;
}

RtTime * __sn__Time_now(RtManagedArena *__caller_arena__) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_now(__caller_arena__);
    goto __sn__Time_now_return;
__sn__Time_now_return:
    return _return_value;
}

RtTime * __sn__Time_utc(RtManagedArena *__caller_arena__) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_utc(__caller_arena__);
    goto __sn__Time_utc_return;
__sn__Time_utc_return:
    return _return_value;
}

RtTime * __sn__Time_fromMillis(RtManagedArena *__caller_arena__, long long __sn__ms) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_from_millis(__caller_arena__, __sn__ms);
    goto __sn__Time_fromMillis_return;
__sn__Time_fromMillis_return:
    return _return_value;
}

RtTime * __sn__Time_fromSeconds(RtManagedArena *__caller_arena__, long long __sn__s) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_from_seconds(__caller_arena__, __sn__s);
    goto __sn__Time_fromSeconds_return;
__sn__Time_fromSeconds_return:
    return _return_value;
}

void __sn__Time_sleep(RtManagedArena *__caller_arena__, long long __sn__ms) {
    sn_time_sleep(__sn__ms);
__sn__Time_sleep_return:
    return;
}

RtHandle __sn__Time_format(RtManagedArena *__caller_arena__, RtTime *__sn__self, RtHandle __sn__pattern) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_time_format(__caller_arena__, __sn__self, (char *)rt_managed_pin(__caller_arena__, __sn__pattern));
    goto __sn__Time_format_return;
__sn__Time_format_return:
    return _return_value;
}

RtHandle __sn__Time_toIso(RtManagedArena *__caller_arena__, RtTime *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_time_to_iso(__caller_arena__, __sn__self);
    goto __sn__Time_toIso_return;
__sn__Time_toIso_return:
    return _return_value;
}

RtHandle __sn__Time_toDate(RtManagedArena *__caller_arena__, RtTime *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_time_to_date(__caller_arena__, __sn__self);
    goto __sn__Time_toDate_return;
__sn__Time_toDate_return:
    return _return_value;
}

RtHandle __sn__Time_toTime(RtManagedArena *__caller_arena__, RtTime *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_time_to_time(__caller_arena__, __sn__self);
    goto __sn__Time_toTime_return;
__sn__Time_toTime_return:
    return _return_value;
}

RtTime * __sn__Time_add(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__ms) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_add(__caller_arena__, __sn__self, __sn__ms);
    goto __sn__Time_add_return;
__sn__Time_add_return:
    return _return_value;
}

RtTime * __sn__Time_addSeconds(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__seconds) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_add_seconds(__caller_arena__, __sn__self, __sn__seconds);
    goto __sn__Time_addSeconds_return;
__sn__Time_addSeconds_return:
    return _return_value;
}

RtTime * __sn__Time_addMinutes(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__minutes) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_add_minutes(__caller_arena__, __sn__self, __sn__minutes);
    goto __sn__Time_addMinutes_return;
__sn__Time_addMinutes_return:
    return _return_value;
}

RtTime * __sn__Time_addHours(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__hours) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_add_hours(__caller_arena__, __sn__self, __sn__hours);
    goto __sn__Time_addHours_return;
__sn__Time_addHours_return:
    return _return_value;
}

RtTime * __sn__Time_addDays(RtManagedArena *__caller_arena__, RtTime *__sn__self, long long __sn__days) {
    RtTime * _return_value = NULL;
    _return_value = sn_time_add_days(__caller_arena__, __sn__self, __sn__days);
    goto __sn__Time_addDays_return;
__sn__Time_addDays_return:
    return _return_value;
}

long long __sn__Time_diff(RtManagedArena *__caller_arena__, RtTime *__sn__self, RtTime * __sn__other) {
    long long _return_value = 0;
    _return_value = sn_time_diff(__sn__self, __sn__other);
    goto __sn__Time_diff_return;
__sn__Time_diff_return:
    return _return_value;
}


/* Lambda function definitions */
static long long __lambda_0__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_mul_long(__sn__x, 2LL);
}

static long long __lambda_1__(void *__closure__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_add_long(__sn__a, __sn__b);
}

static long long __lambda_2__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_mul_long(__sn__x, 3LL);
}

static long long __lambda_3__(void *__closure__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_mul_long(__sn__a, __sn__b);
}

static long long __lambda_4__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_mul_long(__sn__x, __sn__x);
}

static long long __lambda_5__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_sub_long(0LL, __sn__x);
}

static long long __lambda_6__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    return rt_add_long(__sn__x, 1LL);
}

static long long __lambda_7__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)((__Closure__ *)__closure__)->arena;
    long long *__sn__multiplier = ((__closure_7__ *)__closure__)->multiplier;
    return rt_mul_long(__sn__x, (*__sn__multiplier));
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    __sn__show_integers((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_1(void) {
    __sn__show_doubles((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_2(void) {
    __sn__show_strings((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_3(void) {
    __sn__show_chars((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_4(void) {
    __sn__show_booleans((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_5(void) {
    __sn__show_type_conversion((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_6(void) {
    __sn__show_while_loops((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_7(void) {
    __sn__show_for_loops((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_8(void) {
    __sn__show_foreach_loops((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_9(void) {
    __sn__show_break_continue((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_10(void) {
    __sn__show_nested_loops((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_11(void) {
    __sn__greet((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_12(void) {
    __sn__greet_person((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])));
    return rt_box_nil();
}

static RtAny __thunk_13(void) {
    __sn__greet_person((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])));
    return rt_box_nil();
}

static RtAny __thunk_14(void) {
    __sn__print_sum((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1]));
    return rt_box_nil();
}

static RtAny __thunk_15(void) {
    __sn__print_sum((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1]));
    return rt_box_nil();
}

static RtAny __thunk_16(void) {
    __sn__show_byte_basics((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_17(void) {
    __sn__show_byte_values((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_18(void) {
    __sn__show_byte_conversions((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_19(void) {
    __sn__show_byte_arrays((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_20(void) {
    __sn__demo_textfile((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_21(void) {
    __sn__demo_binaryfile((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_22(void) {
    __sn__demo_file_utilities((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_23(void) {
    __sn__demo_types((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_24(void) {
    __sn__demo_loops((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_25(void) {
    __sn__demo_conditionals((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_26(void) {
    __sn__demo_strings((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_27(void) {
    __sn__demo_functions((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_28(void) {
    __sn__demo_arrays((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_29(void) {
    __sn__demo_lambda((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_30(void) {
    __sn__demo_closure((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_31(void) {
    __sn__demo_bytes((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_32(void) {
    __sn__demo_fileio((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_33(void) {
    __sn__demo_date((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_34(void) {
    __sn__demo_time((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

