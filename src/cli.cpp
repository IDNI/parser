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
#include "cli.h"

#define PBOOL(bval) (( bval ) ? "true" : "false")

using namespace std;

namespace idni {

// local helper functions
//
int to_int(const string& s, bool& ok) {
	stringstream is(s);
	int n = 0;
	if (!(is >> n)) return ok = false, 0;
	return ok = true, n;
}

bool is_int(const string& s) {
	bool ok;
	to_int(s, ok);
	return ok;
}

bool is_opt_prefix(const string& opt) {
	return opt.size() >= 2 && opt[0] == '-' && !is_int(opt);
}

bool opt_prefix(string& opt, bool& isshort) {
	isshort = false;
	if (is_int(opt)) return false;
	if (opt.size() < 2 || opt[0] != '-') return false;
	if (opt[1] != '-') isshort = true, opt = opt.substr(1);
	else opt = opt.substr(2);
	return true;
}

// checks case insensitively if the string represents a true value
bool is_true_value(string s) {
	vector<string> trues
		= { "1", "t", "on", "yes", "true" };
	transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return tolower(c); });
	return find(trues.begin(), trues.end(), s) != trues.end();
}

string long_for(char c, const cli::options& opts) {
	for (auto& opt : opts) if (opt.second.matches(c))
		return opt.second.name();
	return "";
}

ostream& info_new_line(ostream& os, size_t indent) {
	os << "#";
	for (int i = indent; i--; ) os << "\t";
	os << " ";
	return os;
};

ostream& print_options(ostream& os, const cli::options& opts) {
	for (auto& [_, opt] : opts) os << "\t--" << opt.name() << "\t-"
		<< opt.short_name() << "\t" << opt.description() << "\n";
	return os;
}

vector<string> argv2args(int argc, char** argv) {
	vector<string> args;
	for (int i = 0; i != argc; ++i) args.push_back(argv[i]);
	return args;
}

// cli::option
//

cli::option::option() {}
cli::option::option(const string& name, char short_, value dflt_value)
	: name_(name), short_(short_), value_(dflt_value) {}

cli::option& cli::option::set_description(const string& d) {
	return desc_ = d, *this;
}

const string& cli::option::description()   const { return desc_; }
const string& cli::option::name()          const { return name_; }
char cli::option::short_name()             const { return short_; }
bool cli::option::matches(const string& n) const { return name_  == n; }
bool cli::option::matches(char c)          const { return short_ == c; }
bool cli::option::is_bool() const {
	return holds_alternative<bool>(value_); }
bool cli::option::is_string() const {
	return holds_alternative<string>(value_); }
bool cli::option::is_int() const {
	return holds_alternative<int>(value_); }
void cli::option::set(value v) { value_ = v; }

int cli::option::set(char* argv) {
	if (is_bool()) set(is_true_value(argv));
	else if (is_string()) set(string{ argv });
	else if (is_int()) {
		stringstream is(argv);
		int n = 0;
		if (!(is >> n)) return 1;
		set(n);
	}
	return 0;
}
const cli::option::value& cli::option::get() const { return value_; }

ostream& cli::option::print(ostream& os) const {
	os << name() << ": ";
	if (is_bool()) os << PBOOL(get<bool>());
	else if (is_int()) os << get<int>();
	else os << '"' << get<string>() << '"';
	return os;
}

// cli::command
//

cli::command::command(const string& name, const string& desc, options opts)
	: name_(name), desc_(desc), opts_(opts) {}
cli::command& cli::command::set_description(const string& desc) {
	return desc_ = desc, *this;
}
cli::command& cli::command::add_option(const option& o) {
	return opts_.insert({ o.name(), o }),  *this;
}

bool cli::command::ok() const { return ok_; }

const string& cli::command::name() const { return name_; }
const string& cli::command::description() const { return desc_; }
bool cli::command::has(const string& n) const {
	return opts_.find(n) != opts_.end(); }
cli::option& cli::command::operator[](const string& n) { return opts_[n]; }
cli::option& cli::command::operator[](char c) {
	return opts_[long_for(c, opts_)]; }

ostream& cli::command::help(ostream& os) const {
	os << name();
	if (opts_.size()) os << " [ options ]";
	os << "\n" << description() << "\n\n";
	return print_options(os << "Command options:\n", opts_);
}

ostream& cli::command::print(ostream& os) const {
	for (auto& [_, opt] : opts_) opt.print(os << "\t") << "\n";
	return os;
}

cli::cli(const string& name, vector<string> args, commands cmds,
	string dflt_cmd, options opts) :
		name_(name), args_(args), cmds_(cmds),
		cmds_default_(cmds_), dflt_cmd_(dflt_cmd),
		opts_(opts), opts_default_(opts_) {}

cli::cli(const string& name, int argc, char** argv, commands cmds,
	const string dflt_cmd, options opts) :
		cli(name, argv2args(argc, argv), cmds, dflt_cmd, opts) {}

cli& cli::set_args(const vector<string>& args) {
	return args_ = args, processed_ = false, *this; }
