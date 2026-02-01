// runtime_malloc_redirect_hashset.c
// Hash set implementation for allocation tracking

/* ============================================================================
 * Hash Set Implementation
 * ============================================================================ */

/* Hash function for pointers */
static size_t hash_ptr(void *ptr, size_t bucket_count)
{
    uintptr_t addr = (uintptr_t)ptr;
    /* Mix bits for better distribution */
    addr ^= addr >> 17;
    addr *= 0xed5ad4bbU;
    addr ^= addr >> 11;
    addr *= 0xac4c1b51U;
    addr ^= addr >> 15;
    return addr % bucket_count;
}

RtAllocHashSet *rt_alloc_hash_set_create(size_t initial_buckets)
{
    if (initial_buckets == 0) {
        initial_buckets = 256;
    }

    /* Use system malloc for hash set internals (not redirected) */
    RtAllocHashSet *set;
    if (orig_malloc) {
        set = orig_malloc(sizeof(RtAllocHashSet));
    } else {
        set = malloc(sizeof(RtAllocHashSet));
    }
    if (!set) return NULL;

    if (orig_calloc) {
        set->buckets = orig_calloc(initial_buckets, sizeof(RtAllocHashEntry *));
    } else {
        set->buckets = calloc(initial_buckets, sizeof(RtAllocHashEntry *));
    }
    if (!set->buckets) {
        if (orig_free) orig_free(set);
        else free(set);
        return NULL;
    }

    set->bucket_count = initial_buckets;
    set->entry_count = 0;
    set->grow_threshold = initial_buckets * 3 / 4;  /* 75% load factor */

    return set;
}

void rt_alloc_hash_set_destroy(RtAllocHashSet *set)
{
    if (!set) return;

    /* Free all entries */
    for (size_t i = 0; i < set->bucket_count; i++) {
        RtAllocHashEntry *entry = set->buckets[i];
        while (entry) {
            RtAllocHashEntry *next = entry->next;
            if (orig_free) orig_free(entry);
            else free(entry);
            entry = next;
        }
    }

    if (orig_free) {
        orig_free(set->buckets);
        orig_free(set);
    } else {
        free(set->buckets);
        free(set);
    }
}

/* Rehash to larger size */
static bool hash_set_grow(RtAllocHashSet *set)
{
    size_t new_bucket_count = set->bucket_count * 2;
    RtAllocHashEntry **new_buckets;

    if (orig_calloc) {
        new_buckets = orig_calloc(new_bucket_count, sizeof(RtAllocHashEntry *));
    } else {
        new_buckets = calloc(new_bucket_count, sizeof(RtAllocHashEntry *));
    }
    if (!new_buckets) return false;

    /* Rehash all entries */
    for (size_t i = 0; i < set->bucket_count; i++) {
        RtAllocHashEntry *entry = set->buckets[i];
        while (entry) {
            RtAllocHashEntry *next = entry->next;
            size_t new_idx = hash_ptr(entry->ptr, new_bucket_count);
            entry->next = new_buckets[new_idx];
            new_buckets[new_idx] = entry;
            entry = next;
        }
    }

    if (orig_free) orig_free(set->buckets);
    else free(set->buckets);

    set->buckets = new_buckets;
    set->bucket_count = new_bucket_count;
    set->grow_threshold = new_bucket_count * 3 / 4;

    return true;
}

bool rt_alloc_hash_set_insert(RtAllocHashSet *set, void *ptr, size_t size)
{
    if (!set || !ptr) return false;

    /* Grow if needed */
    if (set->entry_count >= set->grow_threshold) {
        hash_set_grow(set);  /* Best effort - continue even if grow fails */
    }

    size_t idx = hash_ptr(ptr, set->bucket_count);

    /* Check if already exists */
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) {
            e->size = size;  /* Update size */
            return true;
        }
    }

    /* Create new entry */
    RtAllocHashEntry *entry;
    if (orig_malloc) {
        entry = orig_malloc(sizeof(RtAllocHashEntry));
    } else {
        entry = malloc(sizeof(RtAllocHashEntry));
    }
    if (!entry) return false;

    entry->ptr = ptr;
    entry->size = size;
    entry->next = set->buckets[idx];
    set->buckets[idx] = entry;
    set->entry_count++;

    return true;
}

bool rt_alloc_hash_set_remove(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return false;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    RtAllocHashEntry **prev = &set->buckets[idx];

    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) {
            *prev = e->next;
            if (orig_free) orig_free(e);
            else free(e);
            set->entry_count--;
            return true;
        }
        prev = &e->next;
    }

    return false;
}

bool rt_alloc_hash_set_contains(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return false;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) return true;
    }

    return false;
}

size_t rt_alloc_hash_set_get_size(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return 0;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) return e->size;
    }

    return 0;
}
