#include "cgen/gen_model_split.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ---- Helpers ---- */

/* Extract basename without extension from a file path.
 * E.g., "/path/to/server.sn" -> "server" */
static const char *basename_no_ext(const char *path)
{
    const char *last_slash = strrchr(path, '/');
#ifdef _WIN32
    const char *last_bslash = strrchr(path, '\\');
    if (last_bslash && (!last_slash || last_bslash > last_slash)) last_slash = last_bslash;
#endif
    const char *base = last_slash ? last_slash + 1 : path;

    /* Strip .sn extension if present */
    static char buf[256];
    size_t len = strlen(base);
    if (len > 3 && strcmp(base + len - 3, ".sn") == 0)
        len -= 3;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, base, len);
    buf[len] = '\0';
    return buf;
}

/* Find or create a bucket index for a source file.
 * Returns the index, creating a new bucket if needed. */
static int find_or_create_bucket(const char **bucket_files, int *bucket_count,
                                  const char *source_file, int max_buckets)
{
    for (int i = 0; i < *bucket_count; i++)
    {
        if (strcmp(bucket_files[i], source_file) == 0)
            return i;
    }
    if (*bucket_count >= max_buckets)
        return 0; /* overflow: merge into first bucket */
    int idx = *bucket_count;
    bucket_files[idx] = source_file;
    (*bucket_count)++;
    return idx;
}

/* Deep-copy a json_object (returns a new reference) */
static json_object *json_deep_copy(json_object *src)
{
    if (!src) return NULL;
    json_object *dst = NULL;
    if (json_object_deep_copy(src, &dst, NULL) != 0)
        return NULL;
    return dst;
}

/* ---- Public API ---- */

