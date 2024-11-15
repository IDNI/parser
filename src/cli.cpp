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
#include <filesystem>
#include <fstream>
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
	for (auto& [_, opt]: opts)
		os << "\t--" << opt.name() << ((opt.name().length() < 5)
						       ? "\t\t-"
						       : "\t-")
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
cli& cli::set_default_command_when_files(const string& cmd_name) {
	return dflt_cmd_when_files_ = cmd_name,
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
const vector<string>& cli::get_files() const { return files_; }

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
				<< "\t\t\t" << it.second.description() << "\n";
	}
	return os << "\n";
}

void cli::version() const {
	*out_ << "Tau version: " << GIT_DESCRIBED << "\n";
}

void cli::license() const {
	const char *license = R"(
License Agreement for the Tau Language Framework
The Tau Language Framework (the “Product,” including all associated files) is protected by IDNI AG legal rights, including intellectual property rights, which may be covered by patents and/or copyrights.  IDNI AG retains all of its intellectual property rights, but grants permission to use the Product for free (1) when building applications, smart contracts, and/or blockchain specifications solely for deployment and execution over the Tau Net blockchain (including its testnets) deployed by IDNI AG, regardless of whether the usage is for commercial purposes or not, (2) as part of running the aforementioned Tau Net blockchain (including its testnets) client software, and (3) in the following specific enumerated instances (the “enumerated instances”): educational use, research purposes, and non-commercial personal use.  The enumerated instances do not include any instance where data or software incorporating or embodying IDNI AG intellectual property associated with the Product is:
1. redistributed to third parties;
2. used to build  software or hardware of any kind for profit or commercial usage;
3. used outside of the context of studying the Tau language and associated mathematical theories in the context of education or research.
Usage, installation, and/or download of the Product constitutes acceptance of the terms of this license.  The terms of this license are subject to modification by IDNI AG at its sole discretion, and continued usage of the Product after modification of the license terms constitute acceptance of the modified terms.  However if a version of the Product you obtained allows a free use according to the license it is shipped with, you may continue using that version for free in accordance with that license.
To request a license for commercial use, contact IDNI AG.
Any person agreeing to this license on behalf of an entity represents having authority to accept the terms of this license agreement on behalf of the entity.


§ 1 The Tau Language Framework.  The Product consists of a set of materials published by IDNI AG as part of the Tau Language Framework, which may include executable files in machine code, data files, source code, object code, link libraries, utility programs, project files, scripts related to the software listed above, and related documentation associated with the Tau Language Framework.


§ 2 Grant of license.  For usage of the Product that is permitted for free, IDNI AG grants you a personal, non-exclusive, non-transferable, limited license without fees to install, execute, and use the Product.  “Educational Use” is any use by teachers or students.
The Product is licensed, not sold.
IDNI AG reserves all rights not expressly granted in this license.


§ 3 Termination. The Agreement is effective on the date you receive the Product and remains effective until terminated. Your rights under this Agreement terminate immediately without notice from IDNI AG if you materially breach any terms of this agreement or take any action against IDNI AG’s rights to the Product.  IDNI AG may terminate this Agreement immediately should any part of the Product become the subject of a claim of intellectual property infringement or trade secret misappropriation. Upon termination, you will cease use of and destroy all copies of the Product under your control and confirm compliance in writing to IDNI AG. Neither termination of this Agreement nor any deletion or removal of the Product shall limit any obligations you may have to IDNI AG, or any rights and/or remedies that IDNI AG may have with respect to any past or future infringing use of the Product (including, but not limited to, any use of the Product outside the scope of the license provided in this Agreement).  Sections 2-8 survive termination of this Agreement.


§ 4 Disclaimer of Warranty.  IDNI AG provides the Product “as is,” without warranty of any kind, either express or implied.  IDNI AG specifically disclaims any implied warranties of merchantability, fitness for a particular purpose, title, and non-infringement.  You assume the entire risk as to the quality and performance of the Product.  If the Product is defective in any way, you assume the cost of all necessary servicing, repair, or correction.


§ 5 Limitation of Liability. In no event will IDNI AG or any personnel associated with IDNI AG be liable for any lost revenue, profit, data, or data use, or for special, indirect, consequential, incidental, or punitive damages, regardless of the cause of loss or theory of liability, arising out of or related to the use of or inability to use the Product.  In no event will IDNI AG or any personnel associated with IDNI AG be liable for third party claims based on your use of the Product.  You agree to indemnify IDNI AG and personnel associated with IDNI AG for any claims brought by third parties arising from use of the Product.  In no event will IDNI AG’s liability to you, whether in contract, tort (including negligence), or otherwise, exceed the amount paid by you for the Product under this Agreement.


§ 6 Separately Licensed Third Party Technology.  The Product may contain or require the use of third party technology that is provided with the Product.  IDNI AG may provide certain notices to you in the Product’s documentation, readmes or notice files in connection with such third party technology.  Third party technology will be licensed to you either under the terms of this Agreement or, if specified in the documentation, readmes, or notice files, under Separate Terms.  Your rights to use Separately Licensed Third Party Technology under Separate Terms are not restricted in any way by this Agreement.  However, third party technology that is not Separately Licensed Third Party Technology shall be deemed part of the Product and is licensed to you under the terms of this Agreement. “Separate Terms” refers to separate license terms that are specified in the Product’s documentation, readmes, or notice files, and that apply to Separately Licensed Third Party Technology. “Separately Licensed Third Party Technology” refers to third party technology that is licensed under Separate Terms and not under the terms of this Agreement.


§ 7 Relevant Laws, Regulations, and Sanctions.  Government laws, regulations, and/or legal sanctions (including export laws) may apply to the Product. You agree that such laws, regulations, and/or legal sanctions (collectively the “Rules”) govern your use of the Product (including technical data) provided under this Agreement, and you agree to comply with all such Rules (including “deemed export” and “deemed re-export” regulations). You agree that no data, information, and/or Product (or direct product thereof) will be exported, directly or indirectly, in violation of these Rules, or will be used for any purpose prohibited by these Rules including, without limitation, nuclear, chemical, or biological weapons proliferation, or development of missile technology.  To the extent that any of the Rules prohibits granting a license to you, no license is granted.


§ 8 Government End Users.  For any end user using the Product on behalf of a governmental entity, the Product is “commercial computer software” pursuant to relevant applicable regulations, such as the Federal Acquisition Regulation and agency-specific supplemental regulations for the U.S. government. Therefore, use, duplication, disclosure, modification, and adaptation of the programs, including any operating system, integrated software, any programs installed on the hardware, and/or documentation, shall be subject to license terms and license restrictions applicable to the programs.  No other rights are granted to the governmental entity.


§ 9 Miscellaneous. This Agreement is the entire agreement between you and IDNI AG relating to its subject matter.  It supersedes all prior or contemporaneous oral or written communications, proposals, representations and warranties and prevails over any conflicting or additional terms of any quote, order, acknowledgment, or other communication between the parties relating to its subject matter.  If any provision of this Agreement is held to be unenforceable, this Agreement will remain in effect with the provision omitted, unless omission would frustrate the intent of the parties, in which case this Agreement will immediately terminate.  This Agreement is governed by the laws of Liechtenstein, and you agree to submit to the exclusive jurisdiction of, and venue in, the courts of Liechtenstein in any dispute arising out of or relating to this Agreement.  You agree to pay within 30 days of written notification any fees applicable to your unlicensed use of the Product.  If a legal action or proceeding is commenced in connection with the enforcement of this Agreement, IDNI AG shall be entitled to its costs and attorneys’ fees actually incurred in connection with such action or proceeding if IDNI AG prevails.)";

	*out_ << license << "\n";
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
	if (!opt_prefix(opt, isshort)) {
		if (filesystem::exists(opt))
			return files_.push_back(opt), ++arg, 0;
		else return error("Invalid command or file not exists: "
								+ opt, true);
	}
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
	if (status_ == 3 && (dflt_cmd_ == ""
			|| (files_.size() && dflt_cmd_when_files_ == "")))
		return 1;

	// get command
	string cmdarg = status_ == 2 ? args_[arg++]
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

int cli::status() const { return status_; }

} // namespace idni