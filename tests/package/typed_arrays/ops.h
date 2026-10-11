#include <stdint.h>
void *sn_typed_new(void);
void *sn_typed_copy(void *, void *, int64_t);
void *sn_typed_ints(void *, void *);
void sn_typed_free(void *);
void sn_typed_observe(void *);
void sn_typed_set_callback(void (*)(void *));
uint64_t sn_typed_dead_rows(void);
uint64_t sn_typed_dead_strings(void);
uint64_t sn_typed_calls(void);
