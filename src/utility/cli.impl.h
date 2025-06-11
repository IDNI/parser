// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include <filesystem>
#include <fstream>
#include "cli.h"

#define PBOOL(bval) (( bval ) ? "true" : "false")

namespace idni {

// local helper functions
//
inline int to_int(const std::string& s, bool& ok) {
	std::stringstream is(s);
	int n = 0;
	if (!(is >> n)) return ok = false, 0;
	return ok = true, n;
}

inline bool is_int(const std::string& s) {
	bool ok;
	to_int(s, ok);
	return ok;
}

inline bool is_opt_prefix(const std::string& opt) {
	return opt.size() >= 2 && opt[0] == '-' && !is_int(opt);
}

inline bool opt_prefix(std::string& opt, bool& isshort) {
	isshort = false;
	if (is_int(opt)) return false;
	if (opt.size() < 2 || opt[0] != '-') return false;
	if (opt[1] != '-') isshort = true, opt = opt.substr(1);
	else opt = opt.substr(2);
	return true;
}

// checks case insensitively if the std::string represents a true value
inline bool is_true_value(std::string s) {
	std::vector<std::string> trues
		= { "1", "t", "on", "yes", "true" };
	transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return tolower(c); });
	return find(trues.begin(), trues.end(), s) != trues.end();
}

inline std::string long_for(char c, const cli::options& opts) {
	for (auto& opt : opts) if (opt.second.matches(c))
		return opt.second.name();
	return "";
}

inline std::ostream& info_new_line(std::ostream& os, size_t indent) {
	os << "#";
	for (int i = indent; i--; ) os << "\t";
	os << " ";
	return os;
};

inline std::ostream& print_options(std::ostream& os, const cli::options& opts) {
	for (auto& [_, opt]: opts)
		os << "\t--" << opt.name() << ((opt.name().length() < 6)
						       ? "\t\t-"
						       : "\t-")
		<< opt.short_name() << "\t" << opt.description() << "\n";
	return os;
}

inline std::vector<std::string> argv2args(int argc, char** argv) {
	std::vector<std::string> args;
	for (int i = 0; i != argc; ++i) args.push_back(argv[i]);
	return args;
}

// cli::option
//

inline cli::option::option() {}
inline cli::option::option(const std::string& name, char short_, value dflt_value)
	: name_(name), short_(short_), value_(dflt_value) {}

inline cli::option& cli::option::set_description(const std::string& d) {
	return desc_ = d, *this;
}

inline const std::string& cli::option::description()   const { return desc_; }
inline const std::string& cli::option::name()          const { return name_; }
inline char cli::option::short_name()             const { return short_; }
inline bool cli::option::matches(const std::string& n) const { return name_  == n; }
inline bool cli::option::matches(char c)          const { return short_ == c; }
inline bool cli::option::is_bool() const {
	return holds_alternative<bool>(value_); }
inline bool cli::option::is_string() const {
	return holds_alternative<std::string>(value_); }
inline bool cli::option::is_int() const {
	return holds_alternative<int>(value_); }
inline void cli::option::set(value v) { value_ = v; }

inline int cli::option::set(char* argv) {
	if (is_bool()) set(is_true_value(argv));
	else if (is_string()) set(std::string{ argv });
	else if (is_int()) {
		std::stringstream is(argv);
		int n = 0;
		if (!(is >> n)) return 1;
		set(n);
	}
	return 0;
}
inline const cli::option::value& cli::option::get() const { return value_; }

inline std::ostream& cli::option::print(std::ostream& os) const {
	os << name() << ": ";
	if (is_bool()) os << PBOOL(get<bool>());
	else if (is_int()) os << get<int>();
	else os << '"' << get<std::string>() << '"';
	return os;
}

// cli::command
//

inline cli::command::command(const std::string& name, const std::string& desc, options opts)
	: name_(name), desc_(desc), opts_(opts) {}
inline cli::command& cli::command::set_description(const std::string& desc) {
	return desc_ = desc, *this;
}
inline cli::command& cli::command::add_option(const option& o) {
	return opts_.insert({ o.name(), o }),  *this;
}

inline bool cli::command::ok() const { return ok_; }

inline const std::string& cli::command::name() const { return name_; }
inline const std::string& cli::command::description() const { return desc_; }
inline bool cli::command::has(const std::string& n) const {
	return opts_.find(n) != opts_.end(); }
inline cli::option& cli::command::operator[](const std::string& n) { return opts_[n]; }
inline cli::option& cli::command::operator[](char c) {
	return opts_[long_for(c, opts_)]; }

