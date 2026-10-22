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
it, configure uses `-DTAU_BOOST_INCLUDE_DIR=<dir>` or a system Boost 1.86 or
newer.

The `-all` presets turn the option on. The option is always off for
Emscripten.

## tgf serve

    tgf [<grammar>] serve [--port N] [--json] [--debug-sessions] [--max-sessions N]
        [--idle-timeout N] [--session-memory N] [--max-line N]
        [--write-timeout N] [--first-line-timeout N] [--max-pending N]
        [--max-open-requests N] [--max-answer N] [--max-stderr-line N]
        [--detached-buffer N] [--end-timeout N]
        [--enabled-productions LIST] [--log-dir DIR] [--no-log]

The grammar file is optional. Without it every session starts with no
grammar and a `load` command installs one.

| Option | Default | Meaning |
|---|---|---|
| `--port N`, `-p N` | 4000 | TCP port. 0 lets the system pick a free port. |
| `--json`, `-J` | off | Write JSON lines on stdout instead of text lines. |
| `--debug-sessions`, `-d` | off | Also print each session log line (`in`, `out`, `stderr`) on stdout. It works with `--no-log` too. |
| `--max-sessions N` | 16 | Refuse a new session at this count. |
| `--idle-timeout N` | 30 | End a session after N minutes with no traffic. 0 disables the timeout. |
| `--session-memory N` | 0 | Address space limit of a session in MB. 0 means no limit. POSIX only. |
| `--max-line N` | 16 MiB | Largest request line in bytes. A longer line is an error and its tail is dropped up to the next newline. The session child reads with the same limit, so the option holds end to end. |
| `--write-timeout N` | 30 | Close a connection that makes no write progress on its queued output for N seconds. The deadline restarts on every completed write. 0 disables the deadline. |
| `--first-line-timeout N` | 30 | Close a connection that sends no first line within N seconds. 0 disables the deadline. |
| `--max-pending N` | 16 | Refuse a new connection while N connections have sent no first line. |
| `--max-open-requests N` | 64 | Stop the read of a client while N of its requests wait for an answer of the child. The read also stops while `--max-line` bytes wait for the child. |
| `--max-answer N` | 16 MiB | Largest answer line of a child in bytes. A longer line ends the session. `connect` refuses a longer answer. |
| `--max-stderr-line N` | 1 MiB | Largest stderr line of a child in bytes. The tail of a longer line is dropped. |
| `--detached-buffer N` | 16 MiB | Child answers in bytes that a session keeps while no client is attached. |
| `--end-timeout N` | 5 | Stop a child that does not exit within N seconds after `end-session`, or after the server ends the session. |
| `--log-dir DIR` | `~/.tau/tgf/logs` | Directory of the session logs and `server.log`. With no home folder, give `--log-dir` or `--no-log`. |
| `--no-log` | off | Write no logs. |
| `--start SYM`, `-s SYM` | none | Initial start symbol of each session. |
| `--enabled-productions LIST`, `-G LIST` | none | Comma-separated production guards each session enables. |

The server binds `127.0.0.1` only. No host option exists. When the port
is busy, the server stops with an error that names the port. Use `-p <port>`
for another port, or `-p 0` to let the system pick a free port.

Without `--json`, the server writes text lines on stdout. The first lines
give the port and the log directory. Then each server event writes one
line. Each line starts with the UTC time, the same `t` value that the log
line of the event gets:

    2026-10-13T08:02:14.328Z listening on 127.0.0.1:4000
    2026-10-13T08:02:14.328Z logs in ~/.tau/tgf/logs
    2026-10-13T08:02:20.101Z session bf48da16a1c0e1f2a3b4c5d6e7f80910 new
    2026-10-13T08:02:31.554Z session bf48da16a1c0e1f2a3b4c5d6e7f80910 detach
    2026-10-13T08:02:40.012Z session bf48da16a1c0e1f2a3b4c5d6e7f80910 exit 143

With `--json`, stdout gets JSON lines only. The first line gives the port,
and each server event writes the JSON object that `server.log` gets. The
`event_line` definition of the API schema describes these lines:

    {"listening":{"kind":"tcp","port":4000}}
    {"t":"2026-10-13T10:15:03.123Z","dir":"event","event":"new","session":"bf48..."}

With `--debug-sessions`, each line that goes to a session log also goes to
stdout, with the full payload. A text line has the time and session prefix
of the event lines, then the direction:

    2026-10-13T08:02:14.328Z session ff267e05604ed0bd9ce56640790485e0 in  {"cmd":"version"}
    2026-10-13T08:02:14.330Z session ff267e05604ed0bd9ce56640790485e0 out {"id":null,"cmd":"version",...}

With `--json`, the line is the JSON object of the session log plus a
`session` field. The `debug_line` definition of the API schema describes
it.

A program that reads the port starts the server with `--json -p 0`. The
server prints its report (errors and warnings) at the end, as text or as
one JSON line. The log files always hold JSON. A text report has colors
only when the `colors` option is on, in every tgf command. `-c false`
turns them off.

A SIGINT or SIGTERM stops the server and ends its sessions.

