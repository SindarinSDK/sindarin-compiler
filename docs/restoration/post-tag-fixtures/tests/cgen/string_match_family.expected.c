#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Matcher (as val) */
typedef struct {
    char * __sn__text;
} __sn__Matcher;
/* Value operations */
static inline __sn__Matcher __sn__Matcher_copy(const __sn__Matcher *src) {
    __sn__Matcher dst;
    dst.__sn__text = src->__sn__text ? strdup(src->__sn__text) : NULL;
    return dst;
}

static inline void __sn__Matcher_cleanup(__sn__Matcher *p) {
    free(p->__sn__text);

}

#define sn_auto_Matcher __attribute__((cleanup(__sn__Matcher_cleanup)))

static inline void __sn__Matcher_cleanup_elem(void *p) { __sn__Matcher_cleanup((__sn__Matcher *)p); }
static inline void __sn__Matcher_copy_into(const void *src, void *dst) { *(__sn__Matcher *)dst = __sn__Matcher_copy((const __sn__Matcher *)src); }

/* Ref/pointer operations */
static inline __sn__Matcher *__sn__Matcher_alloc(void) {
    return calloc(1, sizeof(__sn__Matcher));
}

static inline void __sn__Matcher_release(__sn__Matcher **p) {
    if (*p) {
        free((*p)->__sn__text);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Matcher __attribute__((cleanup(__sn__Matcher_release)))

static inline void __sn__Matcher_release_elem(void *p) { __sn__Matcher_release((__sn__Matcher **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Matcher_to_string(const __sn__Matcher *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Matcher { ");
    off += snprintf(buf + off, sizeof(buf) - off, "text: ");
    off += snprintf(buf + off, sizeof(buf) - off, "\"%s\"", p->__sn__text ? p->__sn__text : "nil");
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



char * __sn__makeSubject(long long *);
SnArray * __sn__makeLabels(long long *);
SnArray * __sn__makeRows(long long *);
long long __sn__selectedValue(long long *, long long);
long long __sn__parameterValue(char *);
long long __sn__Matcher_instanceValue(__sn__Matcher *);
bool __sn__Matcher_staticValue(char *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char * __sn__makeSubject(long long *__sn__calls) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return sn_str_concat("to", "ken");}


SnArray * __sn__makeLabels(long long *__sn__calls) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return ({
             SnArray *__al__ = sn_array_new(sizeof(char *), 1);
             __al__->elem_tag = SN_TAG_STRING;
     
             __al__->elem_release = (void (*)(void *))sn_cleanup_str;
     
             __al__->elem_copy = sn_copy_str;
     
             sn_array_push(__al__, &(char *){ strdup("indexed") });
             __al__;
         });}


SnArray * __sn__makeRows(long long *__sn__calls) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return ({
             SnArray *__al__ = sn_array_new(sizeof(SnArray *), 1);
             __al__->elem_tag = SN_TAG_ARRAY;
     
             __al__->elem_release = (void (*)(void *))sn_cleanup_array;
     
             __al__->elem_copy = sn_copy_array;
     
             sn_array_push(__al__, &(SnArray *){ ({
             SnArray *__al__ = sn_array_new(sizeof(char *), 1);
             __al__->elem_tag = SN_TAG_STRING;
     
             __al__->elem_release = (void (*)(void *))sn_cleanup_str;
     
             __al__->elem_copy = sn_copy_str;
     
             sn_array_push(__al__, &(char *){ strdup("nested") });
             __al__;
         }) });
             __al__;
         });}


long long __sn__selectedValue(long long *__sn__calls, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__value;}


long long __sn__parameterValue(char * __sn__value) {

    long long __sn__result = ({
            long long __match_result__;
            char * __match_subject__ = __sn__value;
            if (strcmp(__match_subject__, "token") == 0) {
                __match_result__ = 7LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });

    return __sn__result;}


long long __sn__Matcher_instanceValue(__sn__Matcher *__sn__self) {

    long long __sn__result = ({
            long long __match_result__;
            char * __match_subject__ = __sn__self->__sn__text;
            if (strcmp(__match_subject__, "north") == 0) {
                __match_result__ = 10LL;
            } else if (strcmp(__match_subject__, "north") == 0) {
                __match_result__ = 20LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });

    return __sn__result;}

bool __sn__Matcher_staticValue(char * __sn__value) {

    return ({
             bool __match_result__;
             char * __match_subject__ = __sn__value;
             if (strcmp(__match_subject__, "") == 0) {
                 __match_result__ = false;
             } else if (strcmp(__match_subject__, "héllo") == 0 || strcmp(__match_subject__, "hello") == 0) {
                 __match_result__ = true;
             } else {
                 __match_result__ = false;
             }
             __match_result__;
         });}

int main() {
    sn_auto_str char * __sn__subject = strdup("token");
    long long __sn__statementCalls = 0LL;
    ({
            long long __match_result__;
            char * __match_subject__ = __sn__subject;
            if (strcmp(__match_subject__, "miss") == 0) {
                __match_result__ = (__sn__statementCalls = 100LL);
            } else if (strcmp(__match_subject__, "token") == 0 || strcmp(__match_subject__, "token") == 0) {
                __match_result__ = ({
        long long *__sn_place__ = &(__sn__statementCalls);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
            } else {
                __match_result__ = (__sn__statementCalls = 200LL);
            }
            __match_result__;
        });
    
    printf("%s\n", (((__sn__statementCalls == 1LL) && (strcmp(__sn__subject, "token") == 0))) ? "true" : "false");
    
    long long __sn__subjectCalls = 0LL;
    long long __sn__armCalls = 0LL;
    long long __sn__chosen = ({
            long long __match_result__;
            char * __match_subject__ = __sn__makeSubject(&__sn__subjectCalls);
            if (strcmp(__match_subject__, "miss") == 0) {
                __match_result__ = __sn__selectedValue(&__sn__armCalls, 1LL);
            } else if (strcmp(__match_subject__, "token") == 0) {
                __match_result__ = __sn__selectedValue(&__sn__armCalls, 7LL);
            } else if (strcmp(__match_subject__, "token") == 0) {
                __match_result__ = __sn__selectedValue(&__sn__armCalls, 9LL);
            } else {
                __match_result__ = __sn__selectedValue(&__sn__armCalls, 0LL);
            }
            free(__match_subject__);
            __match_result__;
        });
    printf("%s\n", ((((__sn__chosen == 7LL) && (__sn__subjectCalls == 1LL)) && (__sn__armCalls == 1LL))) ? "true" : "false");
    
    long long __sn__noElseCalls = 0LL;
    ({
            char * __match_subject__ = "absent";
            if (strcmp(__match_subject__, "present") == 0) {
                ({
        long long *__sn_place__ = &(__sn__noElseCalls);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    printf("%s\n", ((__sn__noElseCalls == 0LL)) ? "true" : "false");
    
    bool __sn__literalResult = ({
            bool __match_result__;
            char * __match_subject__ = "";
            if (strcmp(__match_subject__, "") == 0) {
                __match_result__ = true;
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    printf("%s\n", (__sn__literalResult) ? "true" : "false");
    
    bool __sn__nested = ({
            bool __match_result__;
            char * __match_subject__ = "outer";
            if (strcmp(__match_subject__, "outer") == 0) {
                __match_result__ = ({
            bool __match_result__;
            char * __match_subject__ = __sn__subject;
            if (strcmp(__match_subject__, "token") == 0) {
                __match_result__ = true;
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    printf("%s\n", (__sn__nested) ? "true" : "false");
    
    sn_auto_str char * __sn____sn_match_subject = strdup("source-subject");
    long long __sn____sn_match_array = 0LL;
    long long __sn____sn_match_index = 41LL;
    sn_auto_str char * __sn____sn_match_subject_0 = strdup("candidate-subject");
    long long __sn____sn_match_array_0 = 7LL;
    long long __sn____sn_match_index_0 = 8LL;
    long long __sn__hygieneCalls = 0LL;
    sn_auto_arr SnArray * __sn__hygieneRows = ({
            SnArray *__al__ = sn_array_new(sizeof(SnArray *), 1);
            __al__->elem_tag = SN_TAG_ARRAY;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_array;
    
            __al__->elem_copy = sn_copy_array;
    
            sn_array_push(__al__, &(SnArray *){ ({
            SnArray *__al__ = sn_array_new(sizeof(char *), 1);
            __al__->elem_tag = SN_TAG_STRING;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_str;
    
            __al__->elem_copy = sn_copy_str;
    
            sn_array_push(__al__, &(char *){ strdup("nested") });
            __al__;
        }) });
            __al__;
        });
    sn_auto_arr SnArray * __sn____chain_tmp_0 = __sn__makeRows(&__sn__hygieneCalls);
    ({
            char * __match_subject__ = (((char * *)(((SnArray * *)__sn____chain_tmp_0->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn____chain_tmp_0->len : __ai__; })])->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + (((SnArray * *)__sn____chain_tmp_0->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn____chain_tmp_0->len : __ai__; })])->len : __ai__; })]);
            if (strcmp(__match_subject__, "nested") == 0) {
                ({
        char *__sn_tmp__ = strdup(__sn____sn_match_subject);
        free(__sn____sn_match_subject);
        __sn____sn_match_subject = __sn_tmp__;
        __sn____sn_match_subject;
    });
    
    (__sn____sn_match_array = sn_add_long(__sn____sn_match_index, 1LL));
    
    ({
        char *__sn_tmp__ = strdup(__sn____sn_match_subject_0);
        free(__sn____sn_match_subject_0);
        __sn____sn_match_subject_0 = __sn_tmp__;
        __sn____sn_match_subject_0;
    });
    
    (__sn____sn_match_array_0 = sn_add_long(__sn____sn_match_index_0, 1LL));
    
    ({
            char * __match_subject__ = (((char * *)(((SnArray * *)__sn__hygieneRows->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__hygieneRows->len : __ai__; })])->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + (((SnArray * *)__sn__hygieneRows->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__hygieneRows->len : __ai__; })])->len : __ai__; })]);
            if (strcmp(__match_subject__, "nested") == 0) {
                (__sn____sn_match_index = sn_sub_long(__sn____sn_match_array, 1LL));
    
    (__sn____sn_match_index_0 = sn_sub_long(__sn____sn_match_array_0, 1LL));
    
    
            }
        });
    
    ({
            float __match_subject__ = 1.0f;
            if (__match_subject__ == 1.0f) {
                ({
        char *__sn_tmp__ = strdup(__sn____sn_match_subject);
        free(__sn____sn_match_subject);
        __sn____sn_match_subject = __sn_tmp__;
        __sn____sn_match_subject;
    });
    
    
            }
        });
    
    
            }
        });
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup(__sn____sn_match_subject);
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_array));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_index));
            sn_str_concat_multi(5, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__);
        }); sn_println(__ps__); };
    
    printf("%s\n", (((((__sn__hygieneCalls == 1LL) && (strcmp(__sn____sn_match_subject_0, "candidate-subject") == 0)) && (__sn____sn_match_array_0 == 9LL)) && (__sn____sn_match_index_0 == 8LL))) ? "true" : "false");
    
    sn_auto_arr SnArray * __sn__labels = ({
            SnArray *__al__ = sn_array_new(sizeof(char *), 1);
            __al__->elem_tag = SN_TAG_STRING;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_str;
    
            __al__->elem_copy = sn_copy_str;
    
            sn_array_push(__al__, &(char *){ strdup("indexed") });
            __al__;
        });
    bool __sn__indexed = ({
            bool __match_result__;
            char * __match_subject__ = (((char * *)__sn__labels->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__labels->len : __ai__; })]);
            if (strcmp(__match_subject__, "indexed") == 0) {
                __match_result__ = true;
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    printf("%s\n", ((((__sn__indexed && (strcmp((((char * *)__sn__labels->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__labels->len : __ai__; })]), "indexed") == 0)) && (__sn____sn_match_array == 42LL)) && (__sn____sn_match_index == 41LL))) ? "true" : "false");
    
    long long __sn__indexedReceiverCalls = 0LL;
    sn_auto_arr SnArray * __sn____chain_tmp_1 = __sn__makeLabels(&__sn__indexedReceiverCalls);
    long long __sn__indexedReceiverResult = ({
            long long __match_result__;
            char * __match_subject__ = (((char * *)__sn____chain_tmp_1->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn____chain_tmp_1->len : __ai__; })]);
            if (strcmp(__match_subject__, "indexed") == 0) {
                __match_result__ = 1LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    printf("%lld\n", (long long)(__sn__indexedReceiverResult));
    
    printf("%lld\n", (long long)(__sn__indexedReceiverCalls));
    
    long long __sn__nestedReceiverCalls = 0LL;
    sn_auto_arr SnArray * __sn____chain_tmp_2 = __sn__makeRows(&__sn__nestedReceiverCalls);
    long long __sn__nestedReceiverResult = ({
            long long __match_result__;
            char * __match_subject__ = (((char * *)(((SnArray * *)__sn____chain_tmp_2->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn____chain_tmp_2->len : __ai__; })])->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + (((SnArray * *)__sn____chain_tmp_2->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn____chain_tmp_2->len : __ai__; })])->len : __ai__; })]);
            if (strcmp(__match_subject__, "nested") == 0) {
                __match_result__ = 1LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    printf("%lld\n", (long long)(__sn__nestedReceiverResult));
    
    printf("%lld\n", (long long)(__sn__nestedReceiverCalls));
    
    long long __sn__concatenated = ({
            long long __match_result__;
            char * __match_subject__ = sn_str_concat("con", "tent");
            if (strcmp(__match_subject__, "content") == 0) {
                __match_result__ = 1LL;
            } else {
                __match_result__ = 0LL;
            }
            free(__match_subject__);
            __match_result__;
        });
    printf("%s\n", ((__sn__concatenated == 1LL)) ? "true" : "false");
    
    sn_auto_Matcher __sn__Matcher __sn__matcher = (__sn__Matcher){ .__sn__text = strdup("north") };
    printf("%s\n", (((__sn__Matcher_instanceValue(&__sn__matcher) == 10LL) && (strcmp(__sn__matcher.__sn__text, "north") == 0))) ? "true" : "false");
    
    sn_auto_str char * __sn__greeting = strdup("héllo");
    printf("%s\n", ((__sn__Matcher_staticValue(__sn__greeting) && (strcmp(__sn__greeting, "héllo") == 0))) ? "true" : "false");
    
    printf("%s\n", (((__sn__parameterValue(__sn__subject) == 7LL) && (strcmp(__sn__subject, "token") == 0))) ? "true" : "false");
    
    fflush(stdout);
    return 0;
}
