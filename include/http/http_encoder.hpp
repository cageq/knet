#pragma once
#include "http_message.hpp"
#include "utils/knet_log.hpp"
#include <map>

namespace knet {
namespace http {

template <class T> class HttpEncoder {
public:
  struct Header {
    std::string key;
    std::string value;
  };

  HttpEncoder(T &msg) : http_message(msg) {}
  void set_method(HttpMethod protocol) { http_method = protocol; }
  void set_status(uint32_t status) { status_code = status; }

  void set_cookie(const std::string &v) { add_header("Cookie", v); }

  void set_content_type(const std::string &v) { add_header("Content-Type", v); }

  void set_host(const std::string &host) { add_header("Host", host); }

  void set_url(const std::string &url) { http_url = url; }

  void set_content(const std::string &body, const std::string &type = "txt") {
    context_type = type;
    http_body = body;
    add_header("Content-Length", std::to_string(http_body.size()));
  }
  void set_agent(const std::string &agent) { add_header("User-Agent", agent); }

  void add_header(const std::string &key, const std::string &value) {
    http_headers[key] = value;
  }

  std::string encode_request(HttpMethod method, const std::string &url,
                             const std::string &body,
                             const std::string &type = "txt") {

    set_method(method);
    set_url(url);
    context_type = type;

    fmt::memory_buffer msgBuf;

    fmt::format_to(std::back_inserter(msgBuf), "{} {} {}\r\n",
                   http_method_string(http_method), http_url, http_version_1_1);

    for (const auto &h : http_headers) {
      if (!h.first.empty()) {
        fmt::format_to(std::back_inserter(msgBuf), "{}: {}\r\n", h.first,
                       h.second);
      }
    }

    if (!content.empty()) {
      set_content_type(mime_types::to_mime(type));
    }

    fmt::format_to(std::back_inserter(msgBuf), "\r\n{}", content);
    return fmt::to_string(msgBuf);
  }

  std::string encode_response() {
    fmt::memory_buffer msgBuf;

    fmt::format_to(std::back_inserter(msgBuf), "{}\r\n",
                   status_strings::to_string(status_code));

    for (const auto &h : http_headers) {
      fmt::format_to(std::back_inserter(msgBuf), "{}: {}\r\n", h.first,
                     h.second);
    }

    if (!content.empty()) {
      set_content_type(mime_types::to_mime(context_type));
    }

    fmt::format_to(std::back_inserter(msgBuf), "\r\n{}", content);

    return fmt::to_string(msgBuf);
  }

  std::string encode() {
    fmt::memory_buffer msgBuf;
    if (status_code > 0) {
      fmt::format_to(std::back_inserter(msgBuf), "{}\r\n",
                     status_strings::to_string(status_code));
    } else {
      fmt::format_to(std::back_inserter(msgBuf), "{} {} {}\r\n",
                     http_method_string(http_method), http_url,
                     http_version_1_1);
    }
    set_agent("GHttp v0.1");

    for (const auto &h : http_headers) {
      if (!h.first.empty()) {
        fmt::format_to(std::back_inserter(msgBuf), "{}: {}\r\n", h.first,
                       h.second);
      }
    }
    // add_time();

    if (!content.empty()) {
      set_content_type(mime_types::to_mime(context_type));
    }

    fmt::format_to(std::back_inserter(msgBuf), "\r\n{}", content);
    return fmt::to_string(msgBuf);
  }

  // std::string to_string() const {
  // 	fmt::memory_buffer msgBuf;
  // 	fmt::format_to(msgBuf, "{}\r\n",
  // status_strings::to_string(status_code)); 	if (status_code == 200) {

  // 		fmt::format_to(msgBuf, "Server: GHttp Server v0.1\r\n");
  // 		for (const auto& h : http_headers) {
  // 			fmt::format_to(msgBuf, "{}: {}\r\n", h.first, h.second);
  // 		}

  // 		fmt::format_to(msgBuf, "Content-Type: {}\r\n",
  // mime_types::to_mime("txt")); 		fmt::format_to(msgBuf, "Content-Length:
  // {}\r\n", content.length());

  // 		if (!content.empty())
  // 		{
  // 			fmt::format_to(msgBuf, "\r\n{}", content);
  // 		}

  //         dlog("send response : \n{}", fmt::to_string(msgBuf));
  // 		return fmt::to_string(msgBuf);
  // 	} else {
  // 		auto body = status_strings::to_string(status_code);
  // 		fmt::format_to(msgBuf, "Content-Type: {}\r\n",
  // mime_types::to_mime("txt")); 		fmt::format_to(msgBuf, "Content-Length:
  // {}\r\n", body.length()); 		fmt::format_to(msgBuf, "\r\n{}", body); 		dlog("send
  // response : \n{}", fmt::to_string(msgBuf)); 		return fmt::to_string(msgBuf);
  // 	}
  // }

  void add_time() {
    std::locale::global(std::locale("zh_CN.utf8"));
    time_t now = std::time(0);
    struct tm tm = *gmtime(&now);
    char szBuf[1024];
    size_t len = strftime(szBuf, sizeof szBuf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    szBuf[len] = 0;
    add_header("Date", szBuf);
  }

  T &http_message;
  std::string context_type;
  std::string content;
  uint32_t status_code = 0;
  std::string http_body;
  HttpMethod http_method;
  std::string http_url;
  std::map<std::string, std::string> http_headers;
};
} // namespace http
} // namespace knet
