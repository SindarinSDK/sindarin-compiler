# GC Tasks V3: Arena Cleanup, Dispose Pattern & Private Arenas for Native Resources

## Context

**Problem 1 — Misleading shim:** The `rt_arena_destroy()` inline shim in `arena_v2.h` wraps
`rt_arena_v2_condemn()` but is misleadingly named — it doesn't destroy, it condemns. Meanwhile,
`rt_arena_v2_destroy()` (the real synchronous destructor) is `static` in `arena_gc.c` and
unavailable to SDK code.

**Problem 2 — Missing dispose:** YAML and XML lack the explicit `.dispose()` method that JSON
already has.

**Problem 3 — GC-unsafe native resource structs:** Long-lived native opaque structs (TcpStream,
TlsStream, UdpSocket, SshConnection, GitRepo, TextFile, BinaryFile, etc.) are allocated in the
function arena. Their factory methods extract `handle->ptr` into a raw `T*` pointer which is
returned to Sindarin and stored as the variable value. All subsequent method calls pass this raw
pointer. If GC compacts the block, `handle->ptr` is updated but the cached raw pointer becomes
dangling — every subsequent method call is a use-after-move.

Additionally, these structs cache raw pointers to internal sub-allocations (read buffers, address
strings) which are also invalidated by compaction.

**Solution:** Each resource struct creates a **private, detached arena** (`parent=NULL`) that is
invisible to GC. Since GC never discovers it, it never compacts it, and raw pointers remain valid
for the struct's entire lifetime. The `close()`/`dispose()` method calls `rt_arena_v2_destroy()`
to synchronously free everything.

### Phase overview
- **Tasks 7.1-7.4:** Prerequisites — remove shim, expose destroy, migrate callers
- **Tasks 7.5-7.6:** Add dispose pattern to YAML and XML
- **Task 7.7:** Build and test prerequisites
- **Tasks 7.8-7.18:** Migrate SDK resource structs to private arenas
- **Task 7.19:** Final integration test

---

## Task 7.1: Delete `rt_arena_destroy` shim

**Status:** `[ ]`

### Files
- `sindarin-compiler/src/runtime/arena/arena_v2.h` (lines 208-211)
- `sindarin-compiler/bin/include/runtime/arena/arena_v2.h` (lines 208-211)

### Changes
Delete the following from both files:
```c
/* Legacy destroy shim - condemn the arena for GC collection */
static inline void rt_arena_destroy(RtArenaV2 *arena) {
    if (arena) rt_arena_v2_condemn(arena);
}
```

---

## Task 7.2: Expose `rt_arena_v2_destroy` via `arena_gc.h`

**Status:** `[ ]`
**Depends on:** 7.1

### Files
- `sindarin-compiler/src/runtime/arena/arena_gc.c` (lines 36, 39)
- `sindarin-compiler/src/runtime/arena/arena_gc.h`
- `sindarin-compiler/bin/include/runtime/arena/arena_gc.h`

### Changes

**arena_gc.c:** Remove `static` from the forward declaration (line 36) and definition (line 39):
```c
// Before:
static void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);
static void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent) { ... }

// After:
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent) { ... }
```

**arena_gc.h (both source and bin/include copies):** Add declaration before `#endif`:
```c
/* Synchronously destroy an arena and all its children/handles/blocks.
 * Use for detached arenas (parent=NULL) that GC cannot reach.
 * For arenas in the GC tree, use rt_arena_v2_condemn() instead. */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);
```

---

## Task 7.3: Migrate `rt_arena_destroy` callers in random.sn.c

**Status:** `[ ]`
**Depends on:** 7.1

### Files
- `sindarin-pkg-sdk/src/core/random.sn.c` (10 call sites across 5-6 functions)

### Current pattern (broken — parentless arenas leak)
```c
RtArenaV2 *temp_arena = rt_arena_create(NULL);  // parentless — GC can't find it
// ... use temp_arena ...
rt_arena_destroy(temp_arena);                     // shim just condemns → leak
```

