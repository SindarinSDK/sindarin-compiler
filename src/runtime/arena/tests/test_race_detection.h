#ifndef TEST_RACE_DETECTION_H
#define TEST_RACE_DETECTION_H

#include "test_framework.h"

/* ============================================================================
 * Race Detection Stress Tests - Shared Header
 * ============================================================================
 * These tests are designed to expose race conditions by:
 * - Using higher thread counts than normal tests
 * - Running operations for longer durations
 * - Mixing operations that stress synchronization boundaries
 * - Using barriers to maximize timing sensitivity
 * ============================================================================ */

/* Simple barrier implementation for coordinating thread start */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int threshold;
    int generation;
} Barrier;

static inline void barrier_init(Barrier *b, int count)
{
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->threshold = count;
    b->generation = 0;
}

static inline void barrier_wait(Barrier *b)
{
    pthread_mutex_lock(&b->mutex);
    int gen = b->generation;
    b->count++;
    if (b->count >= b->threshold) {
        b->generation++;
        b->count = 0;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (gen == b->generation) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }
    pthread_mutex_unlock(&b->mutex);
}

static inline void barrier_destroy(Barrier *b)
{
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

/* Test function declarations */
void test_race_scaling_run(void);
void test_race_mixed_lifecycle_run(void);
void test_race_compaction_run(void);
void test_race_table_stability_run(void);
void test_race_pins_run(void);
void test_race_hierarchy_run(void);
void test_race_destroy_reset_run(void);
void test_race_recycling_clone_run(void);

#endif /* TEST_RACE_DETECTION_H */
