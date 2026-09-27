# tgf serve and tgf connect

`tgf serve` keeps grammar sessions for clients on TCP. A session is one
child process that runs the JSON API, so a session lives longer than one
connection: a client can detach and attach again until the session ends.
`tgf connect` is a text REPL that runs its commands in a session.

The JSON API itself is in `docs/tgf_json_api.md`.

## Build

Both commands are behind the CMake option `TAU_PARSER_BUILD_SERVE`, which
is off by default. The option needs the Boost headers, only for Asio; the
parser library itself never uses Boost.

    ./dev preset release -DTAU_PARSER_BUILD_SERVE=ON -DTAU_PARSER_DEPS_FROM_STORE=ON

With the store option, configure gets the headers from the store. Without
it, configure uses -DTAU_BOOST_INCLUDE_DIR=<dir> or a system Boost 1.86 or
newer.

The `-all` presets turn the option on. The option is always off for
Emscripten.

## tgf serve

    tgf [<grammar>] serve [--port N] [--max-sessions N]
        [--idle-timeout N] [--session-memory N] [--max-line N]
        [--write-timeout N] [--productions LIST] [--log-dir DIR]
        [--no-log]

The grammar file is optional. Without it every session starts with no
grammar and a `load` command installs one.

| Option | Default | Meaning |
|---|---|---|
| `--port N`, `-p N` | 0 | TCP port. 0 lets the system pick a free port. |
| `--max-sessions N` | 16 | Refuse a new session at this count. |
| `--idle-timeout N` | 30 | End a session after N minutes with no traffic. 0 disables the timeout. |
| `--session-memory N` | 0 | Address space limit of a session in MB. 0 means no limit. POSIX only. |
| `--max-line N` | 16 MiB | Largest request line in bytes. A longer line is an error and its tail is dropped up to the next newline. The session child reads with the same limit, so the option holds end to end. |
| `--write-timeout N` | 30 | Close a connection that makes no write progress on its queued output for N seconds. The deadline restarts on every completed write. 0 disables the deadline. |
| `--log-dir DIR` | `~/.tau/tgf/logs` | Directory of the session logs and `server.log`. |
| `--no-log` | off | Write no logs. |
| `--start SYM`, `-s SYM` | none | Initial start symbol of each session. |
| `--productions LIST`, `-G LIST` | none | Comma-separated production guards each session enables. |

The server binds `127.0.0.1` only. No host option exists. With `--port 0`
the system picks a port and the server writes one line on stdout:

    {"listening":5000}

A SIGINT or SIGTERM stops the server and ends its sessions.

## tgf connect

    tgf connect <host>:<port> [--session ID] [--legacy-repl]

    tgf connect 127.0.0.1:5000
    tgf connect 127.0.0.1:5000 --session 0123456789abcdef0123456789abcdef

`connect` needs no grammar file; the session holds the grammar. Without
`--session` it starts a new session, otherwise it attaches to the session
with that id. `--legacy-repl` uses the plain terminal REPL instead of the
FTXUI one. The client prints its session id at start, so a user can attach
again later.

The client parses each REPL line itself. `load "f"` and `parse file "f"`
read the file on the client and send its text; the server never reads a
path a client gives. `quit` ends the client and leaves the session alive.

An attach prints the answers the session buffered while no client was
attached before the first prompt, under one `earlier answers:` header,
with the same renderer as a normal answer.

## The session

The first line of a connection picks a session:

    {"cmd":"new"}
    {"cmd":"attach","session":"0123456789abcdef0123456789abcdef"}

A new session gets a random 128-bit id in hex, 32 characters. The server
answers with the `hello` line of the child. An attach sends the lines the
session buffered while no client was attached, then asks the child for a
fresh `hello`:

    {"id":7,"cmd":"hello","status":"ok","hello":{...},"state":{...}}

An unknown id, a session that already has a client, and a session count
past `--max-sessions` give an error.