### New pattern (child arena — GC sweeps it)
```c
RtArenaV2 *temp_arena = rt_arena_v2_create(arena, RT_ARENA_MODE_DEFAULT, NULL);
// ... use temp_arena ...
rt_arena_v2_condemn(temp_arena);
```

### Functions to update
Each function has an arena creation + 2 call sites (error path + success path):

| Function | Create line | Condemn sites |
|----------|------------|---------------|
| `sn_random_static_weighted_choice_int` | ~865 | ~872, ~880 |
| `sn_random_static_weighted_choice_double` | ~905 | ~912, ~920 |
| `sn_random_static_weighted_choice_str` | ~940 | ~947, ~955 |
| `sn_random_weighted_choice_int` | ~1303 | ~1310, ~1318 |
| `sn_random_weighted_choice_double` | ~1343 | ~1350, ~1358 |

Verify whether `weighted_choice_str` instance method exists and has the same pattern.

---

## Task 7.4: Update documentation example

**Status:** `[ ]`
**Depends on:** 7.1

### Files
- `sindarin-compiler/docs/threading.md` (line 848)

### Changes
Replace `rt_arena_destroy(arena)` with `rt_arena_v2_condemn(arena)` in the thread cleanup example.

---

## Task 7.5: Add `handle` field and `dispose()` to YAML

**Status:** `[ ]`

### Files
- `sindarin-pkg-sdk/src/encoding/yaml.sn.c`
- `sindarin-pkg-sdk/src/encoding/yaml.sn`

### Changes to yaml.sn.c

**7.5.1** Add `handle` field to `SnYaml` struct (after `is_root`, ~line 60):
```c
typedef struct SnYaml {
    SnYamlNode *root;
    SnYamlNode *node;
    int32_t is_root;
    RtHandleV2 *handle;    /* Self-reference for dispose */
} SnYaml;
```

**7.5.2** In `sn_yaml_wrap` (~line 215), store handle after existing field assignments:
```c
y->handle = _h;
```

**7.5.3** Add `sn_yaml_dispose` function (before the parsing section, ~line 227):
```c
/* ============================================================================
 * Dispose - Deterministic Resource Cleanup
 * ============================================================================
 * Immediately frees the YAML node tree. The cleanup callback is removed so
 * arena destruction doesn't double-free. Matches the pattern in json.sn.c.
 * ============================================================================ */

void sn_yaml_dispose(SnYaml *y)
{
    if (y == NULL) return;

    if (y->is_root && y->root != NULL) {
        sn_yaml_node_free(y->root);
        y->root = NULL;
        y->node = NULL;
    }

    if (y->handle != NULL) {
        RtHandleV2 *h = y->handle;
        y->handle = NULL;
        rt_arena_v2_remove_cleanup(h->arena, h);
        rt_handle_v2_unpin(h);
        rt_arena_v2_free(h);
    }
}
```

### Changes to yaml.sn

**7.5.4** Add dispose method after `typeName()` (~line 223, before closing of struct methods):
```sindarin
    # Release this YAML value's resources immediately
    @alias "sn_yaml_dispose"
    native fn dispose(): void
```

---

## Task 7.6: Add `handle` field and `dispose()` to XML

**Status:** `[ ]`

### Files
- `sindarin-pkg-sdk/src/encoding/xml.sn.c`
- `sindarin-pkg-sdk/src/encoding/xml.sn`

### Changes to xml.sn.c

**7.6.1** Add `handle` field to `SnXml` struct (after `is_root`, ~line 29):
```c
typedef struct SnXml {
    xmlDocPtr doc;
    xmlNodePtr node;
    int32_t is_root;
    RtHandleV2 *handle;    /* Self-reference for dispose */
} SnXml;
```

**7.6.2** In `sn_xml_wrap` (~line 84), store handle after existing field assignments:
```c
x->handle = _h;
```

