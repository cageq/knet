//***************************************************************
//	created:	2019/07/01
//	author:		wkui
//***************************************************************

#pragma once

#include <asio/ssl.hpp>

namespace knet {
namespace tcp {
std::string get_password() { return "test"; }

inline asio::ssl::context* init_ssl(
	const std::string& chainFile, const std::string& dhFile, const std::string& caFile = "ca.pem") {
	asio::ssl::context* ssl_context = nullptr;
	ssl_context = new asio::ssl::context(asio::ssl::context::sslv23);
	ssl_context->set_options(asio::ssl::context::default_workarounds |
							 asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
	// ssl_context->set_password_callback(get_password);
	ssl_context->use_certificate_chain_file(chainFile);
	ssl_context->use_private_key_file(chainFile, asio::ssl::context::pem);
	ssl_context->use_tmp_dh_file(dhFile);

	// ssl_context->load_verify_file(caFile);
	return ssl_context;
}

} // namespace tcp
} // namespace knet