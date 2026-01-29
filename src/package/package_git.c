/* ==============================================================================
 * package_git.c - Git Operations for Package Manager
 * ==============================================================================
 * Handles git clone, checkout, and fetch using libgit2.
 *
 * Authentication:
 *   SSH:   SN_GIT_SSH_KEY, SN_GIT_SSH_PASSPHRASE env vars (explicit key)
 *          Falls back to ~/.ssh/id_ed25519, id_rsa, id_ecdsa, id_dsa
 *          Then falls back to ssh-agent
 *   HTTPS: SN_GIT_USERNAME, SN_GIT_PASSWORD / SN_GIT_TOKEN
 * ============================================================================== */

#include "../package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <git2.h>

/* ANSI color codes for terminal output (matching diagnostic.c) */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"

/* ============================================================================
 * libgit2 Initialization
 * ============================================================================ */

static int libgit2_initialized = 0;

void package_git_init(void)
{
    if (!libgit2_initialized) {
        git_libgit2_init();
        libgit2_initialized = 1;
    }
}

void package_git_cleanup(void)
{
    if (libgit2_initialized) {
        git_libgit2_shutdown();
        libgit2_initialized = 0;
    }
}

/* ============================================================================
 * Credential Callback
 * ============================================================================ */

static int cred_attempt_count = 0;

/* Try to find SSH key in default locations (~/.ssh/) */
static const char *find_default_ssh_key(void)
{
    static char key_path[512];
    const char *home = NULL;

#ifdef _WIN32
    home = getenv("USERPROFILE");
#else
    home = getenv("HOME");
#endif

    if (!home) return NULL;

    /* Try common key names in order of preference */
    const char *key_names[] = {
        "id_ed25519",
        "id_rsa",
        "id_ecdsa",
        "id_dsa",
        NULL
    };

    for (int i = 0; key_names[i] != NULL; i++) {
#ifdef _WIN32
        snprintf(key_path, sizeof(key_path), "%s\\.ssh\\%s", home, key_names[i]);
#else
        snprintf(key_path, sizeof(key_path), "%s/.ssh/%s", home, key_names[i]);
#endif
        FILE *f = fopen(key_path, "r");
        if (f) {
            fclose(f);
            return key_path;
        }
    }

    return NULL;
}

static int credential_callback(git_credential **out, const char *url,
                               const char *username_from_url,
                               unsigned int allowed_types, void *payload)
{
    (void)payload;
    (void)url;

    /* Prevent infinite retry loops */
    cred_attempt_count++;
    if (cred_attempt_count > 3) {
        return GIT_EAUTH;
    }

    const char *user = username_from_url;
    if (!user) user = getenv("SN_GIT_USERNAME");
    if (!user) user = "git"; /* Default for SSH */

    /* Try SSH key authentication */
    if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
        const char *key_path = getenv("SN_GIT_SSH_KEY");
        const char *passphrase = getenv("SN_GIT_SSH_PASSPHRASE");

        /* If no explicit key path, try to find key in ~/.ssh/ */
        if (!key_path) {
            key_path = find_default_ssh_key();
        }

        if (key_path) {
            int rc = git_credential_ssh_key_new(out, user, NULL, key_path,
                                              passphrase ? passphrase : "");
            if (rc == 0) return 0;
        }

        /* Fall back to SSH agent */
        return git_credential_ssh_key_from_agent(out, user);
    }

    /* Userpass for HTTPS */
    if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        const char *username = getenv("SN_GIT_USERNAME");
        const char *password = getenv("SN_GIT_PASSWORD");
        if (!password) password = getenv("SN_GIT_TOKEN");

        if (username && password) {
            return git_credential_userpass_plaintext_new(out, username, password);
        }
    }

    return GIT_EAUTH;
}

/* ============================================================================
 * Error Handling
 * ============================================================================ */

static void print_git_error(const char *context)
{
    const git_error *err = git_error_last();
    if (err) {
        fprintf(stderr, "%serror%s: git %s: %s\n",
                COLOR_RED, COLOR_RESET, context, err->message);
    } else {
        fprintf(stderr, "%serror%s: git %s: unknown error\n",
                COLOR_RED, COLOR_RESET, context);
    }
}