inline std::ostream& cli::command::help(std::ostream& os) const {
	os << name();
	if (opts_.size()) os << " [ options ]";
	os << "\n";
	if (description().size()) os << description() << "\n\n";
	return print_options(os << "Command options:\n", opts_);
}

inline std::ostream& cli::command::print(std::ostream& os) const {
	for (auto& [_, opt] : opts_) opt.print(os << "\t") << "\n";
	return os;
}

inline cli::cli(const std::string& name, std::vector<std::string> args, commands cmds,
	std::string dflt_cmd, options opts) :
		name_(name), args_(args), cmds_(cmds),
		cmds_default_(cmds_), dflt_cmd_(dflt_cmd),
		opts_(opts), opts_default_(opts_) {}

inline cli::cli(const std::string& name, int argc, char** argv, commands cmds,
	const std::string dflt_cmd, options opts) :
		cli(name, argv2args(argc, argv), cmds, dflt_cmd, opts) {}

inline cli& cli::set_args(const std::vector<std::string>& args) {
	return args_ = args, processed_ = false, *this; }
inline cli& cli::set_commands(const commands& cmds) {
	return cmds_default_ = cmds_ = cmds,
		status_ = 2, processed_= false, *this; }
inline cli& cli::set_options(const options& opts) {
	return opts_default_ = opts_ = opts,
		status_ = 2, processed_ = false, *this; }
inline cli& cli::set_default_command(const std::string& cmd_name) {
	return dflt_cmd_ = cmd_name,
		status_ = 2, processed_ = false, *this; }
inline cli& cli::set_default_command_when_files(const std::string& cmd_name) {
	return dflt_cmd_when_files_ = cmd_name,
		status_ = 2, processed_ = false, *this; }
inline cli& cli::set_description(const std::string& desc) {
	return desc_ = desc, status_ = 2, processed_ = false, *this; }
inline cli& cli::set_help_header(const std::string& header) {
	return help_header_ = header, status_ = 2, processed_ = false, *this; }
inline cli& cli::set_output_stream(std::ostream& os) { return out_ = &os, *this; }
inline cli& cli::set_error_stream(std::ostream& os) { return err_ = &os, *this; }

inline cli::options cli::get_processed_options() {
	if (!processed_) status_ = process_args();
	return opts_;
}
inline cli::command cli::get_processed_command() {
	if (!processed_) status_ = process_args();
	return cmd_;
}
inline const std::vector<std::string>& cli::get_files() const { return files_; }

inline std::ostream& cli::info(std::ostream& os, const std::string& msg, size_t indent) const {
	info_new_line(os, indent);
	for (size_t i = 0; i != msg.size(); ++i)
		if ((os << msg[i], msg.at(i)) == '\n' && i != msg.size() - 1)
			info_new_line(os, indent);
	return os;
}
inline std::ostream& cli::info(const std::string& msg, size_t indent) const {
	return info(*out_, msg, indent);
}

inline int cli::error(std::ostream& os, const std::string& msg, bool print_help) const {
	os << msg << "\n";
	if (print_help) help(os << "\n");
	return 1;
}

inline int cli::error(const std::string& msg, bool print_help) const {
	return error(*err_, msg, print_help);
}

inline std::ostream& cli::help(command cmd) const {
	return help(*out_, cmd);
}

inline std::ostream& cli::help(std::ostream& os, command cmd) const {
	auto print_header = [this](std::ostream& os) -> std::ostream& {
		return help_header_.size() ? os << help_header_ : os << name_;
	};
	if (cmd.ok()) return cmds_default_.at(cmd.name())
					.help(print_header(os) << " ") << "\n";
	print_header(os);
	if (opts_.size()) os << " [ options ] ";
	if (cmds_.size()) {
		if (dflt_cmd_.size()) os << "[ ";
		os << "<command> [ <command options> ] ";
		if (dflt_cmd_.size()) os << "]";
	}
	os << "\n";
	if (desc_.size()) os << desc_ << "\n";
	if (opts_.size()) print_options(os << "\nOptions:\n", opts_);
	if (cmds_.size()) {
		os << "Commands:\n";
		for (auto it : cmds_) os << "\t" << it.first
				<< "\t\t\t" << it.second.description() << "\n";
	}
	return os << "\n";
}

