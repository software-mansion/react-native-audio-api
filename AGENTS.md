# AGENTS.md

This repository requires `peon` feedback for agent status changes.

## Required behavior

- Use PascalCase for all component file names.
- Before finishing work always run `yarn lint:fix` and `yarn format` and `yarn typecheck`
- Do not rely only on Codex `notify` config for this repo. In this environment it may not fire for Desktop turns.

-- When you are starting the to write code, trigger peon directly:

```sh
peon play session.start
```

- When a turn reaches a completed state and you are about to send the final answer, trigger peon directly:

```sh
peon play task.complete
```

- When you need the user to step in because of approval/input/blocking, trigger peon before asking:

```sh
peon play input.required
```

- When a turn fails in a meaningful way, trigger peon before reporting the failure:

```sh
peon play task.error
```

## Verification

- If you need to verify peon locally, use:

```sh
peon notifications test
```

- Optional sound preview:

```sh
peon preview task.acknowledge
```

# Local File Lock Rules For Agents

You are operating in a shared working tree with a local file-lock daemon as the only lock authority.
Never revert any changes created not during your workflow.

How to talk to the daemon:

- Default Unix socket path: `/tmp/hodl-<uid>.sock`
- Preferred interface for agents in this repo: `npx agent-hodl ctl`
- Socket override: pass `--socket-path /custom/path.sock` to every command if the daemon is not using the default socket.
- Commands return JSON on stdout. Parse that JSON and use the returned `token` from a successful acquire as the input to `renew` and `release`.

Required command flow:

- Acquire before writing:
  `npx agent-hodl ctl acquire --path /absolute/path/to/file --owner-type agent --owner-id <task-or-thread-id> --session-id <unique-session-id>`
- If the acquire response contains `"outcome": "acquired"`, store the returned `token` and proceed.
- If the acquire response contains `"outcome": "denied"`, do not write the file. Either fail fast or retry later according to the parent task policy.
- Renew during long edits:
  `npx agent-hodl ctl renew --token <token>`
- Release immediately after the write is finished:
  `npx agent-hodl ctl release --token <token>`
- Inspect a file without acquiring:
  `npx agent-hodl ctl status --path /absolute/path/to/file`
- Wait for changes if the parent task explicitly allows waiting:
  `npx agent-hodl ctl subscribe --path /absolute/path/to/file`
  or
  `npx agent-hodl ctl subscribe --prefix /absolute/path/prefix`

Expected acquire responses:

- Success:
  `{ "outcome": "acquired", "token": "...", "canonical_path": "...", "expires_at": "...", "generation": 1 }`
- Denied:
  `{ "outcome": "denied", "holder": { ... }, "expires_at": "...", "retry_after_ms": 1234 }`

Rules:

- MUST acquire a valid file lease from the daemon before writing any file.
- MUST NOT modify a file without a current lease token for that exact canonical absolute path.
- MUST renew the lease during long edits until the write is complete.
- MUST release the lease promptly after the file is written and no longer being edited.
- MUST treat notifications and lock events as wake-up hints only, never as proof that you own a file.
- MUST re-run ACQUIRE or GET_STATUS after any relevant event before assuming a file is available.
- SHOULD fail fast if a lock is unavailable instead of waiting indefinitely.
- SHOULD use bounded retries with jitter only when the parent task explicitly allows waiting.
- Parent agents SHOULD coordinate retry policy for subagents and avoid spawning multiple contenders for the same file set.

Ownership:

- Use `owner_type=agent` for the top-level Codex agent.
- Use `owner_type=subagent` for spawned subagents.
- `owner_id` should identify the logical task or thread.
- `session_id` must be unique per running process or agent instance.

Multi-file edits:

- Prefer changing one file at a time.
- If multiple files must be edited together, acquire them in canonical sorted absolute path order.
- If any acquire fails, release every lease already obtained in that set, back off with jitter, and retry the full set only if the parent task requires it.
- Do not hold unrelated file leases while waiting for another file.

Error handling:

- If RENEW fails, assume the lease is lost immediately and stop editing until ACQUIRE succeeds again.
- If the daemon restarts or returns an epoch mismatch, assume every prior token is invalid and reacquire.
- If a lock cannot be acquired quickly, report the conflict instead of waiting forever.
