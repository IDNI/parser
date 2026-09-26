// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__MEMORY_MAP_TMPL_H__
#define __IDNI__PARSER__UTILITY__MEMORY_MAP_TMPL_H__

#include <cerrno>
#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "memory_map.h"
#include "../parser_strings.h"

namespace idni::fs {

inline memory_map::memory_map() : mode_(MMAP_NONE),state_(CLOSED),filename_(),size_(0) {}
inline memory_map::memory_map(path filename, size_t s, mmap_mode m,
	bool do_open, bool do_map)
	: mode_(m), state_(CLOSED), filename_(std::move(filename)),
	  size_(s)
{
	if (mode_ == MMAP_NONE) return;
	if (do_open || do_map) if (!open().has_value()) return;
	if (do_map) map();
}
inline memory_map::~memory_map() { close(); }
inline size_t memory_map::size() const { return size_; }
inline const path& memory_map::file_name() const { return filename_; }
inline bool memory_map::has_error() const { return report_.has_error(); }
inline const diagnostics::report& memory_map::report() const { return report_; }
inline void* memory_map::data() {
	if (state_ != MAPPED) return 0;
	return map_.data;
}
inline result<bool> memory_map::open() {
	if (mode_  == MMAP_NONE)
		return err(diagnostics::code::invalid_state, "none mmap - cannot");
	if (state_ != CLOSED)
		return err(diagnostics::code::invalid_state, "file is already opened");
#ifdef _WIN32
	if (filename_.empty()) {
		auto r = create_temp();
		if (!r.has_value()) return r;
	}
	auto fh = fs::open(filename_, mode_);
	if (!fh.has_value()) {
		report_.append(fh.report());
		return cleanup_failed_open(diagnostics::fail<bool>(std::move(fh).report()));
	}
	fh_ = fh.value();
#else
	if (mode_ == MMAP_WRITE && (filename_.empty() || !fs::exists(filename_))) {
		auto r = create();
		if (!r.has_value()) return cleanup_failed_open(std::move(r));
	} else {
		auto fh = fs::open(filename_, mode_);
		if (!fh.has_value()) {
			report_.append(fh.report());
			return cleanup_failed_open(
				diagnostics::fail<bool>(std::move(fh).report()));
		}
		fh_ = fh.value();
		if (mode_ == MMAP_WRITE) {
			auto r = truncate();
			if (!r.has_value()) return cleanup_failed_open(std::move(r));
		}
	}
#endif
	state_ = UNMAPPED;
	if (!size_) { // autodetect map size
		auto sz = file_size();
		if (!sz.has_value())
			return cleanup_failed_open(
				diagnostics::fail<bool>(std::move(sz).report()));
		size_ = sz.value();
	}
	result<bool> r;
	return r.with_value(true);
}
inline result<bool> memory_map::cleanup_failed_open(result<bool> r) {
	if (fh_ != invalid_file_handle) {
		auto cr = fs::close(fh_);
		if (!cr.has_value()) {
			report_.append(cr.report());
			r.report().append(cr.report());
		}
		fh_ = invalid_file_handle;
	}
	if (temporary_) {
		auto ur = fs::unlink(filename_);
		if (!ur.has_value()) {
			report_.append(ur.report());
			r.report().append(ur.report());
		}
		temporary_ = false;
	}
	state_ = CLOSED;
	return r;
}
inline result<bool> memory_map::map() {
	if (state_ != UNMAPPED)
		return err(diagnostics::code::invalid_state,
			"file is not opened or already mapped");
	// An empty file has no bytes to map, so it is not an io error.
	if (!size_) {
		result<bool> rr;
		return rr.with_value(true);
	}
	auto r = fs::map(fh_, size_, mode_);
	if (!r.has_value()) {
		report_.append(r.report());
		return diagnostics::fail<bool>(std::move(r).report());
	}
	map_ = r.value();
	state_ = MAPPED;
	result<bool> rr;
	return rr.with_value(true);
}
inline result<bool> memory_map::sync() {
	auto r = fs::sync(fh_, map_);
	if (!r.has_value()) report_.append(r.report());
	return r;
}
inline result<bool> memory_map::unmap() {
	if (state_ != MAPPED)
		return warn("file not mapped, cannot be unmapped\n");
	// Unmaps even when sync fails, so a flush failure never leaves
	// the mapping and the descriptor held for the life of the object.
	auto sr = sync();
	auto ur = fs::unmap(map_);
	if (ur.has_value()) state_ = UNMAPPED;
	else report_.append(ur.report());
	if (!sr.has_value()) {
		if (!ur.has_value()) sr.report().append(ur.report());
		return sr;
	}
	return ur;
}
inline result<bool> memory_map::unlink() {
	if (state_ != CLOSED)
		return err(diagnostics::code::invalid_state,
			"file is not closed. it cannot be deleted\n");
	result<bool> r;
	auto ok = r.merge_take(fs::unlink(filename_));
	report_.append(r.report());
	if (!ok) return r;
	return r.with_value(*ok);
}
inline result<bool> memory_map::close() {
	if (state_ == CLOSED)
		return warn("file is already closed");
	if (state_ == MAPPED) {
		auto r = unmap();
		if (!r.has_value()) return r;
	}
	if (fh_ == invalid_file_handle) {
		state_ = CLOSED;
	} else {
		// POSIX close() releases the descriptor number even when it
		// reports a failure, so fh_ and state_ move on regardless: a
		// second close must never run on the same descriptor.
		auto r = fs::close(fh_);
		fh_ = invalid_file_handle;
		state_ = CLOSED;
		if (!r.has_value()) {
			report_.append(r.report());
			return r;
		}
	}
	if (temporary_) { temporary_ = false; unlink(); }
	result<bool> rr;
	return rr.with_value(true);
}
inline char memory_map::operator[](const size_t i) noexcept {
	return (static_cast<char*>(map_.data))[i];
}
inline result<bool> memory_map::create_temp() {
	auto r = temp_filename("mmap");
	if (!r.has_value()) {
		report_.append(r.report());
		return diagnostics::fail<bool>(std::move(r).report());
	}
	filename_ = std::move(r.value());
	temporary_ = true;
	result<bool> rr;
	return rr.with_value(true);
}
#ifndef _WIN32
inline result<bool> memory_map::truncate() {
	if (state_==MAPPED)
		return err(diagnostics::code::invalid_state,
			"cannot truncate mapped file");
	if (size_) {
		auto sz = file_size();
		if (!sz.has_value())
			return diagnostics::fail<bool>(std::move(sz).report());
		if (size_ != sz.value()) {
			auto r = fs::resize(fh_, size_);
			if (!r.has_value()) {
				report_.append(r.report());
				return r;
			}
		}
	}
	result<bool> r;
	return r.with_value(true);
}
inline result<bool> memory_map::seek_beginning() {
	using label = idni::parser_strings::label;
	if (::lseek(fh_, 0, SEEK_SET) == -1)
		return err(diagnostics::code::io_error,
			"failed to seek to the beginning of the file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	result<bool> r;
	return r.with_value(true);
}
inline result<bool> memory_map::fill() {
	auto r = fs::resize(fh_, size_);
	if (!r.has_value()) {
		report_.append(r.report());
		return r;
	}
	return seek_beginning();
}
inline result<bool> memory_map::create() {
	if (filename_.empty()) {
		auto r = create_temp();
		if (!r.has_value()) return r;
	}
	// mkstemp already created the file; fs::create below finds it.
	auto fh = fs::create(filename_, mode_);
	if (!fh.has_value()) {
		report_.append(fh.report());
		return diagnostics::fail<bool>(std::move(fh).report());
	}
	fh_ = fh.value();
	return fill();
}
#endif
inline result<size_t> memory_map::file_size() {
	auto r = fs::size(fh_);
	if (!r.has_value()) report_.append(r.report());
	return r;
}
inline result<bool> memory_map::err(diagnostics::code c,
	std::string_view message, std::initializer_list<diagnostics::attr_in> extra)
{
	result<bool> r;
	r.error(c, message, extra);
	report_.append(r.report());
	return r;
}
inline result<bool> memory_map::warn(std::string_view message,
	std::initializer_list<diagnostics::attr_in> extra)
{
	result<bool> r;
	r.warning(message, extra);
	report_.append(r.report());
	return r.with_value(false);
}

template <typename T>
memory_map_allocator<T>::memory_map_allocator() : fn(), m(MMAP_NONE) { }
template <typename T>
memory_map_allocator<T>::memory_map_allocator(path fn, mmap_mode m) :
	fn(std::move(fn)), m(m) { }
template <typename T>
memory_map_allocator<T>::memory_map_allocator(const memory_map_allocator<T>& a) :
	fn(a.fn), m(a.m) { }
template <typename T>
T* memory_map_allocator<T>::allocate(size_t n) {
	if (m == MMAP_NONE) return (T*) nommap.allocate(n);
	if (n == 0) return 0;
	mm = std::make_unique<memory_map>(fn, n*sizeof(T), m);
	return (T*) mm->data();
}
template <typename T>
void memory_map_allocator<T>::deallocate(T* p, size_t n) {
	if (m == MMAP_NONE) return (void) nommap.deallocate(p, n);
	if (!p || !n || !mm) return;
	mm->close();
}
template <typename T>
bool memory_map_allocator<T>::operator==(const memory_map_allocator& t) const {
	return fn == t.fn && m == t.m;
}
template <typename T>
bool memory_map_allocator<T>::operator!=(const memory_map_allocator& t) const {
	return fn != t.fn || m != t.m;
}

} // namespace idni::fs

#endif // __IDNI__PARSER__UTILITY__MEMORY_MAP_TMPL_H__