**7.6.3** Add `sn_xml_dispose` function (before the helper functions section, ~line 95):
```c
/* ============================================================================
 * Dispose - Deterministic Resource Cleanup
 * ============================================================================
 * Immediately frees the libxml2 document. The cleanup callback is removed so
 * arena destruction doesn't double-free. Matches the pattern in json.sn.c.
 * ============================================================================ */

void sn_xml_dispose(SnXml *x)
{
    if (x == NULL) return;

    if (x->doc != NULL) {
        xmlFreeDoc(x->doc);
        x->doc = NULL;
        x->node = NULL;
    }

    if (x->handle != NULL) {
        RtHandleV2 *h = x->handle;
        x->handle = NULL;
        rt_arena_v2_remove_cleanup(h->arena, h);
        rt_handle_v2_unpin(h);
        rt_arena_v2_free(h);
    }
}
```

### Changes to xml.sn

**7.6.4** Add dispose method after `copy()` (~line 239, before closing of struct):
```sindarin
    # Release this XML document's resources immediately
    @alias "sn_xml_dispose"
    native fn dispose(): void
```

---

## Task 7.7: Build and test

**Status:** `[ ]`
**Depends on:** 7.1, 7.2, 7.3, 7.4, 7.5, 7.6

### Verification steps
```bash
# 1. Build compiler
cd sindarin-compiler && make clean && make build

# 2. Run all compiler tests
timeout 900 make test

# 3. Copy to global
cp -r bin/* ~/.sn/bin/

# 4. Test SDK (yaml, xml, json tests exercise the dispose/cleanup paths)
cd ../sindarin-pkg-sdk && make test

# 5. Reinstall and test HTTP
cd ../sindarin-pkg-http && sn --clear-cache && rm -rf ./.sn && sn --install && make test

# 6. Return
cd ../sindarin-compiler
```

### Expected results
- Compiler: all tests pass (no codegen uses `rt_arena_destroy`)
- SDK: 28/28 pass (yaml, xml, json dispose works; random weighted choice uses child arenas)
- HTTP: 4/4 pass (no direct dependency on changed APIs)

---
---

# Phase 2: Private Arenas for Long-Lived Native Resource Structs

## Design Pattern

Every resource struct that has a factory (static create/connect/open) and a close/dispose
lifecycle switches from allocating in the caller's arena to creating a **private detached arena**.

### Before (GC-unsafe)
```c
static RtTcpStream *sn_tcp_stream_create(RtArenaV2 *arena, socket_t sock, ...) {
    RtHandleV2 *_h = rt_arena_v2_alloc(arena, sizeof(RtTcpStream));  // caller's arena
    rt_handle_v2_pin(_h);
    RtTcpStream *stream = (RtTcpStream *)_h->ptr;
    stream->arena = arena;
    // sub-allocations also in arena
    stream->read_buf = (unsigned char *)buf_h->ptr;  // cached raw ptr
    return stream;  // raw ptr returned — GC can invalidate it
}

void sn_tcp_stream_close(RtTcpStream *stream) {
    CLOSE_SOCKET(stream->socket_fd);
    rt_handle_v2_unpin(stream->addr_handle);
    rt_arena_v2_free(stream->addr_handle);
    rt_handle_v2_unpin(stream->buf_handle);
    rt_arena_v2_free(stream->buf_handle);
    rt_handle_v2_unpin(stream->self_handle);
    rt_arena_v2_free(stream->self_handle);
}
```

### After (GC-safe — private arena)
```c
static RtTcpStream *sn_tcp_stream_create(RtArenaV2 *arena, socket_t sock, ...) {
    RtArenaV2 *private = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "tcp_stream");
    RtHandleV2 *_h = rt_arena_v2_alloc(private, sizeof(RtTcpStream));
    RtTcpStream *stream = (RtTcpStream *)_h->ptr;
    stream->arena = private;  // owns the private arena
    // sub-allocations also in private arena
    stream->read_buf = (unsigned char *)buf_h->ptr;  // safe — GC can't touch private arena
    return stream;  // raw ptr — safe because block can't be compacted
}

void sn_tcp_stream_close(RtTcpStream *stream) {
    CLOSE_SOCKET(stream->socket_fd);
    RtArenaV2 *private = stream->arena;
    // stream pointer invalid after this — save everything needed first
    rt_arena_v2_destroy(private, false);  // synchronously frees everything
}
```

