/* ==============================================================================
 * package_visit.c - Package Visit Tracking
 * ==============================================================================
 * Functions for tracking visited packages during recursive installation
 * to detect cycles and version conflicts.
 * ============================================================================== */

/* Check if package has been visited during recursive install */
static bool package_is_visited(const PackageVisited *visited, const char *name)
{
    for (int i = 0; i < visited->count; i++) {
        if (strcmp(visited->names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

/* Get the ref (version) for a visited package, returns NULL if not found */
static const char *package_get_visited_ref(const PackageVisited *visited, const char *name)
{
    for (int i = 0; i < visited->count; i++) {
        if (strcmp(visited->names[i], name) == 0) {
            return visited->refs[i][0] ? visited->refs[i] : NULL;
        }
    }
    return NULL;
}

/* Mark package as visited, returns false if at capacity */
static bool package_mark_visited(PackageVisited *visited, const char *name, const char *ref)
{
    if (visited->count >= PKG_MAX_VISITED) {
        pkg_warning("too many packages, some may not be tracked for cycles");
        return false;
    }
    strncpy(visited->names[visited->count], name, PKG_MAX_NAME_LEN - 1);
    visited->names[visited->count][PKG_MAX_NAME_LEN - 1] = '\0';
    if (ref && ref[0]) {
        strncpy(visited->refs[visited->count], ref, PKG_MAX_REF_LEN - 1);
        visited->refs[visited->count][PKG_MAX_REF_LEN - 1] = '\0';
    } else {
        visited->refs[visited->count][0] = '\0';
    }
    visited->count++;
    return true;
}