## tgf connect

    tgf [<grammar>] connect [<address>] [--host H] [--port N]
        [--session ID] [--json] [--colors BOOL] [--legacy-repl]

    tgf connect
    tgf connect -p 5000
    tgf connect tgf://example.org:5000
    tgf connect [::1]:5000
    tgf g.tgf connect --session 0123456789abcdef0123456789abcdef

`connect` reaches the server at `--host` (`-H`, `localhost` by default)
on `--port` (`-p`, 4000 by default). Port 0 is an error. An address
argument gives the host and the port instead. It has one of these forms:

- `tgf://<host>[:<port>][/]`
- `<host>[:<port>]`
- `[<ipv6>][:<port>]`, an IPv6 address in brackets

A missing port is 4000. Another scheme, a path, a query, a fragment, user
info or a bad port is an error that names the address. An address argument
together with `--host` or `--port` is an error. Only the command line
counts here, not a default or an environment value. The client resolves the
host and tries each of its addresses. A host that does not resolve gives
an error with the host. A failed connect gives an error with the host and
the port. On glibc, a non-ASCII host name resolves through IDN. Without
IDN support, the client gives an error for such a name.

The server binds `127.0.0.1` only. To reach it from another machine, use
port forwarding (for example `ssh -L 4000:127.0.0.1:4000 server`) or
`tailscale serve`.

Without `--session` the client starts a new session, otherwise it attaches
to the session with that id.
`--legacy-repl` uses the plain terminal REPL instead of the FTXUI one. The
client prints its session id at start, so a user can attach again later.

With a grammar file, the client loads the file into the session after the
connect, the same as the REPL command `load "<file>"`. This also replaces
the grammar of an attached session. When the load fails, the client prints
the report and exits with 1. Without a grammar file, the session keeps its
grammar.

A specialized binary, for example `json_grammar`, serves its built-in
grammar with `serve`. Its `connect` sends no grammar, so the session keeps
its grammar.

The client prompt is the prompt of `tgf repl`. With no grammar it is
`[ (no grammar loaded) ] tgf> `. The client reads the `colors` and
`status` options from its own command line and environment, for example
`--colors false`. The client sends nothing about colors to the server. The
print options belong to the session, so `set` changes them on the server.

With `--json`, the client reads one JSON request per line on stdin and
writes each answer of the server as one line on stdout, the protocol of
`tgf repl --json`. It writes the `hello` line first, then the answer of
the grammar load, if any. It shows no prompt and no colors. A `load` or a
`parse_file` request with `file` reads the file on the client. The loop
stops at the end of stdin or after a `quit` answer.

The client parses each REPL line itself. `load "f"` and `parse file "f"`
read the file on the client and send its text; the server never reads a
path a client gives. `quit` ends the client and leaves the session alive.

An attach prints the answers the session buffered while no client was
attached before the first prompt, under one `earlier answers:` header,
with the same renderer as a normal answer.

## tgf-connect

`tgf-connect` is a small program that runs only the client of
`tgf connect`. It has no grammar loader, no parser generator and no server.

    ./dev preset release-tgf-connect

The CMake option `TAU_PARSER_BUILD_CONNECT` builds it. The option is off by
default, and it does not need `TAU_PARSER_BUILD_TGF`. It needs the Boost
headers, the same as `TAU_PARSER_BUILD_SERVE`.

    tgf-connect [<address>] [<grammar>] [--host H] [--port N]
        [--session ID] [--json] [--colors BOOL] [--legacy-repl]

    tgf-connect -p 5000
    tgf-connect g.tgf tgf://example.org:5000

The options and the address forms are those of `tgf connect`. An argument
that names a file or ends in `.tgf` is the grammar file. Any other argument
is the address. A `.tgf` name that does not exist gives an error before the
connect. The client loads the grammar file into the session, the same as
`tgf <grammar> connect`.

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
buffer of `--detached-buffer` bytes. Past the limit the server drops the oldest lines and the
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
exit within `--end-timeout` seconds is stopped with `SIGKILL` on POSIX; on
Windows the server calls `TerminateProcess`. An idle timeout or a lost pipe
sends `SIGTERM` first and does not wait for the exit: the reap writes
`child-exit` later.

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
`child-exit` (with `exit`). `first-line-timeout` (with `kind`) goes into
`server.log` only, because the connection has no session yet. `stderr` carries the standard error of the
child in `line`. The server never deletes a log. A failed log write goes in
the report of the server once and never stops it.

## Security

A REPL on a socket runs code for any client, so the server listens on
`127.0.0.1` only. A remote user needs an SSH tunnel. Any local user can
open a new session, and only the session id protects an attached session.
The id is a secret of the server and of the client: the server logs it and
the client prints it, so do not share them.

A session refuses `parse_file` and `load` with `file` with the
diagnostics code `server_path`. `load` with `name` and `source` works.
`reload` reloads the stored text of the session grammar, also for a
session that the server started with a grammar file.

## Tests

The serve and connect doctests run on Linux, macOS and Windows, and in
the MinGW cross build under wine. Every platform starts the `tgf` binary
with `serve --json --port 0` through `tests/serve/tgf_serve_process.h` and reads
the `listening` line; no test forks. `tests/serve/tgf_serve_smoke.py`
drives a real `tgf serve` process through `tests/serve/harness.py`, which
reads process lines with a thread and uses socket timeouts; a cross build
runs the native `python3` and passes the emulator before `tgf.exe`.