cli& cli::set_commands(const commands& cmds) {
	return cmds_default_ = cmds_ = cmds,
		status_ = 2, processed_= false, *this; }
cli& cli::set_options(const options& opts) {
	return opts_default_ = opts_ = opts,
		status_ = 2, processed_ = false, *this; }
cli& cli::set_default_command(const string& cmd_name) {
	return dflt_cmd_ = cmd_name,
		status_ = 2, processed_ = false, *this; }
cli& cli::set_description(const string& desc) {
	return desc_ = desc, status_ = 2, processed_ = false, *this; }
cli& cli::set_help_header(const string& header) {
	return help_header_ = header, status_ = 2, processed_ = false, *this; }
cli& cli::set_output_stream(ostream& os) { return out_ = &os, *this; }
cli& cli::set_error_stream(ostream& os) { return err_ = &os, *this; }

cli::options cli::get_processed_options() {
	if (!processed_) status_ = process_args();
	return opts_;
}
cli::command cli::get_processed_command() {
	if (!processed_) status_ = process_args();
	return cmd_;
}

ostream& cli::info(ostream& os, const string& msg, size_t indent) const {
	info_new_line(os, indent);
	for (size_t i = 0; i != msg.size(); ++i)
		if ((os << msg[i], msg.at(i)) == '\n' && i != msg.size() - 1)
			info_new_line(os, indent);
	return os;
}
ostream& cli::info(const string& msg, size_t indent) const {
	return info(*out_, msg, indent);
}

int cli::error(ostream& os, const string& msg, bool print_help) const {
	os << msg << "\n";
	if (print_help) help(os << "\n");
	return 1;
}

int cli::error(const string& msg, bool print_help) const {
	return error(*err_, msg, print_help);
}

ostream& cli::help(command cmd) const {
	return help(*out_, cmd);
}

ostream& cli::help(ostream& os, command cmd) const {
	auto print_header = [this](ostream& os) -> ostream& {
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
				<< "\t\t" << it.second.description() << "\n";
	}
	return os << "\n";
}

// processes an option argument and returns 0 if successful
// if it is a known error returns 1
// if it is a command, returns 2
// if it is invalid option and we dont have a command returns 3
// if it is invalid option and we have a command returns 1
int cli::process_arg(int& arg, bool& has_cmd, options& opts) {
	//DBG(cout << "process_arg: " << arg << " args[arg]: " << args_[arg] << "\n";)
	cli::option* cur = 0;
	string opt(args_[arg]);
	bool isshort;
	// if its command return 2 as we are done with CLI options
	if (cmds_.find(opt) != cmds_.end()) return has_cmd = true, 2;
	if (!opt_prefix(opt, isshort))
		return error("Invalid command: " + opt, true);
	//DBG(cout << "opt: " << opt << " isshort: " << PBOOL(isshort) << endl;)
	if (isshort) for (size_t o = 0; o != opt.size(); ++o) {
		string n = long_for(opt[o], opts);
		//DBG(cout << "long_for " << opt[o] << ": " << n << "\n";)
		if (n.size() == 0 || opts.find(n) == opts.end()) {
			if (!has_cmd) return 3;
			stringstream ss;
			ss << "Invalid option: -" << opt[o];
			return error(ss.str(), true);
		}
		cur = &opts[n];
		//DBG(cout << "cur->name() " << cur->name() << "\n";);
		if (cur->is_bool()) cur->set(true);
		else if (opt.size() > o + 1) {
			stringstream ss;
			ss << "Missing argument for option: -"
				<< opt[o];
			return error(ss.str(), true);
		}
	}
	else { // long
		if (opts.find(opt) == opts.end()) {
			if (!has_cmd) return 3;
			stringstream ss;
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
		string v = args_[arg];
		//DBG(cout << "v: " << v << "\n";)
		if (cur->is_string()) cur->set(v);
		else if (cur->is_bool())
			cur->set(is_true_value(v));
		else if (cur->is_int()) {
			stringstream is(v);
			int n = 0;
			if (!(is >> n)) return
				error("Option --" + cur->name()
					+ " argument is not a "
					"number: " + v);
			cur->set(n);
		}
		//DBG(if (cur->is_string()) cout
		//	<< "cur->get_string(): " << cur->get<string>() << "\n";)
	}
	++arg;
	return 0;
}

// processes CLI args provided and populates cmd_ and options_ with values
int cli::process_args() {
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
	if (status_ == 3 && dflt_cmd_ == "")
		return error("Invalid option: " + args_[arg], true);

	// get command
	string cmdarg = status_ == 2 ? args_[arg++]
		: (status_ == 3 || status_ == 0) ? dflt_cmd_
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
		if (status_) {
			stringstream ss;
			ss << "Invalid option: " << args_[arg];
			return error(ss.str(), true);
		}
		//DBG(cmd_.print(cout << "command:\n") << endl;)
		cmd_.ok_ = true;
	}

	processed_ = true;
	return 0;
}

int cli::status() const { return status_; }

} // namespace idni