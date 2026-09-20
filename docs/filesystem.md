[back to index](../README.md#classes-and-structs)

# filesystem

```
namespace idni::fs;
```

`idni::fs` is a thin portable layer over the platform file API. It hides the difference between the POSIX calls and the Windows calls behind one set of functions. Every call that can fail returns a `diagnostics::result<T>`, aliased in this namespace as `fs::result<T>`. On failure, the error names the failing path in `label::path` and the operating system error code in `label::exit_code`.

The module lives in `src/utility/filesystem.h` (the portable API), `src/utility/temp_file.h` (the RAII temporary file), and `src/utility/memory_map.h` (the mapped-file object). Each header carries its `.tmpl.h` twin with the platform-specific definitions.


## fs::path

```
#ifdef _WIN32
using path = std::wstring;
#else
using path = std::string;
#endif
```

`fs::path` holds the encoding the platform file API takes. On Windows this is UTF-16. On every other platform this is raw bytes.

A caller that holds a UTF-8 string converts it to `fs::path` with `to_path`, and converts a `fs::path` back to UTF-8 with `to_utf8`.

```
path to_path(const std::string& utf8);
std::string to_utf8(const path& p);
```

A caller must not pass a UTF-8 string straight to a Windows file call. Windows reads that string as UTF-16 code units, not as UTF-8 bytes, and the resulting path is wrong.


## path operations

These functions take a `fs::path` and open no persistent handle.

```
bool exists(const path& p);
result<size_t> size(const path& p);
result<bool> unlink(const path& p);
result<bool> write(const path& p, std::string_view content);
result<path> temp_dir();
result<path> temp_filename(const std::string& prefix);
```

`exists` answers a plain `bool`, not a `result<bool>`. A missing file is not an error, so the caller needs no error branch to ask the question.

`size` reads the size of a file, in bytes. `unlink` deletes a file. `write` creates or overwrites a file with the given content.

`temp_dir` finds the directory for temporary files. It asks `std::filesystem` first. On Windows it then falls back to a native temporary-path lookup, which can still fail. On POSIX it falls back to `/tmp` and never fails.

`temp_filename` reserves a fresh, uniquely named temporary file and creates it. It does not only choose a name: once the call returns a value, the file exists on disk and is empty. It validates the prefix as UTF-8 first. On Windows, it keeps only the first three UTF-16 units of the prefix, and records a warning when it must cut the prefix. On POSIX, it rejects a prefix that holds a path separator or a NUL.


## the handle and mapping operations

```
using file_handle = HANDLE;   // Windows
using file_handle = int;      // everywhere else

inline const file_handle invalid_file_handle = ...;

enum mmap_mode { MMAP_NONE, MMAP_READ, MMAP_WRITE };

struct mapping {
    void*  data;
    size_t size;
};

result<file_handle> open(const path& p, mmap_mode m);
result<file_handle> create(const path& p, mmap_mode m);
result<bool>         close(file_handle f);
result<size_t>       size(file_handle f);
result<bool>         resize(file_handle f, size_t n);
result<size_t>       write(file_handle f, std::string_view content);
result<mapping>      map(file_handle f, size_t n, mmap_mode m);
result<bool>         sync(file_handle f, const mapping& r);
result<bool>         unmap(mapping& r);
```

A `file_handle` is a raw handle, not an owning type. The caller that opens or creates one closes it with `close`. `invalid_file_handle` is the value that never names an open file.

`open` opens an existing file. `create` creates a file, or opens it if it already exists. `create` differs from `open` only on POSIX. On Windows, `create` calls the same `CreateFileW`, because its `OPEN_ALWAYS` flag already creates a missing file.

`size`, `resize`, and `write` act on an already open handle. `write` on a handle never checks for a short write. The caller compares the returned count against the size of `content` itself, unlike `write(const path&, ...)`, which does check.

`map` maps an open file into memory and returns a `mapping`. The file named by `f` must stay open for the life of the mapping. `sync` flushes a mapped region to disk. It needs `f` because Windows also flushes the file handle, not only the mapped view. `unmap` undoes a mapping and resets its fields to their default state on success.


## temp_file

`fs::temp_file` is an RAII temporary file.

`temp_file::create` reserves the file with `temp_filename`, then writes the given content into it. The object takes ownership of the file as soon as the file exists, before the write runs. A failing write therefore still leaves the destructor free to delete the file.

`path()` returns the path of the file in UTF-8, not in the native path encoding.

The destructor deletes the file it owns. A moved-from `temp_file` owns nothing, so its destructor deletes nothing.


## memory_map

`fs::memory_map` opens a file and maps it into memory as two separate steps. It moves through three states.

| State | Meaning | Reached by |
|---|---|---|
| `CLOSED` | No file is open. | Default construction, or a successful `close`. |
| `UNMAPPED` | The file is open, not mapped. | A successful `open`. |
| `MAPPED` | The file is open and mapped. | A successful `map`. |

`open` needs the `CLOSED` state. `map` needs the `UNMAPPED` state. `unlink` needs the `CLOSED` state. `unmap` needs the `MAPPED` state. A call outside that state is not a failure, but a redundant call. `close` unmaps first when the object is `MAPPED`, then closes the handle, and moves to `CLOSED` from any state.

`data()` answers null unless the state is `MAPPED`.

The constructor takes a size. Zero, its default, means autodetect the size from the file once it is open. An empty filename means a temporary file, which `close` deletes.

Each operation returns a `result<bool>`. The object also keeps a `report` of its own, reached through `has_error()` and `report()`, so a caller can inspect a past failure later. A redundant `close` or `unmap` records a warning and answers `false`, not an error. A failing `open` leaves no open handle and no temporary file behind.


## an ordinary path

Open a file, map it, read it, then close it.

```cpp
using namespace idni::fs;

memory_map mm(to_path("input.txt"), 0, MMAP_READ, false, false);

if (!mm.open().has_value()) { /* mm.report() holds the reason */ }
if (!mm.map().has_value())  { /* mm.report() holds the reason */ }

const char* data = static_cast<const char*>(mm.data());
// read mm.size() bytes starting at data

mm.close();
```
