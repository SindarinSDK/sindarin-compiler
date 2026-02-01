// src/runtime/runtime_thread_lock.c
// Sync Variable Lock Table

/* ============================================================================
 * Sync Variable Lock Table
 * ============================================================================
 * Hash table mapping variable addresses to mutexes for lock blocks.
 * Uses a simple open-addressed hash table with linear probing.
 * ============================================================================ */

#define RT_SYNC_LOCK_TABLE_SIZE 256  /* Must be power of 2 */

typedef struct RtSyncLockEntry {
    void *addr;                      /* Variable address (NULL = empty slot) */
    pthread_mutex_t mutex;           /* Associated mutex */
    bool initialized;                /* True if mutex is initialized */
} RtSyncLockEntry;

static RtSyncLockEntry g_sync_lock_table[RT_SYNC_LOCK_TABLE_SIZE];
static pthread_mutex_t g_sync_lock_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_sync_lock_table_initialized = false;

/* Initialize the sync lock table */
void rt_sync_lock_table_init(void)
{
    if (g_sync_lock_table_initialized) {
        return;
    }

    pthread_mutex_lock(&g_sync_lock_table_mutex);
    if (!g_sync_lock_table_initialized) {
        for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
            g_sync_lock_table[i].addr = NULL;
            g_sync_lock_table[i].initialized = false;
        }
        g_sync_lock_table_initialized = true;
    }
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
}

/* Clean up all sync locks */
void rt_sync_lock_table_cleanup(void)
{
    if (!g_sync_lock_table_initialized) {
        return;
    }

    pthread_mutex_lock(&g_sync_lock_table_mutex);
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        if (g_sync_lock_table[i].initialized) {
            pthread_mutex_destroy(&g_sync_lock_table[i].mutex);
            g_sync_lock_table[i].initialized = false;
            g_sync_lock_table[i].addr = NULL;
        }
    }
    g_sync_lock_table_initialized = false;
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
}

/* Hash function for pointer addresses */
static unsigned int rt_sync_lock_hash(void *addr)
{
    /* Simple hash: use bits from the pointer, shift to avoid alignment patterns */
    uintptr_t val = (uintptr_t)addr;
    val = (val >> 3) ^ (val >> 7) ^ (val >> 11);
    return (unsigned int)(val & (RT_SYNC_LOCK_TABLE_SIZE - 1));
}

/* Find or create a mutex for an address */
static pthread_mutex_t *rt_sync_lock_get_mutex(void *addr)
{
    if (!g_sync_lock_table_initialized) {
        rt_sync_lock_table_init();
    }

    unsigned int hash = rt_sync_lock_hash(addr);

    /* Lock table for safe access */
    pthread_mutex_lock(&g_sync_lock_table_mutex);

    /* Linear probe to find existing or empty slot */
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        int idx = (hash + i) & (RT_SYNC_LOCK_TABLE_SIZE - 1);

        if (g_sync_lock_table[idx].addr == addr && g_sync_lock_table[idx].initialized) {
            /* Found existing entry */
            pthread_mutex_unlock(&g_sync_lock_table_mutex);
            return &g_sync_lock_table[idx].mutex;
        }

        if (g_sync_lock_table[idx].addr == NULL) {
            /* Empty slot - create new entry */
            g_sync_lock_table[idx].addr = addr;
            pthread_mutex_init(&g_sync_lock_table[idx].mutex, NULL);
            g_sync_lock_table[idx].initialized = true;
            pthread_mutex_unlock(&g_sync_lock_table_mutex);
            return &g_sync_lock_table[idx].mutex;
        }
    }

    /* Table is full - this shouldn't happen in practice */
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
    fprintf(stderr, "rt_sync_lock_get_mutex: lock table full\n");
    return NULL;
}

/* Acquire a mutex lock for a sync variable */
void rt_sync_lock(void *addr)
{
    if (addr == NULL) {
        fprintf(stderr, "rt_sync_lock: NULL address\n");
        return;
    }

    pthread_mutex_t *mutex = rt_sync_lock_get_mutex(addr);
    if (mutex != NULL) {
        pthread_mutex_lock(mutex);
    }
}

/* Release a mutex lock for a sync variable */
void rt_sync_unlock(void *addr)
{
    if (addr == NULL) {
        fprintf(stderr, "rt_sync_unlock: NULL address\n");
        return;
    }

    /* Find the existing mutex */
    if (!g_sync_lock_table_initialized) {
        fprintf(stderr, "rt_sync_unlock: table not initialized\n");
        return;
    }

    unsigned int hash = rt_sync_lock_hash(addr);

    /* Linear probe to find existing entry */
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        int idx = (hash + i) & (RT_SYNC_LOCK_TABLE_SIZE - 1);

        if (g_sync_lock_table[idx].addr == addr && g_sync_lock_table[idx].initialized) {
            pthread_mutex_unlock(&g_sync_lock_table[idx].mutex);
            return;
        }

        if (g_sync_lock_table[idx].addr == NULL) {
            /* Empty slot - mutex was never created */
            fprintf(stderr, "rt_sync_unlock: no mutex found for address %p\n", addr);
            return;
        }
    }

    fprintf(stderr, "rt_sync_unlock: mutex not found for address %p\n", addr);
}
