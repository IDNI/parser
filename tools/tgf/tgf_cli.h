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
#ifndef __IDNI_TGF_CLI_H__
#define __IDNI_TGF_CLI_H__
#include "../../src/cli.h"

namespace idni {

// TGF options and descriptions
cli::options tgf_options();

// TGF commands, their options and descriptions
cli::commands tgf_commands();

// TGF main function
int tgf_run(int argc, char** argv);

} // namespace idni
#endif // __IDNI_TGF_CLI_H__