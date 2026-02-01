// src/package/package_lfs_pointer.c
// LFS Pointer Parsing

/* ============================================================================
 * CPU Count and Threading Helpers
 * ============================================================================ */

/* Get the number of CPU cores available */
static int get_cpu_count(void)
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (int)sysinfo.dwNumberOfProcessors;
#else
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return nprocs > 0 ? (int)nprocs : 1;
#endif
}

/* ============================================================================
 * LFS Pointer Parsing
 * ============================================================================ */

typedef struct {
    char oid[MAX_OID_LEN];      /* SHA256 hash */
    long long size;              /* File size in bytes */
    bool valid;
} LfsPointer;

/* Parse an LFS pointer file content */
static LfsPointer parse_lfs_pointer(const char *content, size_t len)
{
    LfsPointer ptr = {0};
    ptr.valid = false;

    if (len > LFS_POINTER_MAX_SIZE) {
        return ptr;  /* Too large to be a pointer file */
    }

    /* Check version line */
    if (strncmp(content, LFS_POINTER_VERSION, strlen(LFS_POINTER_VERSION)) != 0) {
        return ptr;
    }

    /* Find oid line */
    const char *oid_start = strstr(content, LFS_OID_PREFIX);
    if (!oid_start) {
        return ptr;
    }
    oid_start += strlen(LFS_OID_PREFIX);

    /* Extract OID (64 hex chars) */
    size_t i;
    for (i = 0; i < 64 && oid_start[i] && oid_start[i] != '\n' && oid_start[i] != '\r'; i++) {
        ptr.oid[i] = oid_start[i];
    }
    ptr.oid[i] = '\0';

    if (i != 64) {
        return ptr;  /* Invalid OID length */
    }

    /* Find size line */
    const char *size_start = strstr(content, LFS_SIZE_PREFIX);
    if (!size_start) {
        return ptr;
    }
    size_start += strlen(LFS_SIZE_PREFIX);

    ptr.size = atoll(size_start);
    if (ptr.size <= 0) {
        return ptr;
    }

    ptr.valid = true;
    return ptr;
}

/* Check if a file is an LFS pointer */
static bool is_lfs_pointer_file(const char *path, LfsPointer *out_ptr)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* LFS pointers are small */
    if (fsize <= 0 || fsize > LFS_POINTER_MAX_SIZE) {
        fclose(f);
        return false;
    }

    char content[LFS_POINTER_MAX_SIZE + 1];
    size_t read_size = fread(content, 1, (size_t)fsize, f);
    fclose(f);

    if (read_size != (size_t)fsize) {
        return false;
    }
    content[read_size] = '\0';

    LfsPointer ptr = parse_lfs_pointer(content, read_size);
    if (ptr.valid && out_ptr) {
        *out_ptr = ptr;
    }
    return ptr.valid;
}
