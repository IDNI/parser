// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
#define __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <sstream>
#include <iostream>
#include <memory>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
static inline std::string temp_filename() {
	TCHAR name[MAX_PATH], path[MAX_PATH];
	DWORD r = GetTempPath(MAX_PATH, path);
	if (r > MAX_PATH || !r ||
		!GetTempFileName(path, TEXT("MMAPXXXX"), 0, name)) return "";
	return std::string(name);
}
#else
static inline int temp_fileno() { return fileno(tmpfile()); }
static inline std::string filename(int fd) {
        return std::filesystem::read_symlink(
                        std::filesystem::path("/proc/self/fd") /
                                std::to_string(fd));
}
#endif

enum mmap_mode { MMAP_NONE, MMAP_READ, MMAP_WRITE };

class memory_map {
public:
	bool silent = true; // true to disable printing messages to cerr
	bool error = false;
	std::string error_message = "";
	memory_map() : mode_(MMAP_NONE),state_(CLOSED),filename_(""),size_(0) {}
	memory_map(std::string filename, size_t s=0, mmap_mode m = MMAP_READ,
		bool do_open=1, bool do_map=1)
		: mode_(m), state_(CLOSED), filename_(filename), size_(s)
	{
		if (mode_ == MMAP_NONE) return;
		if (do_open || do_map) if (open() == -1) return;
		if (do_map) map();
	}
	~memory_map() { close(); }
	size_t size() const { return size_; }
	std::string file_name() const { return filename_; }
	void clear_error() { error = false, error_message = ""; }
	void* data() {
		if (state_ != MAPPED) return 0;
		return data_;
	}
	int open() {
		if (mode_  == MMAP_NONE) return err("none mmap - cannot");
		if (state_ != CLOSED)    return err("file is already opened");
#ifdef _WIN32
		if (filename_ == "") create_temp();
		fh_ = CreateFileA(filename_.c_str(),
			mode_ == MMAP_READ ? GENERIC_READ
				: GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (fh_ == INVALID_HANDLE_VALUE) return err("cannot open file");
#else
		if (mode_ == MMAP_WRITE && (filename_ == "" || !file_exists()))
			create();
		else {
			fd_ = ::open(filename_.c_str(),mode_ == MMAP_READ ?
				O_RDONLY : O_RDWR, 0600);
    			if (fd_ == -1) return err(errno, "cannot open file");
			if (mode_ == MMAP_WRITE)
				if (truncate() == -1) return -1;
		}
#endif
		state_ = UNMAPPED;
		if (!size_) { // autodetect map size
			size_ = file_size();
		}
		return 0;
	}
	int map() {
		if (state_ != UNMAPPED)
			return err("file is not opened or already mapped");
#ifdef _WIN32
		mh_ = ::CreateFileMapping(fh_, 0, mode_ == MMAP_READ ?
			PAGE_READONLY : PAGE_READWRITE, 0, size_, 0);
    		if (mh_ == INVALID_HANDLE_VALUE)
			return data_ = 0, err("mmap err mfd");
		data_ = ::MapViewOfFile(mh_, mode_ == MMAP_READ ? FILE_MAP_READ
			: FILE_MAP_WRITE, 0, 0, size_);
		if (data_ == 0) {
			::CloseHandle(mh_);
			return err(GetLastError(), "mmap err");
		}
#else
		data_ = ::mmap(0, size_, mode_ == MMAP_READ ? PROT_READ :
			PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
		if (data_==MAP_FAILED) return data_=0,err(errno, "mmap err");
#endif
		state_ = MAPPED;
		return 0;
	}
	int sync() {
		int r = 0;
#ifdef _WIN32
		if (::FlushViewOfFile(data_, size_) == 0 ||
			::FlushFileBuffers(fh_) == 0) r = -1;
#else
		r = ::msync(data_, size_, MS_SYNC);
#endif
		if (r == -1) return err(errno, "memory_map: sync");
		return r;
	}
	int unmap() {
		if (state_ != MAPPED)
			return err("file not mapped, cannot be unmapped\n");
		if (sync() == -1) return -1;
		int r = 0;
#ifdef _WIN32
		::UnmapViewOfFile(data_);
		::CloseHandle(mh_);
#else
		r = ::munmap(data_, size_);
#endif
		data_ = 0, state_ = UNMAPPED;
		return r;
	}
	int unlink() {
		if (state_ != CLOSED) return err(
			"file is not closed. it cannot be deleted\n");
#ifdef _WIN32
		return ::DeleteFileA(filename_.c_str());
#else
		return ::unlink(filename_.c_str());
#endif
	}
	int close() {
		if (state_ == CLOSED) return -1;
		else if (state_ == MAPPED && unmap() == -1) return -1;
#ifdef _WIN32
		::CloseHandle(fh_);
#else
		if (fd_ != -1) ::close(fd_), fd_ = -1;
#endif
		state_ = CLOSED;
		if (temporary_) temporary_ = false, unlink();
		return 0;
	}
	char operator[](const size_t i) noexcept {
		return (static_cast<char*>(data_))[i];
	}
private:
	mmap_mode mode_;
	enum { CLOSED, UNMAPPED, MAPPED } state_;
	std::string filename_;
	size_t size_;
	void* data_ = 0;
	bool temporary_ = false;
#ifdef _WIN32
	HANDLE fh_;
	HANDLE mh_;
	inline void create_temp() {
		filename_ = temp_filename(), temporary_ = true;
	}
#else
	int fd_;
	int truncate() {
		if (state_==MAPPED) return err("cannot truncate mapped file");
		if (size_ && size_ != (size_t)file_size()) {
			if (::ftruncate(fd_, size_) == -1)
				return err(errno, "error truncate failed");
		}
		return size_;
	}
	inline void seek_beginning() {
		FILE *file = fdopen(fd_, "w");
		fseek(file, 0, SEEK_SET);
	}
	inline int fill() {
		int w = ::write(fd_, std::string(size_, 0).c_str(), size_);
		if (w == -1) err(errno, "memory_map: create (fill)");
		else seek_beginning();
		return w;
	}
	inline void create_temp() {
		temporary_ = true,
		fd_ = temp_fileno(),
		filename_ = filename(fd_);
	}
	void create() {
		if (filename_ == "") create_temp();
		else fd_ = ::open(filename_.c_str(), O_CREAT|O_RDWR, 0600);
    		if (fd_ == -1) { err(errno, "memory_map: create"); return; }
		if (fill() == -1) return;
	}
	bool file_exists() {
 		struct stat s;
    		return filename_=="" ? false : stat(filename_.c_str(),&s) == 0;
	}
#endif
	size_t file_size() {
#ifdef _WIN32
		LARGE_INTEGER file_size;
		if (::GetFileSizeEx(fh_, &file_size) != 0)
			return static_cast<size_t>(file_size.QuadPart);
#else
		struct stat s;
		if (::stat(filename_.c_str(), &s) != -1)
			return s.st_size;
#endif
		return err(errno, "cannot get file stats"), 0;
	}
	int err(int err_code, std::string message, int return_value = -1) {
		return err(message, return_value, err_code);
	}
	int err(std::string message, int return_value = -1, int err_code = 0) {
		error = true; std::stringstream ss; ss << "error: ";
		if (err_code) ss << '[' << err_code << "] ";
		ss << message << std::endl, error_message = ss.str();
		if (!silent) std::cerr << error_message;
		return return_value;
	}
};

template <typename T>
class memory_map_allocator {
public:
	typedef T value_type;
	memory_map_allocator() : fn(""), m(MMAP_NONE) { }
	memory_map_allocator(std::string fn, mmap_mode m = MMAP_WRITE) :
		fn(fn), m(m) { }
	memory_map_allocator(const memory_map_allocator<T>& a) :
		fn(a.fn), m(a.m) { }
	T* allocate(size_t n) {
		if (m == MMAP_NONE) return (T*) nommap.allocate(n);
		if (n == 0) return 0;
		mm = std::make_unique<memory_map>(fn, n*sizeof(T), m);
		return (T*) mm->data();
	}
	void deallocate(T* p, size_t n) {
		if (m == MMAP_NONE) return (void) nommap.deallocate(p, n);
		if (!p || !n) return;
		mm->close();
	}
	bool operator==(const memory_map_allocator& t) const {
		return fn == t.fn && m == t.m && mm == t.mm && nommap==t.nommap;
	}
	bool operator!=(const memory_map_allocator& t) const {
		return fn != t.fn || m != t.m || mm != t.mm || nommap!=t.nommap;
	}
private:
	std::string fn;
	mmap_mode m;
	std::unique_ptr<memory_map> mm;
	std::allocator<T> nommap;
};

#endif // __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