### Key rules
1. **Factory** creates `rt_arena_v2_create(NULL, ...)` — detached, invisible to GC
2. **All internal allocations** (struct, buffers, strings) go into the private arena
3. **Return values** to Sindarin (strings, arrays) still use the `arena` parameter (caller's arena)
4. **Close/dispose** saves the arena pointer to a local, then calls `rt_arena_v2_destroy(arena, false)`
5. **Remove all `rt_handle_v2_pin`/`rt_handle_v2_unpin` calls** — no longer needed
6. **Remove individual `rt_arena_v2_free` calls** — destroy takes care of everything
7. **Remove individual handle tracking** (`self_handle`, `buf_handle`, `addr_handle`) — optional cleanup

---

## Task 7.8: TCP — TcpStream + TcpListener private arenas

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/tcp.sn.c`

### Structs affected
- `RtTcpStream` — factory: `sn_tcp_stream_create`, close: `sn_tcp_stream_close`
- `RtTcpListener` — factory: `sn_tcp_listener_create`, close: `sn_tcp_listener_close`

### Changes
1. `sn_tcp_stream_create`: Create private arena, allocate struct + buffer + addr into it
2. `sn_tcp_stream_close`: Save socket_fd + arena to locals, close socket, `rt_arena_v2_destroy`
3. `sn_tcp_listener_create`: Create private arena, allocate struct into it
4. `sn_tcp_listener_close`: Save socket_fd + arena, close socket, `rt_arena_v2_destroy`
5. Remove all `rt_handle_v2_pin`/`unpin` calls
6. Remove individual `rt_arena_v2_free` calls from close methods
7. Can remove `self_handle`, `buf_handle`, `addr_handle` fields if no longer needed

---

## Task 7.9: TLS — TlsStream + TlsListener private arenas

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/tls.sn.c`

### Structs affected
- `RtTlsStream` — factory: `sn_tls_stream_create`, close: `sn_tls_stream_close`
- `RtTlsListener` — factory: `sn_tls_listener_create`, close: `sn_tls_listener_close`

### Changes
Same pattern as TCP. Note: TLS readLine allocates temp buffers for line extraction — these
are short-lived and can stay in the private arena (freed on arena destroy).

---

## Task 7.10: DTLS — DtlsConnection + DtlsListener private arenas

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/dtls.sn.c`

### Structs affected
- `RtDtlsConnection` — factory: `sn_dtls_connection_connect` + accept thread, close: `sn_dtls_connection_close`
- `RtDtlsListener` — factory: `sn_dtls_listener_create`, close: `sn_dtls_listener_close`

### Note
Server-side connections are created in the accept thread. The accept thread creates the private
arena for the connection. The listener's close method destroys server-side connection arenas too
(via its connection list).

---

## Task 7.11: QUIC — QuicConnection + QuicListener + QuicConfig private arenas

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/quic.sn.c`

### Structs affected
- `RtQuicConnection` — client + server variants
- `RtQuicListener` — owns server-side connections
- `RtQuicConfig` — config struct (pin-only today, lives until arena death)

### Note
Most complex networking module. Listener owns server connections — its close iterates and
destroys each connection's arena before destroying its own.

---

## Task 7.12: UDP — UdpSocket private arena

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/udp.sn.c`

### Structs affected
- `RtUdpSocket` — factory: `sn_udp_socket_create`, close: `sn_udp_socket_close`

### Note
UDP receive results (`recv_result_handle`, `recv_sender_handle`) are allocated per-call and
old ones freed at the start of the next call. These move into the private arena. Since the
private arena is never compacted, the "unpin old, pin new" pattern simplifies to just allocate
new (old freed on arena destroy).

---

## Task 7.13: SSH — SshConnection + SshListener + SshSession + SshChannel private arenas

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/ssh.sn.c`

### Structs affected
- `RtSshConnection` — factory: `sn_ssh_connection_connect`, close: `sn_ssh_connection_close`
- `RtSshListener` — factory: listener bind in `sn_ssh_listener_accept`, close: `sn_ssh_listener_close`
- `RtSshSession` — factory: `sn_ssh_listener_accept`, close: `sn_ssh_session_close`
- `RtSshChannel` — factory: `sn_ssh_session_accept_channel`, close: `sn_ssh_channel_close`
- `RtSshServerConfig` — config struct (pin-only, lives until arena death)
- `RtSshExecResult` — result struct (pin-only, lives until arena death)

### Note
Config and exec result structs are value types returned to Sindarin — these stay in the
caller's arena. Only connection/listener/session/channel get private arenas.

---

## Task 7.14: Git — GitRepo private arena

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/net/git.sn.c`

### Structs affected
- `RtGitRepo` — factories: `sn_git_repo_open/clone/init/init_bare`, close: `sn_git_repo_close`

### Note
Sub-objects (RtGitStatus, RtGitCommit, RtGitBranch, RtGitRemote, RtGitDiff, RtGitTag) are
**returned to Sindarin** as arrays — these must stay in the caller's arena. The repo struct
itself and its internal path string go into the private arena. The `arena_strdup` helper used
for sub-object strings must use the caller arena, not the repo's private arena.

---

## Task 7.15: TextFile private arena

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/io/textfile.sn.c`

### Structs affected
- `RtSnTextFile` — factory: `sn_text_file_open`, close: `sn_text_file_close`

---

## Task 7.16: BinaryFile private arena

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/io/binaryfile.sn.c`

### Structs affected
- `RtSnBinaryFile` — factory: `sn_binary_file_open`, close: `sn_binary_file_close`

---

## Task 7.17: JSON — private arena (with existing dispose)

**Status:** `[ ]`
**Depends on:** 7.7

### Files
- `sindarin-pkg-sdk/src/encoding/json.sn.c`

### Structs affected
- `SnJson` — factory: `sn_json_wrap`, dispose: `sn_json_dispose`

### Note
JSON already has dispose. The change is: wrap factory creates a private arena, dispose calls
`rt_arena_v2_destroy` instead of individual handle free. The cleanup callback should also call
`rt_arena_v2_destroy` for the safety-net path (arena destruction of parent — but since the
private arena has no parent, this means the cleanup is registered on... hmm).

**Design decision needed:** JSON wrappers are created with `sn_json_wrap(arena, ...)` where
`arena` is the caller's arena. The cleanup callback is registered on the caller's arena so
the json-c object gets freed when the caller's arena dies. With private arenas, the cleanup
callback would be on the private arena — but who triggers that arena's destruction?

Options:
- Keep JSON cleanup callbacks on the **caller's** arena (as today), but the struct itself
  lives in a private arena. The callback calls `rt_arena_v2_destroy(private_arena, false)`.
- Or: JSON objects aren't truly long-lived resources like sockets — they're typically
  function-scoped. May not need private arenas. Skip this task if so.

---

## Task 7.18: YAML + XML — private arena (with new dispose from 7.5/7.6)

**Status:** `[ ]`
**Depends on:** 7.5, 7.6, 7.7

### Files
- `sindarin-pkg-sdk/src/encoding/yaml.sn.c`
- `sindarin-pkg-sdk/src/encoding/xml.sn.c`

### Note
Same design decision as JSON (Task 7.17). These are typically parsed once and used within a
function scope. May not need private arenas. The dispose methods from 7.5/7.6 provide
deterministic cleanup within the existing arena model.

---

## Task 7.19: Final integration test

**Status:** `[ ]`
**Depends on:** 7.8-7.18

### Verification
```bash
# 1. Build compiler
cd sindarin-compiler && make clean && make build

# 2. Run all compiler tests
timeout 900 make test

# 3. Copy to global
cp -r bin/* ~/.sn/bin/

# 4. Test SDK
cd ../sindarin-pkg-sdk && make test

# 5. Reinstall and test HTTP
cd ../sindarin-pkg-http && sn --clear-cache && rm -rf ./.sn && sn --install && make test

# 6. Return
cd ../sindarin-compiler
```

### Expected results
- Compiler: all tests pass
- SDK: 28/28 pass
- HTTP: 4/4 pass
- No ASAN errors (use-after-free, leaks)
