# GC Tasks V4: Private Arenas for Native Resource Structs

## Status Legend
- [ ] Not started
- [~] In progress
- [x] Complete

---

## Prerequisites (from gc-tasks-v3.md — ALL COMPLETE)

- [x] Task 7.1: Delete `rt_arena_destroy` shim
- [x] Task 7.2: Expose `rt_arena_v2_destroy` via `arena_gc.h`
- [x] Task 7.3: Migrate `rt_arena_destroy` callers in `random.sn.c`
- [x] Task 7.4: Update `threading.md` doc example
- [x] Task 7.5: Add `handle` field + `dispose()` to YAML
- [x] Task 7.6: Add `handle` field + `dispose()` to XML
- [x] Task 7.7: Build and test (compiler 1380, SDK 28, HTTP 4 — all pass)

---

## Context

Long-lived native opaque structs (TcpStream, TlsStream, UdpSocket, SshConnection, GitRepo,
TextFile, BinaryFile, etc.) are allocated in the function arena. Their factory methods extract
`handle->ptr` into a raw `T*` pointer which is returned to Sindarin and stored as the variable
value. All subsequent method calls pass this raw pointer. If GC compacts the block,
`handle->ptr` is updated but the cached raw pointer becomes dangling — every subsequent method
call is a use-after-move.

Additionally, these structs cache raw pointers to internal sub-allocations (read buffers,
address strings) which are also invalidated by compaction.

**Solution:** Each resource struct creates a **private, detached arena** (`parent=NULL`) that
is invisible to GC. Since GC never discovers it, it never compacts it, and raw pointers remain
valid for the struct's entire lifetime. The `close()`/`dispose()` method calls
`rt_arena_v2_destroy()` to synchronously free everything.

## Design Pattern

### Before (GC-unsafe)
```c
RtTcpStream *sn_tcp_stream_create(RtArenaV2 *arena, ...) {
    RtHandleV2 *_h = rt_arena_v2_alloc(arena, sizeof(RtTcpStream));  // caller's arena
    rt_handle_v2_pin(_h);
    RtTcpStream *s = (RtTcpStream *)_h->ptr;
    s->read_buf = (unsigned char *)buf_h->ptr;  // cached raw ptr — GC can invalidate
    return s;  // raw ptr — GC can invalidate
}
void sn_tcp_stream_close(RtTcpStream *s) {
    CLOSE_SOCKET(s->socket_fd);
    rt_handle_v2_unpin(s->addr_handle);
    rt_arena_v2_free(s->addr_handle);
    // ... more individual frees ...
}
```

### After (GC-safe)
```c
RtTcpStream *sn_tcp_stream_create(RtArenaV2 *arena, ...) {
    RtArenaV2 *priv = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "tcp_stream");
    RtHandleV2 *_h = rt_arena_v2_alloc(priv, sizeof(RtTcpStream));
    RtTcpStream *s = (RtTcpStream *)_h->ptr;
    s->arena = priv;  // owns the private arena
    s->read_buf = (unsigned char *)buf_h->ptr;  // safe — GC can't touch detached arena
    return s;  // safe — block can't be compacted
}
void sn_tcp_stream_close(RtTcpStream *s) {
    socket_t fd = s->socket_fd;
    RtArenaV2 *priv = s->arena;
    // s pointer invalid after destroy — save everything needed first
    CLOSE_SOCKET(fd);
    rt_arena_v2_destroy(priv, false);  // synchronously frees everything
}
```