/* ============================================================================
 * Git Operations
 * ============================================================================ */

bool package_git_is_repo(const char *path)
{
    if (path == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, path);

    if (rc == 0) {
        git_repository_free(repo);
        return true;
    }

    return false;
}

bool package_git_clone(const char *url, const char *dest_path)
{
    if (url == NULL || dest_path == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    git_clone_options opts;
    git_clone_options_init(&opts, GIT_CLONE_OPTIONS_VERSION);
    opts.fetch_opts.callbacks.credentials = credential_callback;
    cred_attempt_count = 0;

    int rc = git_clone(&repo, url, dest_path, &opts);

    if (rc != 0) {
        print_git_error("clone");
        return false;
    }

    git_repository_free(repo);
    return true;
}

bool package_git_fetch(const char *repo_path)
{
    if (repo_path == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, repo_path);
    if (rc != 0) {
        print_git_error("open repository");
        return false;
    }

    git_remote *remote = NULL;
    rc = git_remote_lookup(&remote, repo, "origin");
    if (rc != 0) {
        print_git_error("lookup remote");
        git_repository_free(repo);
        return false;
    }

    git_fetch_options opts;
    git_fetch_options_init(&opts, GIT_FETCH_OPTIONS_VERSION);
    opts.callbacks.credentials = credential_callback;
    cred_attempt_count = 0;

    rc = git_remote_fetch(remote, NULL, &opts, NULL);

    git_remote_free(remote);
    git_repository_free(repo);

    if (rc != 0) {
        print_git_error("fetch");
        return false;
    }

    return true;
}

bool package_git_checkout(const char *repo_path, const char *ref_name)
{
    if (repo_path == NULL || ref_name == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, repo_path);
    if (rc != 0) {
        print_git_error("open repository");
        return false;
    }

    git_object *target = NULL;
    git_reference *branch_ref = NULL;

    /* First, try to look up as a remote tracking branch (origin/ref_name) */
    char remote_ref[256];
    snprintf(remote_ref, sizeof(remote_ref), "origin/%s", ref_name);

    rc = git_revparse_single(&target, repo, remote_ref);
    if (rc != 0) {
        /* Try as a tag */
        char tag_ref[256];
        snprintf(tag_ref, sizeof(tag_ref), "refs/tags/%s", ref_name);
        rc = git_revparse_single(&target, repo, tag_ref);
    }
    if (rc != 0) {
        /* Try as a direct ref */
        rc = git_revparse_single(&target, repo, ref_name);
    }

    if (rc != 0) {
        print_git_error("resolve ref");
        git_repository_free(repo);
        return false;
    }

    /* Checkout the tree */
    git_checkout_options checkout_opts;
    git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
    checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;

    rc = git_checkout_tree(repo, target, &checkout_opts);
    if (rc != 0) {
        print_git_error("checkout tree");
        git_object_free(target);
        git_repository_free(repo);
        return false;
    }

    /* Try to create/update local branch from remote tracking branch */
    git_reference *new_branch_ref = NULL;
    git_commit *commit = NULL;

    rc = git_commit_lookup(&commit, repo, git_object_id(target));
    if (rc == 0) {
        /* Try to find existing local branch */
        rc = git_branch_lookup(&branch_ref, repo, ref_name, GIT_BRANCH_LOCAL);
        if (rc == 0) {
            /* Branch exists, update HEAD to point to it */
            char full_ref_name[256];
            snprintf(full_ref_name, sizeof(full_ref_name), "refs/heads/%s", ref_name);
            rc = git_repository_set_head(repo, full_ref_name);
            git_reference_free(branch_ref);
        } else {
            /* Create local branch */
            rc = git_branch_create(&new_branch_ref, repo, ref_name, commit, 0);
            if (rc == 0) {
                char full_ref_name[256];
                snprintf(full_ref_name, sizeof(full_ref_name), "refs/heads/%s", ref_name);
                git_repository_set_head(repo, full_ref_name);
                git_reference_free(new_branch_ref);
            } else {
                /* If branch creation fails (e.g., for tags), just detach HEAD */
                git_repository_set_head_detached(repo, git_object_id(target));
            }
        }
        git_commit_free(commit);
    } else {
        /* Detach HEAD at the target */
        git_repository_set_head_detached(repo, git_object_id(target));
    }

    git_object_free(target);
    git_repository_free(repo);

    return true;
}

bool package_git_get_head_sha(const char *repo_path, char *out_sha)
{
    if (repo_path == NULL || out_sha == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, repo_path);
    if (rc != 0) {
        return false;
    }

    git_reference *head_ref = NULL;
    rc = git_repository_head(&head_ref, repo);
    if (rc != 0) {
        git_repository_free(repo);
        return false;
    }

    const git_oid *oid = git_reference_target(head_ref);
    if (oid == NULL) {
        /* If head is symbolic, resolve it */
        git_reference *resolved = NULL;
        rc = git_reference_resolve(&resolved, head_ref);
        if (rc == 0) {
            oid = git_reference_target(resolved);
            git_reference_free(resolved);
        }
    }

    if (oid == NULL) {
        git_reference_free(head_ref);
        git_repository_free(repo);
        return false;
    }

    git_oid_tostr(out_sha, 41, oid);

    git_reference_free(head_ref);
    git_repository_free(repo);
    return true;
}

bool package_git_get_ref_sha(const char *repo_path, const char *ref_name, char *out_sha)
{
    if (repo_path == NULL || ref_name == NULL || out_sha == NULL) {
        return false;
    }

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, repo_path);
    if (rc != 0) {
        return false;
    }

    git_object *target = NULL;

    /* First, try as a remote tracking branch (origin/ref_name) */
    char remote_ref[256];
    snprintf(remote_ref, sizeof(remote_ref), "origin/%s", ref_name);
    rc = git_revparse_single(&target, repo, remote_ref);

    if (rc != 0) {
        /* Try as a tag */
        char tag_ref[256];
        snprintf(tag_ref, sizeof(tag_ref), "refs/tags/%s", ref_name);
        rc = git_revparse_single(&target, repo, tag_ref);
    }

    if (rc != 0) {
        /* Try as a direct ref */
        rc = git_revparse_single(&target, repo, ref_name);
    }

    if (rc != 0) {
        git_repository_free(repo);
        return false;
    }

    /* Get the commit SHA (peel if it's a tag pointing to a commit) */
    git_object *peeled = NULL;
    rc = git_object_peel(&peeled, target, GIT_OBJECT_COMMIT);
    if (rc == 0) {
        git_oid_tostr(out_sha, 41, git_object_id(peeled));
        git_object_free(peeled);
    } else {
        git_oid_tostr(out_sha, 41, git_object_id(target));
    }

    git_object_free(target);
    git_repository_free(repo);
    return true;
}

bool package_git_get_current_branch(const char *repo_path, char *out_branch, size_t out_len)
{
    if (repo_path == NULL || out_branch == NULL || out_len == 0) {
        return false;
    }

    out_branch[0] = '\0';

    package_git_init();

    git_repository *repo = NULL;
    int rc = git_repository_open(&repo, repo_path);
    if (rc != 0) {
        return false;
    }

    /* Check if HEAD is detached */
    if (git_repository_head_detached(repo)) {
        git_repository_free(repo);
        return false; /* Detached HEAD, not on a branch */
    }

    git_reference *head_ref = NULL;
    rc = git_repository_head(&head_ref, repo);
    if (rc != 0) {
        git_repository_free(repo);
        return false;
    }

    /* Get the branch name from the reference */
    const char *branch_name = NULL;
    rc = git_branch_name(&branch_name, head_ref);
    if (rc == 0 && branch_name != NULL) {
        strncpy(out_branch, branch_name, out_len - 1);
        out_branch[out_len - 1] = '\0';
    }

    git_reference_free(head_ref);
    git_repository_free(repo);

    return out_branch[0] != '\0';
}