// processes an option argument and returns 0 if successful
// if it is a known error returns 1
// if it is a command, returns 2
// if it is invalid option and we dont have a command returns 3
// if it is invalid option and we have a command returns 1
inline int cli::process_arg(int& arg, bool& has_cmd, options& opts) {
	//DBG(cout << "process_arg: " << arg << " args[arg]: " << args_[arg] << "\n";)
	cli::option* cur = 0;
	std::string opt(args_[arg]);
	bool isshort;
	// if its command return 2 as we are done with CLI options
	if (cmds_.find(opt) != cmds_.end()) return has_cmd = true, 2;
	if (!opt_prefix(opt, isshort)) {
		if (opt == "-" || std::filesystem::exists(opt))
			return files_.push_back(opt), ++arg, 0;
		else return error("Invalid command or file not exists: "
								+ opt, true);
	}
	//DBG(cout << "opt: " << opt << " isshort: " << PBOOL(isshort) << endl;)
	if (isshort) for (size_t o = 0; o != opt.size(); ++o) {
		std::string n = long_for(opt[o], opts);
		//DBG(cout << "long_for " << opt[o] << ": " << n << "\n";)
		if (n.size() == 0 || opts.find(n) == opts.end()) {
			if (!has_cmd) return 3;
			std::stringstream ss;
			ss << "Invalid option: -" << opt[o];
			return error(ss.str(), true);
		}
		cur = &opts[n];
		//DBG(cout << "cur->name() " << cur->name() << "\n";);
		if (cur->is_bool()) cur->set(true);
		else if (opt.size() > o + 1) {
			std::stringstream ss;
			ss << "Missing argument for option: -"
				<< opt[o];
			return error(ss.str(), true);
		}
	}
	else { // long
		if (opts.find(opt) == opts.end()) {
			if (!has_cmd) return 3;
			std::stringstream ss;
			ss << "Invalid option: " << opt;
			return error(ss.str(), true);
		}
		cur = &opts[opt];
		if (cur->is_bool()) cur->set(true);
	}
	++arg;
	// option argument if any
//#ifdef DEBUG
//	cout << "arg: " << arg << " argc: " << argc;
//	if (arg < argc) cout << " args[arg]: " << args_[arg];
//	cout << "\n";
//#endif // DEBUG
	if (arg >= int(args_.size()) || is_opt_prefix(args_[arg])) { // no option argument
		//DBG(cout << "no option argument?\n";)
		if (!cur->is_bool()) return error(
			"Missing argument for option: --"+cur->name());
		return 0;
	} else {
		std::string v = args_[arg];
		//DBG(cout << "v: " << v << "\n";)
		if (cur->is_string()) cur->set(v);
		else if (cur->is_bool())
			cur->set(is_true_value(v));
		else if (cur->is_int()) {
			std::stringstream is(v);
			int n = 0;
			if (!(is >> n)) return
				error("Option --" + cur->name()
					+ " argument is not a "
					"number: " + v);
			cur->set(n);
		}
		//DBG(if (cur->is_string()) cout
		//	<< "cur->get_string(): " << cur->get<std::string>() << "\n";)
	}
	++arg;
	return 0;
}

// processes CLI args provided and populates cmd_ and options_ with values
inline int cli::process_args() {
	int argc(args_.size());

//#ifdef DEBUG
//	cout << "== DEBUG ========================================"
//		"==========================\n";
//	cout << "argc: " << argc << "\n";
//	cout << "argv:";
//	for (int i = 0; i != argc; ++i)
//		cout << "\t" << i << ": `" << args_[i] << "`\n";
//	cout << "= /DEBUG ========================================"
//		"==========================\n\n";
//#endif

	bool has_cmd = false;
	status_ = 0;
	// get CLI options
	int arg = 1;
	for ( ; arg < argc; )
		if ((status_ = process_arg(arg, has_cmd, opts_)) != 0) break;
	//DBG(cout << "status: " << status_ << endl;)
	if (status_ == 1) return status_;
	if (status_ == 3 && (dflt_cmd_ == ""
			|| (files_.size() && dflt_cmd_when_files_ == "")))
		return 1;

	// get command
	std::string cmdarg = status_ == 2 ? args_[arg++]
		: (status_ == 3 || status_ == 0)
			? (files_.size() ? dflt_cmd_when_files_ : dflt_cmd_)
			: "";
	status_ = 0;
	//DBG(cout << "cmdarg: " << cmdarg << "\n";)

	// if command, get its options
	if (cmdarg.size()) {
		if (cmds_.find(cmdarg) == cmds_.end()) return status_
				= error("Unknown command: " + cmdarg, true);
		cmd_ = cmds_[cmdarg];
		has_cmd = true;
		// get command's options
		for ( ; arg < argc; ) if ((status_
			= process_arg(arg, has_cmd, cmd_.opts_)) != 0) break;
		//DBG(cout << "status: " << status_ << endl;)
		if (status_) return status_;
		//DBG(cmd_.print(cout << "command:\n") << endl;)
		cmd_.ok_ = true;
	}

	processed_ = true;
	return 0;
}

inline int cli::status() const { return status_; }

} // namespace idni