### Key rules
1. **Factory** creates `rt_arena_v2_create(NULL, ...)` — detached, invisible to GC
2. **All internal allocations** (struct, buffers, strings) go into private arena
3. **Return values** to Sindarin (strings, arrays) still use the `arena` parameter (caller's arena)
4. **Close/dispose** saves arena pointer to local, then calls `rt_arena_v2_destroy(arena, false)`
5. **Remove all `rt_handle_v2_pin`/`unpin` calls** — no longer needed
6. **Remove individual `rt_arena_v2_free` calls** — destroy handles everything
7. **Remove handle-tracking fields** (`self_handle`, `buf_handle`, `addr_handle`) — optional cleanup

---

## Phase 1: Network Resource Structs

### Task 7.8: TCP — TcpStream + TcpListener
- [ ] `sindarin-pkg-sdk/src/net/tcp.sn.c`
- [ ] `sn_tcp_stream_create` → private arena, remove pin (3 sites)
- [ ] `sn_tcp_stream_close` → save locals, destroy arena, remove unpin (3) + individual frees
- [ ] `sn_tcp_listener_create` → private arena, remove pin (1)
- [ ] `sn_tcp_listener_close` → destroy arena, remove unpin (1)

### Task 7.9: TLS — TlsStream + TlsListener
- [ ] `sindarin-pkg-sdk/src/net/tls.sn.c`
- [ ] `sn_tls_stream_create` → private arena, remove pin (3)
- [ ] `sn_tls_stream_read_line` → temp buffers in private arena, remove pin (2)
- [ ] `sn_tls_stream_close` → destroy arena, remove unpin (3)
- [ ] `sn_tls_listener_bind` → private arena, remove pin (1)
- [ ] `sn_tls_listener_close` → destroy arena, remove unpin (1)

### Task 7.10: DTLS — DtlsConnection + DtlsListener
- [ ] `sindarin-pkg-sdk/src/net/dtls.sn.c`
- [ ] `sn_dtls_connection_connect` → private arena, remove pin (2)
- [ ] Server-side connections (accept thread) → private arenas
- [ ] `sn_dtls_connection_close` → destroy arena, remove unpin (2)
- [ ] `sn_dtls_listener_create` → private arena, remove pin (1)
- [ ] `sn_dtls_listener_close` → destroy arena, remove unpin (1)

### Task 7.11: QUIC — QuicConnection + QuicListener + QuicConfig
- [ ] `sindarin-pkg-sdk/src/net/quic.sn.c`
- [ ] Client + server connection → private arenas
- [ ] Listener owns server connections — close iterates and destroys each
- [ ] QuicConfig → private arena
- [ ] Remove all pin (7) and unpin (5) calls

### Task 7.12: UDP — UdpSocket
- [ ] `sindarin-pkg-sdk/src/net/udp.sn.c`
- [ ] `sn_udp_socket_create` → private arena, remove pin (1)
- [ ] `sn_udp_socket_receive_from` → per-call allocs in private arena, remove pin (4) + unpin (2)
- [ ] `sn_udp_socket_close` → destroy arena, remove unpin (3)

### Task 7.13: SSH — SshConnection + SshListener + SshSession + SshChannel
- [ ] `sindarin-pkg-sdk/src/net/ssh.sn.c`
- [ ] 4 resource types with lifecycle → private arenas
- [ ] Config + ExecResult are values returned to caller → caller arena
- [ ] Remove all pin (17) and unpin (7) calls

### Task 7.14: Git — GitRepo
- [ ] `sindarin-pkg-sdk/src/net/git.sn.c`
- [ ] `sn_git_repo_open/clone/init/init_bare` → private arena
- [ ] Sub-objects returned to Sindarin → caller arena
- [ ] `arena_strdup` helper for sub-objects must use caller arena
- [ ] `sn_git_repo_close` → destroy private arena
- [ ] Remove all pin (20) and unpin (2) calls

---

## Phase 2: IO Resource Structs

### Task 7.15: TextFile
- [ ] `sindarin-pkg-sdk/src/io/textfile.sn.c`
- [ ] `sn_text_file_open` → private arena, remove pin (2)
- [ ] `sn_text_file_close` → destroy arena, remove unpin (2) + individual frees

### Task 7.16: BinaryFile
- [ ] `sindarin-pkg-sdk/src/io/binaryfile.sn.c`
- [ ] `sn_binary_file_open` → private arena, remove pin (2)
- [ ] `sn_binary_file_close` → destroy arena, remove unpin (2) + individual frees

---

## Phase 3: Encoding Structs (Deferred)

### Task 7.17: JSON — SKIP
JSON objects are function-scoped, not long-lived OS resources. dispose() already provides
deterministic cleanup. Cleanup callback on caller arena is the safety net.

### Task 7.18: YAML + XML — SKIP
Same rationale as JSON. dispose() methods already added (Tasks 7.5/7.6).

---

## Phase 4: Cleanup

### Task 7.19: Remove pin/unpin stubs from runtime
- [ ] `sindarin-compiler/src/runtime/arena/arena_handle.h` (both src + bin copies)
- [ ] Delete `rt_handle_v2_pin`/`rt_handle_v2_unpin` no-op stubs
- [ ] Delete `RT_HANDLE_FLAG_PINNED` from flags enum
- [ ] Clean up stale comments in runtime + codegen

### Task 7.20: Remove remaining pin calls in non-resource files
- [ ] `random.sn.c` — 3 post-alloc pins
- [ ] `uuid.sn.c` — 12 post-alloc pins
- [ ] `process.sn.c` — 7 post-alloc pins
- [ ] `env.sn.c` — 1 post-alloc pin
- [ ] `date.sn.c` — 2 post-alloc pins
- [ ] `time.sn.c` — 1 post-alloc pin
- [ ] `directory.sn.c` — 6 post-alloc pins
- [ ] `path.sn.c` — 2 pin/unpin pairs
- [ ] `json.sn.c` — 2 pins + 1 unpin (cleanup callback + wrap + dispose)
- [ ] `yaml.sn.c` — 2 pins + 1 unpin (cleanup callback + wrap + dispose)
- [ ] `xml.sn.c` — 2 pins + 1 unpin (cleanup callback + wrap + dispose)

All are simple deletes — pin/unpin are no-ops.

---

## Phase 5: Final Integration Test

### Task 7.21: Full test suite
- [ ] Build compiler: `make clean && make build`
- [ ] Run compiler tests: `timeout 900 make test`
- [ ] Install global: `cp -r bin/* ~/.sn/bin/`
- [ ] Test SDK: `cd ../sindarin-pkg-sdk && make test`
- [ ] Reinstall + test HTTP: `cd ../sindarin-pkg-http && sn --clear-cache && rm -rf ./.sn && sn --install && make test`
- [ ] Verify zero pin/unpin references: `grep -r "rt_handle_v2_pin\|rt_handle_v2_unpin" ../sindarin-pkg-sdk/src/ src/runtime/ bin/include/runtime/`
- [ ] No ASAN errors (use-after-free, leaks)
