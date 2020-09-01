#pragma once
namespace knet {

namespace http {

enum class HttpMethod {
	HTTP_GET,
	HTTP_HEAD,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_TRACE,
	HTTP_OPTIONS,
	HTTP_CONNECT,
	HTTP_PATCH
};
enum class HttpVersion { HTTP_1_0, HTTP_1_1, HTTP_2_0 };


const std::string http_version_1_0 = "HTTP/1.0"; 
const std::string http_version_1_1 = "HTTP/1.1"; 
const std::string http_version_2_0 = "HTTP/1.0"; 

const std::string http_method_get = "GET";
const std::string http_method_head = "HEAD";
const std::string http_method_post = "POST";
const std::string http_method_put  = "PUT";
const std::string http_method_delete = "DELETE";
const std::string http_method_trace = "TRACE";
const std::string http_method_options = "OPTIONS";
const std::string http_method_connect = "CONNECT";
const std::string http_method_patch = "PATCH";


// constexpr HttpMethod method_get() { return HttpMethod::HTTP_GET; }
inline const std::string& http_method_string(HttpMethod method){
	switch(method)
	{
		case HttpMethod::HTTP_GET: 
		return http_method_get; 
		break; 
		case HttpMethod::HTTP_POST:
		return http_method_post;
		case HttpMethod::HTTP_HEAD:
		return http_method_head; 
		case HttpMethod::HTTP_PUT:
		return http_method_put; 
		case HttpMethod::HTTP_DELETE:
		return http_method_delete; 
		case HttpMethod::HTTP_TRACE:
		return http_method_trace; 
		case HttpMethod::HTTP_OPTIONS:
		return http_method_options; 
		case HttpMethod::HTTP_CONNECT:
		return http_method_connect; 
		case HttpMethod::HTTP_PATCH: 
		return http_method_trace; 
		default:
		return http_method_get;  
	}
}

class HttpQueryParameter final {
public:
	void add(const std::string& k, const std::string& v) {
		if (!parameters.empty()) {
			parameters += "&";
		}

		parameters += k;
		parameters += "=";
		parameters += v;
	}

	const std::string& to_string() const { return parameters; }

private:
	std::string parameters;
};

namespace mime_types {
static struct mapping {
	const char* extension;
	const char* mime_type;
} mappings[] = {{"gif", "image/gif"}, {"htm", "text/html"}, {"html", "text/html"},
	{"jpg", "image/jpeg"}, {"png", "image/png"}, {"txt", "text/html"}};

inline std::string to_mime(const std::string& ext) {
	for (mapping m : mappings) {
		if (m.extension == ext) {
			return m.mime_type;
		}
	}

	return "text/plain";
}

}; // namespace mime_types

namespace status_strings {
const std::string websocket_handshake = "HTTP/1.1 101  Web Socket Protocol Handshake"; 
const std::string ok = "HTTP/1.0 200 OK";
const std::string created = "HTTP/1.0 201 Created";
const std::string accepted = "HTTP/1.0 202 Accepted";
const std::string no_content = "HTTP/1.0 204 No Content";
const std::string multiple_choices = "HTTP/1.0 300 Multiple Choices";
const std::string moved_permanently = "HTTP/1.0 301 Moved Permanently";
const std::string moved_temporarily = "HTTP/1.0 302 Moved Temporarily";
const std::string not_modified = "HTTP/1.0 304 Not Modified";
const std::string bad_request = "HTTP/1.0 400 Bad Request";
const std::string unauthorized = "HTTP/1.0 401 Unauthorized";
const std::string forbidden = "HTTP/1.0 403 Forbidden";
const std::string not_found = "HTTP/1.0 404 Not Found";
const std::string internal_server_error = "HTTP/1.0 500 Internal Server Error";
const std::string not_implemented = "HTTP/1.0 501 Not Implemented";
const std::string bad_gateway = "HTTP/1.0 502 Bad Gateway";
const std::string service_unavailable = "HTTP/1.0 503 Service Unavailable";

inline const std::string& to_string(uint32_t code) {

	switch (code) {
	case 101:
		return status_strings::websocket_handshake; 
	case 200:
		return status_strings::ok;
	case 201:
		return status_strings::created;
	case 202:
		return status_strings::accepted;
	case 204:
		return status_strings::no_content;

	case 300:
		return status_strings::multiple_choices;
	case 301:
		return status_strings::moved_permanently;
	case 302:
		return status_strings::moved_temporarily;
	case 304:
		return status_strings::not_modified;
	case 400:
		return status_strings::bad_request;

	case 401:
		return status_strings::unauthorized;
	case 403:
		return status_strings::forbidden;

	case 404:
		return status_strings::not_found;
	case 500:
		return status_strings::internal_server_error;
	case 501:
		return status_strings::not_implemented;
	case 502:
		return status_strings::bad_gateway;
	case 503:
		return status_strings::service_unavailable;
	default:
		return status_strings::bad_request;
	};
}

} // namespace status_strings

namespace misc_strings {

const std::string key_value_seprator = ": ";
const std::string crlf = "\r\n";
// const char name_value_separator[] = { ':', ' ' };
// const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

} // namespace http
} // namespace knet
