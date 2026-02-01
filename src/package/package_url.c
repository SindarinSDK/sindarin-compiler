/* ==============================================================================
 * package_url.c - URL Parsing Utilities
 * ==============================================================================
 * Functions for parsing package URLs and extracting package names.
 * ============================================================================== */

bool package_parse_url_ref(const char *url_ref, char *out_url, char *out_ref)
{
    if (url_ref == NULL || out_url == NULL || out_ref == NULL) {
        return false;
    }

    /* Look for @ that separates URL from ref */
    const char *at_pos = strrchr(url_ref, '@');

    /* Check if @ is part of git@ URL scheme */
    if (at_pos != NULL) {
        /* git@github.com:user/repo.git has @ at position 3 */
        /* We want the @ after .git or at the end for ref */
        const char *git_ext = strstr(url_ref, ".git");
        if (git_ext != NULL && at_pos > git_ext) {
            /* @ is after .git, so it's a ref separator */
            size_t url_len = (size_t)(at_pos - url_ref);
            strncpy(out_url, url_ref, url_len);
            out_url[url_len] = '\0';
            strncpy(out_ref, at_pos + 1, PKG_MAX_REF_LEN - 1);
            out_ref[PKG_MAX_REF_LEN - 1] = '\0';
            return true;
        } else if (git_ext == NULL) {
            /* No .git extension, check if @ is after the last / */
            const char *last_slash = strrchr(url_ref, '/');
            if (last_slash != NULL && at_pos > last_slash) {
                size_t url_len = (size_t)(at_pos - url_ref);
                strncpy(out_url, url_ref, url_len);
                out_url[url_len] = '\0';
                strncpy(out_ref, at_pos + 1, PKG_MAX_REF_LEN - 1);
                out_ref[PKG_MAX_REF_LEN - 1] = '\0';
                return true;
            }
        }
    }

    /* No ref specified, copy URL as-is */
    strncpy(out_url, url_ref, PKG_MAX_URL_LEN - 1);
    out_url[PKG_MAX_URL_LEN - 1] = '\0';
    out_ref[0] = '\0';
    return false;
}

bool package_extract_name(const char *url, char *out_name)
{
    if (url == NULL || out_name == NULL) {
        return false;
    }

    /* Find last / or : */
    const char *start = strrchr(url, '/');
    if (start == NULL) {
        start = strrchr(url, ':');
    }

    if (start == NULL) {
        return false;
    }

    start++; /* Skip the separator */

    /* Copy name, stripping .git extension */
    const char *dot_git = strstr(start, ".git");
    size_t len;

    if (dot_git != NULL) {
        len = (size_t)(dot_git - start);
    } else {
        len = strlen(start);
    }

    if (len >= PKG_MAX_NAME_LEN) {
        len = PKG_MAX_NAME_LEN - 1;
    }

    strncpy(out_name, start, len);
    out_name[len] = '\0';

    return len > 0;
}