ModularModel *gen_model_split(json_object *model, const char *entry_file)
{
    if (!model) return NULL;

    ModularModel *m = calloc(1, sizeof(ModularModel));
    if (!m) return NULL;

    /* Get arrays from model */
    json_object *functions = NULL, *globals = NULL, *structs = NULL;
    json_object *pragmas = NULL, *lambdas = NULL, *threads = NULL;
    json_object *fn_wrappers = NULL, *module_obj = NULL;

    json_object_object_get_ex(model, "functions", &functions);
    json_object_object_get_ex(model, "globals", &globals);
    json_object_object_get_ex(model, "structs", &structs);
    json_object_object_get_ex(model, "pragmas", &pragmas);
    json_object_object_get_ex(model, "lambdas", &lambdas);
    json_object_object_get_ex(model, "threads", &threads);
    json_object_object_get_ex(model, "fn_wrappers", &fn_wrappers);
    json_object_object_get_ex(model, "module", &module_obj);

    /* ---- Extract pragmas ---- */
    if (pragmas)
    {
        int pcount = (int)json_object_array_length(pragmas);
        for (int i = 0; i < pcount; i++)
        {
            json_object *p = json_object_array_get_idx(pragmas, i);
            json_object *pt = NULL, *pv = NULL;
            json_object_object_get_ex(p, "pragma_type", &pt);
            json_object_object_get_ex(p, "value", &pv);
            if (!pt || !pv) continue;
            const char *ptype = json_object_get_string(pt);
            const char *pval = json_object_get_string(pv);

            if (strcmp(ptype, "link") == 0)
            {
                m->link_libs = realloc(m->link_libs, (m->link_lib_count + 1) * sizeof(char *));
                m->link_libs[m->link_lib_count] = strdup(pval);
                m->link_lib_count++;
            }
            else if (strcmp(ptype, "source") == 0)
            {
                json_object *sd = NULL;
                json_object_object_get_ex(p, "source_dir", &sd);
                const char *sdir = sd ? json_object_get_string(sd) : ".";
                m->source_files = realloc(m->source_files, (m->source_file_count + 1) * sizeof(char *));
                m->source_dirs = realloc(m->source_dirs, (m->source_file_count + 1) * sizeof(char *));
                m->source_files[m->source_file_count] = strdup(pval);
                m->source_dirs[m->source_file_count] = strdup(sdir);
                m->source_file_count++;
            }
        }
    }

    /* ---- Bucket functions and globals by source_file ---- */
    #define MAX_BUCKETS 256
    const char *bucket_files[MAX_BUCKETS];
    int bucket_count = 0;

    /* Ensure entry file is bucket 0 (the main module) */
    bucket_files[0] = entry_file;
    bucket_count = 1;

    /* Per-bucket arrays of json_object indices */
    json_object *bucket_functions[MAX_BUCKETS];
    json_object *bucket_globals[MAX_BUCKETS];
    for (int i = 0; i < MAX_BUCKETS; i++)
    {
        bucket_functions[i] = json_object_new_array();
        bucket_globals[i] = json_object_new_array();
    }

    /* Distribute functions */
    if (functions)
    {
        int fcount = (int)json_object_array_length(functions);
        for (int i = 0; i < fcount; i++)
        {
            json_object *fn = json_object_array_get_idx(functions, i);
            json_object *sf = NULL;
            json_object_object_get_ex(fn, "source_file", &sf);

            const char *sfile = sf ? json_object_get_string(sf) : entry_file;
            int bucket = find_or_create_bucket(bucket_files, &bucket_count, sfile, MAX_BUCKETS);
            json_object_array_add(bucket_functions[bucket], json_deep_copy(fn));
        }
    }

    /* Distribute globals */
    if (globals)
    {
        int gcount = (int)json_object_array_length(globals);
        for (int i = 0; i < gcount; i++)
        {
            json_object *gv = json_object_array_get_idx(globals, i);
            json_object *sf = NULL;
            json_object_object_get_ex(gv, "source_file", &sf);

            const char *sfile = sf ? json_object_get_string(sf) : entry_file;
            int bucket = find_or_create_bucket(bucket_files, &bucket_count, sfile, MAX_BUCKETS);
            json_object_array_add(bucket_globals[bucket], json_deep_copy(gv));
        }
    }

    /* Ensure structs with methods create buckets (even if no standalone functions/globals) */
    if (structs)
    {
        int scount = (int)json_object_array_length(structs);
        for (int i = 0; i < scount; i++)
        {
            json_object *st = json_object_array_get_idx(structs, i);
            json_object *sf = NULL;
            json_object_object_get_ex(st, "source_file", &sf);
            if (sf)
            {
                const char *sfile = json_object_get_string(sf);
                find_or_create_bucket(bucket_files, &bucket_count, sfile, MAX_BUCKETS);
            }
        }
    }

    /* ---- Build common header model ---- */
    json_object *header = json_object_new_object();

    /* All structs go in the header */
    json_object_object_add(header, "structs", structs ? json_deep_copy(structs) : json_object_new_array());

    /* All pragmas go in the header (for #pragma include directives) */
    json_object_object_add(header, "pragmas", pragmas ? json_deep_copy(pragmas) : json_object_new_array());

    /* Forward declarations for functions (split into two lists):
     * - all_fwd: functions that go in sn_types.h (shared header)
     * - native_externs: native functions without bodies, declared per-module
     *   so wrapper methods can call them without wrong-type forward decls
     *   in the shared header conflicting with system headers */
    json_object *all_fwd = json_object_new_array();
    json_object *native_externs = json_object_new_array();
    if (functions)
    {
        int fcount = (int)json_object_array_length(functions);
        for (int i = 0; i < fcount; i++)
        {
            json_object *fn = json_object_array_get_idx(functions, i);
            json_object *name_obj = NULL;
            json_object_object_get_ex(fn, "name", &name_obj);
            const char *name = name_obj ? json_object_get_string(name_obj) : "";

            /* Skip main — it's defined in the main TU */
            if (strcmp(name, "main") == 0) continue;

            /* Native functions without bodies go to per-module externs only */
            json_object *is_native_obj = NULL, *has_body_obj = NULL;
            json_object_object_get_ex(fn, "is_native", &is_native_obj);
            json_object_object_get_ex(fn, "has_body", &has_body_obj);
            if (is_native_obj && json_object_get_boolean(is_native_obj) &&
                has_body_obj && !json_object_get_boolean(has_body_obj))
            {
                json_object *fn_copy = json_deep_copy(fn);
                /* Native functions with @alias + @source need forward declarations —
                 * their definitions are compiled separately and linked. */
                json_object *ps_obj = NULL;
                if (json_object_object_get_ex(fn_copy, "has_pragma_source", &ps_obj) &&
                    json_object_get_boolean(ps_obj))
                {
                    json_object_object_add(fn_copy, "needs_forward_decl",
                        json_object_new_boolean(true));
                }
                json_object_array_add(native_externs, fn_copy);
                continue;
            }

            json_object_array_add(all_fwd, json_deep_copy(fn));
        }
    }
    json_object_object_add(header, "functions", all_fwd);

    /* Extern declarations for ALL globals */
    json_object *all_extern = json_object_new_array();
    if (globals)
    {
        int gcount = (int)json_object_array_length(globals);
        for (int i = 0; i < gcount; i++)
        {
            json_object_array_add(all_extern, json_deep_copy(json_object_array_get_idx(globals, i)));
        }
    }
    json_object_object_add(header, "globals", all_extern);

    /* Lambda forward decls + closure structs */
    json_object_object_add(header, "lambdas", lambdas ? json_deep_copy(lambdas) : json_object_new_array());

    /* Thread arg structs */
    json_object_object_add(header, "threads", threads ? json_deep_copy(threads) : json_object_new_array());

    /* Fn wrapper forward decls */
    json_object_object_add(header, "fn_wrappers", fn_wrappers ? json_deep_copy(fn_wrappers) : json_object_new_array());

    m->common_header = header;

    /* ---- Build per-module implementation models ---- */
    m->impl_count = bucket_count;
    m->impl_models = calloc(bucket_count, sizeof(json_object *));
    m->impl_names = calloc(bucket_count, sizeof(const char *));

    for (int b = 0; b < bucket_count; b++)
    {
        json_object *impl = json_object_new_object();
        bool is_main = (b == 0);

        json_object_object_add(impl, "is_main_module", json_object_new_boolean(is_main));
        json_object_object_add(impl, "functions", bucket_functions[b]);
        json_object_object_add(impl, "globals", bucket_globals[b]);

        /* Structs for method definitions: methods of structs whose source_file matches this bucket */
        json_object *impl_structs = json_object_new_array();
        if (structs)
        {
            int scount = (int)json_object_array_length(structs);
            for (int s = 0; s < scount; s++)
            {
                json_object *st = json_object_array_get_idx(structs, s);
                json_object *sf = NULL;
                json_object_object_get_ex(st, "source_file", &sf);
                const char *sfile = sf ? json_object_get_string(sf) : entry_file;
                if (strcmp(sfile, bucket_files[b]) == 0)
                    json_object_array_add(impl_structs, json_deep_copy(st));
            }
        }
        json_object_object_add(impl, "structs", impl_structs);

        /* Native functions without bodies — each module gets these so wrapper
         * methods can call them (the actual definitions are in pragma .sn.c files) */
        json_object_object_add(impl, "native_externs",
            native_externs ? json_deep_copy(native_externs) : json_object_new_array());

        /* Distribute lambdas: each lambda ends up in the TU matching its
         * source_file so the body and its references live in the same
         * translation unit. Lambdas without a source_file (shouldn't happen
         * in practice) fall through to the main module. */
        json_object *bucket_lambdas = json_object_new_array();
        if (lambdas)
        {
            int lcount = (int)json_object_array_length(lambdas);
            for (int li = 0; li < lcount; li++)
            {
                json_object *ld = json_object_array_get_idx(lambdas, li);
                json_object *sf = NULL;
                json_object_object_get_ex(ld, "source_file", &sf);
                const char *sfile = sf ? json_object_get_string(sf) : entry_file;
                bool match = (strcmp(sfile, bucket_files[b]) == 0);
                if (!match && is_main && !sf)
                    match = true; /* fallback for lambdas with no source_file */
                if (match)
                    json_object_array_add(bucket_lambdas, json_deep_copy(ld));
            }
        }
        json_object_object_add(impl, "lambdas", bucket_lambdas);

        if (is_main)
        {
            /* Main module gets threads, fn_wrappers, module metadata, and ALL globals for deferred init */
            json_object_object_add(impl, "threads", threads ? json_deep_copy(threads) : json_object_new_array());
            json_object_object_add(impl, "fn_wrappers", fn_wrappers ? json_deep_copy(fn_wrappers) : json_object_new_array());
            json_object_object_add(impl, "module", module_obj ? json_deep_copy(module_obj) : json_object_new_object());

            /* ALL globals for deferred initialization in main() */
            json_object_object_add(impl, "all_globals", globals ? json_deep_copy(globals) : json_object_new_array());

            /* Pragma sources for this module */
            json_object_object_add(impl, "pragmas", pragmas ? json_deep_copy(pragmas) : json_object_new_array());
        }

        /* Derive impl name from source file path */
        const char *bname = basename_no_ext(bucket_files[b]);
        if (is_main)
        {
            m->impl_names[b] = strdup("main");
        }
        else
        {
            /* Prefix with "module_" to avoid name collisions */
            char namebuf[280];
            snprintf(namebuf, sizeof(namebuf), "module_%s", bname);
            m->impl_names[b] = strdup(namebuf);
        }

        m->impl_models[b] = impl;
    }

    /* Free unused bucket arrays for buckets that weren't used
     * (only happens if bucket_count < MAX_BUCKETS) */
    for (int i = bucket_count; i < MAX_BUCKETS; i++)
    {
        json_object_put(bucket_functions[i]);
        json_object_put(bucket_globals[i]);
    }

    return m;
}

void modular_model_free(ModularModel *m)
{
    if (!m) return;

    if (m->common_header)
        json_object_put(m->common_header);

    for (int i = 0; i < m->impl_count; i++)
    {
        if (m->impl_models[i])
            json_object_put(m->impl_models[i]);
        free((void *)m->impl_names[i]);
    }
    free(m->impl_models);
    free((void *)m->impl_names);

    for (int i = 0; i < m->link_lib_count; i++)
        free(m->link_libs[i]);
    free(m->link_libs);

    for (int i = 0; i < m->source_file_count; i++)
    {
        free(m->source_files[i]);
        free(m->source_dirs[i]);
    }
    free(m->source_files);
    free(m->source_dirs);

    free(m);
}