While no client is attached the session keeps the child responses in a
buffer of 16 MiB. Past the limit the server drops the oldest lines and the
session stays.

`quit` is forwarded to the child. The child answers with the status `quit`
and goes on, the server relays the answer and closes the connection; the
session stays. An `eval` request with `quit` inside closes neither. The
answer is:

    {"id":8,"cmd":"quit","status":"quit","result":{},
     "state":{...},"report":{...}}

`end-session` closes the input of the child and the server answers it
itself, with no `state`:

    {"id":9,"cmd":"quit","status":"quit"}

The server removes the session when the child exits. A child that does not
exit after the end grace is stopped with `SIGKILL` on POSIX; on Windows
the server calls `TerminateProcess`.

The idle timeout ends a session with no traffic. While a request waits for
its answer, the timeout counts from the last child output, so a child that
sends nothing for the whole timeout is ended too.

A server answer that the server builds itself, such as an unknown session
id or a request line over the limit, carries no `state`:

    {"id":null,"status":"error","report":{...}}

The server reads its grammar file once at start and keeps those bytes,
so every session and every reload sees the bytes of the load, not a later
change of the file. A file that cannot be read at start fails the server
with a report error.

On POSIX the server forks after the grammar load, so the child starts
warm. The parent runs one thread and one Asio `io_context`; a fork in a
process with more threads is not safe. On Windows the server starts the
same program with `repl --json --session <id> --init-stdin` and three
overlapped named pipes, then writes one init line with the grammar text,
the start symbol, the option values and the line limit. The child applies
the init line
and only then writes its first `hello`, so a client sees the grammar and
the options of the server at once. A built-in grammar is compiled into
the child, so the init line carries no grammar and the child keeps it;
with no grammar the line carries `null`. `--session-memory` has no
equivalent on Windows; the server warns in its report and starts the
child without a limit.

## Logs

Unless `--no-log` is given, the server writes JSON lines logs in
`--log-dir`, default `~/.tau/tgf/logs`. The directory is created when it
is absent. Each session gets one file, `<start time>-<session id>.log`, and
the server keeps one `server.log`. One line is:

    {"t":<time>,"dir":"in"|"out"|"event"|"stderr",...}

`t` is the UTC time in ISO 8601 with milliseconds, for example
`"2026-09-27T10:15:03.123Z"`. `in` and `out` carry the request and the
response line in `line`, including the first line of a connection (`new`
or `attach`). In a Windows session the first `in` line is the init line
the parent writes to the child. `event` carries `event` and,
for a session event,
`session`; the events are `new`, `attach`, `detach`, `quit`, `end-session`,
`idle-timeout`, `line-too-long`, `dropped-buffer-lines` (with `count`) and
`child-exit` (with `exit`). `stderr` carries the standard error of the
child in `line`. The server never deletes a log. A failed log write goes in
the report of the server once and never stops it.

## Security

A REPL on a socket runs code for any client, so the server listens on
`127.0.0.1` only. A remote user needs an SSH tunnel. Any local user can
open a new session, and only the session id protects an attached session.
The id is a secret of the server and of the client: the server logs it and
the client prints it, so do not share them.

A session refuses `parse file` and `load` with `file` with the
diagnostics code `server_path`. `load` with `name` and `source` works.
`reload` reloads the stored text of the session grammar, also for a
session that the server started with a grammar file.

## Tests

The serve and connect doctests run on Linux, macOS and Windows, and in
the MinGW cross build under wine. Every platform starts the `tgf` binary
with `serve --port 0` through `tests/serve/tgf_serve_process.h` and reads
the `listening` line; no test forks. `tests/serve/tgf_serve_smoke.py`
drives a real `tgf serve` process through `tests/serve/harness.py`, which
reads process lines with a thread and uses socket timeouts; a cross build
runs the native `python3` and passes the emulator before `tgf.exe`.
