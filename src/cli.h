// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI_CLI_H__
#define __IDNI_CLI_H__

#include <variant>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "defs.h"

namespace idni {

struct cli {

	struct option {
		typedef std::variant<std::string, bool, int> value;
		option();
		// create an option --name (and -short_name)
		// with a default value denoting also option value's type
		option(const std::string& name, char short_, value dflt_value);
		// add a description to the option
		option& set_description(const std::string& d);
		// getters
		const std::string& description() const;
		const std::string& name() const;
		char short_name() const;
		// returns true if name matches string n
		bool matches(const std::string& n) const;
		// returns true if short_name matches char c
		bool matches(char c) const;
		// checking for a type of option's value
		bool is_bool() const;
		bool is_string() const;
		bool is_int() const;
		// sets the value of the option
		void set(value v);
		int set(char* argv);
		// get_value<string / bool / int> returns the value of the option
		template <typename T> T get() const;
		const value& get() const;
		// prints the option info
		std::ostream& print(std::ostream& os) const;
	private:
		std::string name_  = "";
		char        short_ = '\0';
		std::string desc_  = "";
		value       value_;
	};
	typedef std::map<std::string, struct option> options;

	struct command {
		friend cli;
		command() {};
		command(const std::string& name, const std::string& desc = "",
			options opts = {});
		command& set_description(const std::string& d);
		command& add_option(const option& o);

		bool ok() const;

		const std::string& name() const;
		const std::string& description() const;
		bool has(const std::string& n) const;
		option& operator[](const std::string& n);
		option& operator[](char c);

		template <typename T>
		T get(const std::string& n) const;

		std::ostream& help(std::ostream& os) const;
		std::ostream& print(std::ostream& os) const;
	private:
		std::string  name_ = "";
		std::string  desc_ = "";
		options opts_ = {};
		bool ok_ = false;
	} cmd_;
	typedef std::map<std::string, struct command> commands;

	// constructors
	cli(const std::string& name, std::vector<std::string> args = {},
		commands cmds = {}, std::string dflt_cmd = "",
		options opts = {});
	cli(const std::string& name, int argc, char** argv,
		commands cmds = {}, std::string dflt_cmd = "",
		options opts = {});
	// setters
	cli& set_args(const std::vector<std::string>& args);
	cli& set_commands(const commands& cmds);
	cli& set_options(const options& opts);
	cli& set_default_command(const std::string& cmd_name);
	cli& set_default_command_when_files(const std::string& cmd_name);
	cli& set_description(const std::string& desc);
	cli& set_help_header(const std::string& header);
	cli& set_output_stream(std::ostream& os);
	cli& set_error_stream(std::ostream& os);

	// get options with populated values from args
	options get_processed_options();
	// get command with populated values from args
	command get_processed_command();
	// get files
	const std::vector<std::string>& get_files() const;

	// helpers for printing info
	std::ostream& info(std::ostream& os, const std::string& msg,
		size_t indent = 0) const;
	std::ostream& info(const std::string& msg, size_t indent = 0) const;

	// helpers for printing error message and help if print_help is true
	int error(std::ostream& os, const std::string& msg,
		bool print_help = false) const;
	int error(const std::string& msg, bool print_help = false) const;

	// prints help
	std::ostream& help(std::ostream& os, command cmd = {}) const;
	std::ostream& help(command cmd = {}) const;

	// processes arguments and populates options and commands
	// returns 0 if ok, 1 if error, 2 if not processed
	// it can be called by user or it is called automatically
	// when called get_processed_options() or get_processed_command()
	// if it wasn't called before
	int process_args();

	// returns the status of the last process_args call
	int status() const;

private:
	std::string name_ = "";
	std::string desc_ = "";
	std::vector<std::string> args_{};
	bool processed_ = false;
	commands cmds_{}, cmds_default_{};
	std::string dflt_cmd_ = "";
	std::string dflt_cmd_when_files_ = "";
	options opts_{}, opts_default_{};
	std::string help_header_ = "";
	int status_ = 2; // 0 ok, 1 error, 2 not processed
	std::vector<std::string> files_{};

	std::ostream* out_ = &std::cout;
	std::ostream* err_ = &std::cerr;

	int process_arg(int& arg, bool& has_cmd, options& opts);
};

template <typename T> T cli::option::get() const {
	return std::get<T>(value_);
}
template <typename T> T cli::command::get(const std::string& n) const {
	return opts_.at(n).get<T>();
}


} // namespace idni

#endif // __IDNI_CLI_H__