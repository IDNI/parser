// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_SERVER_ASIO_H__
#define __IDNI__PARSER__NET__LINE_SERVER_ASIO_H__

#include <boost/asio/io_context.hpp>

namespace idni {

/// The io loop of the running line server, for the code of this
/// repository only: the header includes Boost and is no part of the SDK.
/// A program outside the repository uses line_server_post. Valid from
/// the first make_handler call until line_server_run returns.
boost::asio::io_context& line_server_io();

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_SERVER_ASIO_H__
