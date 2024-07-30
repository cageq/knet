#ifndef __HTTP_URL_HPP__
#define __HTTP_URL_HPP__

#include <vector>
#include <utility>
#include <stdexcept>
#include <cstdint>
#include <ostream>
#include <utility>
#include <sstream>
namespace knet {
namespace http {

static const uint8_t tbl[256] = {0, 0, 0, 0, 0, 0, 0, 0, // NUL SOH STX ETX  EOT ENQ ACK BEL
	0, 0, 0, 0, 0, 0, 0, 0,								 // BS  HT  LF  VT   FF  CR  SO  SI
	0, 0, 0, 0, 0, 0, 0, 0,								 // DLE DC1 DC2 DC3  DC4 NAK SYN ETB
	0, 0, 0, 0, 0, 0, 0, 0,								 // CAN EM  SUB ESC  FS  GS  RS  US
	0x00, 0x01, 0x00, 0x00, 0x01, 0x20, 0x01, 0x01,		 // SP ! " #  $ % & '
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x08,		 //  ( ) * +  , - . /
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  0 1 2 3  4 5 6 7
	0x01, 0x01, 0x04, 0x01, 0x00, 0x01, 0x00, 0x10,		 //  8 9 : ;  < = > ?
	0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  @ A B C  D E F G
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  H I J K  L M N O
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  P Q R S  T U V W
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,		 //  X Y Z [  \ ] ^ _
	0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  ` a b c  d e f g
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  h i j k  l m n o
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,		 //  p q r s  t u v w
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,		 //  x y z {  | } ~ DEL
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

class parse_error : public std::invalid_argument {
	public:
		parse_error(const std::string& reason)
			: std::invalid_argument(reason) {}
};
inline bool is_char(char c, std::uint8_t mask) {
	return (tbl[static_cast<unsigned char>(c)] & mask) != 0;
}

inline bool is_chars(const char* s, const char* e, std::uint8_t mask) {
	while (s != e)
		if (!is_char(*s++, mask))
			return false;
	return true;
}

inline bool is_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

inline bool is_num(char c) { return c >= '0' && c <= '9'; }

inline bool is_alnum(char c) { return is_alpha(c) || is_num(c); }

inline bool is_hexdigit(char c) {
	return is_num(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

inline bool is_uint(const char*& s, const char* e, uint32_t max) {
	if (s == e || !is_num(*s))
		return false;
	const char* t = s;
	uint32_t val = *t++ - '0';
	if (val)
		while (t != e && is_num(*t))
			val = val * 10 + (*t++ - '0');
	if (val > max)
		return false;
	s = t;
	return true;
}

inline bool is_pchars(const char* s, const char* e) { return is_chars(s, e, 0x07); }

inline char get_hex_digit(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

inline void to_lower(std::string& s) {
	for (auto& c : s)
		if (c >= 'A' && c <= 'Z')
			c |= 0x20;
}

inline const char* find_first_of(const char* s, const char* e, const char* q) {
	for (; s != e; ++s)
		for (const char* t = q; *t; ++t)
			if (*s == *t)
				return s;
	return e;
}

inline const char* find_char(const char* s, const char* e, const char c) {
	while (s != e && *s != c)
		++s;
	return s;
}

inline bool is_scheme(const char* s, const char* e) {
	if (!s || !e || s == e || !is_alpha(*s))
		return false;
	char c;
	while (++s != e)
		if (!is_alnum(c = *s) && c != '+' && c != '-' && c != '.')
			return false;
	return true;
}

inline bool is_scheme(const std::string& s) { return is_scheme(s.data(), s.data() + s.length()); }

inline std::string normalize_scheme(const char* b, const char* e) {
	std::string o(b, e - b);
	to_lower(o);
	return o;
}

inline bool is_ipv4(const char* s, const char* e) {
	size_t l = e - s;
	if (l < 7 || l > 254)
		return false;
	for (const char* p = s; p != e; ++p)
		if (*p != '.' && !is_num(*p))
			return false;
	return true;
}

inline bool is_ipv4(const std::string& s) { return is_ipv4(s.data(), s.data() + s.length()); }

inline bool is_valid_ipv4(const char* s, const char* e) {
	return is_uint(s, e, 255) && s != e && *s++ == '.' && is_uint(s, e, 255) && s != e &&
		   *s++ == '.' && is_uint(s, e, 255) && s != e && *s++ == '.' && is_uint(s, e, 255) &&
		   s == e;
}

inline bool is_valid_ipv4(const std::string& s) {
	return is_valid_ipv4(s.data(), s.data() + s.length());
}

inline bool is_reg_name(const char* s, const char* e) { return is_chars(s, e, 0x01); }

inline bool is_reg_name(const std::string& s) {
	return is_reg_name(s.data(), s.data() + s.length());
}

inline std::string normalize_reg_name(const std::string& s) {
	std::string o(s);
	to_lower(o); // see rfc 4343
	return o;
}

inline bool is_ipv6(const char* s, const char* e) {
	size_t l = e - s;
	if (l < 2 || l > 254)
		return false;
	for (const char* p = s; p != e; ++p)
		if (*p != ':' && *p != '.' && !is_hexdigit(*p))
			return false;
	return true;
}

inline bool is_ipv6(const std::string& s) { return is_ipv6(s.data(), s.data() + s.length()); }

inline bool is_valid_ipv6(const char* s, const char* e) {
	if ((e - s) > 39 || (e - s) < 2)
		return false;
	bool null_field = false;
	const char *b = s, *p = s;
	int nfields = 0, ndigits = 0;
	if (p[0] == ':') {
		if (p[1] != ':')
			return false;
		null_field = true;
		b = (p += 2);
		if (p == e)
			return true;
	}
	while (p != e) {
		if (*p == '.') {
			return ((!null_field && nfields == 6) || (null_field && nfields < 7)) &&
				   is_valid_ipv4(b, e);
		} else if (*p == ':') {
			if (ndigits == 0) {
				if (null_field)
					return false;
				null_field = true;
			} else {
				++nfields;
				ndigits = 0;
			}
			b = ++p;
		} else {
			if ((++ndigits > 4) || !is_hexdigit(*p++))
				return false;
		}
	}
	if (ndigits > 0)
		++nfields;
	else {
		if (e[-1] == ':') {
			if (e[-2] == ':' && nfields < 8)
				return true;
			return false;
		}
	}
	return (!null_field && nfields == 8) || (null_field && nfields < 8);
}

inline bool is_valid_ipv6(const std::string& s) {
	return is_valid_ipv6(s.data(), s.data() + s.length());
}

static std::string normalize_IPv6(const char* s, const char* e) {
	if (!is_ipv6(s, e))
		throw parse_error("IPv6 [" + std::string(s, e - s) + "] is invalid");
	if ((e - s) == 2 && s[0] == ':' && s[1] == ':')
		return std::string(s, e - s);

	// Split IPv6 at colons
	const char *p = s, *tokens[10];
	if (*p == ':')
		++p;
	if (e[-1] == ':')
		--e;
	const char* b = p;
	size_t i = 0;
	while (p != e) {
		if (*p++ == ':') {
			tokens[i++] = b;
			b = p;
		}
	}
	if (i < 8)
		tokens[i++] = b;
	tokens[i] = p;
	size_t ntokens = i;

	// Get IPv4 address which is normalized by default
	const char *ipv4_b = nullptr, *ipv4_e = nullptr;
	if ((tokens[ntokens] - tokens[ntokens - 1]) > 5) {
		ipv4_b = tokens[ntokens - 1];
		ipv4_e = tokens[ntokens];
		--ntokens;
	}

	// Decode the fields
	std::uint16_t fields[8];
	size_t null_pos = 8, null_len = 0, nfields = 0;
	for (size_t i = 0; i < ntokens; ++i) {
		const char* p = tokens[i];
		if (p == tokens[i + 1] || *p == ':')
			null_pos = i;
		else {
			std::uint16_t field = get_hex_digit(*p++);
			while (p != tokens[i + 1] && *p != ':')
				field = (field << 4) | get_hex_digit(*p++);
			fields[nfields++] = field;
		}
	}
	i = nfields;
	nfields = (ipv4_b) ? 6 : 8;
	if (i < nfields) {
		size_t last = nfields;
		if (i != null_pos)
			do
				fields[--last] = fields[--i];
			while (i != null_pos);
		do
			fields[--last] = 0;
		while (last != null_pos);
	}

	// locate first longer sequence of zero
	i = null_len = 0;
	null_pos = nfields;
	size_t first = 0;
	for (;;) {
		while (i < nfields && fields[i] != 0)
			++i;
		if (i == nfields)
			break;
		first = i;
		while (i < nfields && fields[i] == 0)
			++i;
		if ((i - first) > null_len) {
			null_pos = first;
			null_len = i - first;
		}
		if (i == nfields)
			break;
	}
	if (null_len == 1) {
		null_pos = nfields;
		null_len = 1;
	}

	// Encode normalized IPv6
	std::stringstream str;
	if (null_pos == 0) {
		str << std::hex << ':';
		i = null_len;
	} else {
		str << std::hex << fields[0];
		for (i = 1; i < null_pos; ++i)
			str << ':' << fields[i];
		if (i < nfields)
			str << ':';
		i += null_len;
		if (i == 8 && null_len != 0)
			str << ':';
	}
	for (; i < nfields; ++i) {
		str << ':' << fields[i];
	}

	if (ipv4_b) {
		str << ':' << std::string(ipv4_b, ipv4_e - ipv4_b);
	}

	return str.str();
}

inline std::string normalize_IPv6(const std::string& s) {
	return normalize_IPv6(s.data(), s.data() + s.length());
}

inline bool is_port(const char* s, const char* e) { return is_uint(s, e, 65535) && s == e; }

inline bool is_port(const std::string& s) { return is_port(s.data(), s.data() + s.length()); }

static std::string normalize_path(const std::string& s) {
	if (s.empty())
		return s;
	std::string elem;
	std::vector<std::string> elems;
	std::stringstream si(s);

	while (!std::getline(si, elem, '/').eof()) {
		if (elem == "" || elem == ".")
			continue;
		if (elem == "..") {
			if (!elems.empty())
				elems.pop_back();
			continue;
		}
		elems.push_back(elem);
	}
	if (elem == ".")
		elems.push_back("");
	else if (elem == "..") {
		if (!elems.empty())
			elems.pop_back();
	} else
		elems.push_back(elem);

	std::stringstream so;
	if (s[0] == '/')
		so << '/';
	if (!elems.empty()) {
		auto it = elems.begin(), end = elems.end();
		so << *it;
		while (++it != end)
			so << '/' << *it;
	}
	return so.str();
}

static std::string decode(const char* s, const char* e) {
	std::string o;
	o.reserve(e - s);
	while (s != e) {
		char c = *s++, a, b;
		if (c == '%') {
			if (s == e || (a = get_hex_digit(*s++)) < 0 || s == e || (b = get_hex_digit(*s++)) < 0)
				throw parse_error("Invalid percent encoding");
			c = (a << 4) | b;
		}
		o.push_back(c);
	}
	return o;
}

static std::string decode_plus(const char* s, const char* e) {
	std::string o;
	o.reserve(e - s);
	while (s != e) {
		char c = *s++, a, b;
		if (c == '+')
			c = ' ';
		else if (c == '%') {
			if (s == e || (a = get_hex_digit(*s++)) < 0 || s == e || (b = get_hex_digit(*s++)) < 0)
				throw parse_error("Invalid percent encoding");
			c = (a << 4) | b;
		}
		o.push_back(c);
	}
	return o;
}

class encode {
public:
	encode(const std::string& s, std::uint8_t mask)
		: m_s(s)
		, m_mask(mask) {}

private:
	const std::string& m_s;
	std::uint8_t m_mask;
	friend std::ostream& operator<<(std::ostream& o, const encode& e) {
		for (const char c : e.m_s)
			if (is_char(c, e.m_mask))
				o << c;
			else
				o << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0xF];
		return o;
	}
};

class encode_query_key {
public:
	encode_query_key(const std::string& s, std::uint8_t mask)
		: m_s(s)
		, m_mask(mask) {}

private:
	const std::string& m_s;
	std::uint8_t m_mask;
	friend std::ostream& operator<<(std::ostream& o, const encode_query_key& e) {
		for (const char c : e.m_s)
			if (c == ' ')
				o << '+';
			else if (c == '+')
				o << "%2B";
			else if (c == '=')
				o << "%3D";
			else if (c == '&')
				o << "%26";
			else if (c == ';')
				o << "%3B";
			else if (is_char(c, e.m_mask))
				o << c;
			else
				o << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0xF];
		return o;
	}
};

class encode_query_val {
public:
	encode_query_val(const std::string& s, std::uint8_t mask)
		: m_s(s)
		, m_mask(mask) {}

private:
	const std::string& m_s;
	std::uint8_t m_mask;
	friend std::ostream& operator<<(std::ostream& o, const encode_query_val& e) {
		for (const char c : e.m_s)
			if (c == ' ')
				o << '+';
			else if (c == '+')
				o << "%2B";
			else if (c == '&')
				o << "%26";
			else if (c == ';')
				o << "%3B";
			else if (is_char(c, e.m_mask))
				o << c;
			else
				o << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0xF];
		return o;
	}
};

class HttpUrl {
public:
	// Exception thut may be thrown when decoding an URL or an assigning value
	

	// Exception that may be thrown when building an URL
	class build_error : public std::runtime_error {
	public:
		build_error(const std::string& reason)
			: std::runtime_error(reason) {}
	};

	// Default constructor
	HttpUrl()
		: m_parse(true)
		, m_built(true)
		, m_ip_v(-1) {}

	// Copy initializer constructor
	HttpUrl(const HttpUrl& url)
		: m_ip_v(-1) {
		assign(url);
	}

	// Move constructor
	HttpUrl(HttpUrl&& url)
		: m_ip_v(-1) {
		assign(std::move(url));
	}

	// Construct HttpUrl with the given string
	HttpUrl(const std::string& url_str)
		: m_url(url_str)
		, m_parse(false)
		, m_built(false)
		, m_ip_v(-1) {}

	// Assign the given URL string
	HttpUrl& operator=(const std::string& url_str) { return str(url_str); }

	// Assign the given HttpUrl object
	HttpUrl& operator=(const HttpUrl& url) {
		assign(url);
		return *this;
	}

	// Move the given HttpUrl object
	HttpUrl& operator=(HttpUrl&& url) {
		assign(std::move(url));
		return *this;
	}

	// Clear the HttpUrl object
	HttpUrl& clear() {
		m_url.clear();
		m_scheme.clear();
		m_user.clear();
		m_host.clear();
		m_port.clear();
		m_path.clear();
		m_query.clear();
		m_fragment.clear();
		m_ip_v = -1;
		m_built = true;
		m_parse = true;
		return *this;
	}

	// Build HttpUrl if needed and return it as string
	std::string str() const {
		if (!m_built)
			build_url();
		return m_url;
	}

	// Set the HttpUrl to the given string. All fields are overwritten
	HttpUrl& str(const std::string& url_str) {
		m_url = url_str;
		m_built = m_parse = false;
		return *this;
	}

	// Get scheme
	const std::string& scheme() const {
		lazy_parse();
		return m_scheme;
	}

	// Set scheme

	HttpUrl& scheme(const std::string& s) {
		if (!is_scheme(s))
			throw parse_error("Invalid scheme '" + s + "'");
		lazy_parse();
		std::string o(s);
		to_lower(o);
		if (o != m_scheme) {
			m_scheme = o;
			m_built = false;
			if ((m_scheme == "http" && m_port == "80") || (m_scheme == "https" && m_port == "443"))
				m_port = "";
		}
		return *this;
	}

	// Get user info
	const std::string& user_info() const {
		lazy_parse();
		return m_user;
	}

	// Set user info

	HttpUrl& user_info(const std::string& s) {
		if (s.length() > 256)
			throw parse_error("User info is longer than 256 characters '" + s + "'");
		lazy_parse();
		if (m_user != s) {
			m_user = s;
			m_built = false;
		}
		return *this;
	}

	// Get host
	const std::string& host() const {
		lazy_parse();
		return m_host;
	}

	// Set host
	HttpUrl& host(const std::string& h, uint8_t ip_v = 0) {
		if (h.length() > 253)
			throw parse_error("Host is longer than 253 characters '" + h + "'");
		lazy_parse();
		std::string o;
		if (h.empty())
			ip_v = -1;
		else if (is_ipv4(h)) {
			if (!is_valid_ipv4(h))
				throw parse_error("Invalid IPv4 address '" + h + "'");
			ip_v = 4;
			o = h;
		} else if (ip_v != 0 && ip_v != 4 && ip_v != 6) {
			if (!is_ipv6(h)) {
				throw parse_error("Invalid IPvFuture address '" + h + "'");
			}
			o = h;
		} else if (is_ipv6(h)) {
			if (!is_valid_ipv6(h))
				throw parse_error("Invalid IPv6 address '" + h + "'");
			ip_v = 6;
			o = normalize_IPv6(h);
		} else if (is_reg_name(h)) {
			ip_v = 0;
			o = normalize_reg_name(h);
		} else
			throw parse_error("Invalid host '" + h + "'");
		if (m_host != o || m_ip_v != ip_v) {
			m_host = o;
			m_ip_v = ip_v;
			m_built = false;
		}
		return *this;
	}

	// Get host IP version: 0=name, 4=IPv4, 6=IPv6, -1=undefined
	std::int8_t ip_version() const {
		lazy_parse();
		return m_ip_v;
	}

	// Get port
	uint32_t  port() const {
		lazy_parse();
		if (m_port.empty())
        {
            return 80; 
        }
        return std::stoi(m_port);         
	}

	// Set Port given as string

	HttpUrl& port(const std::string& p) {
		if (!is_port(p))
			throw parse_error("Invalid port '" + p + "'");
		lazy_parse();
		std::string o(p);
		if ((m_scheme == "http" && o == "80") || (m_scheme == "https" && o == "443"))
			o = "";
		if (m_port != o) {
			m_port = o;
			m_built = false;
		}
		return *this;
	}

	// Set port given as a 16bit unsigned integer
	HttpUrl& port(std::uint16_t num) { return port(std::to_string(num)); }

	// Get path
	const std::string& path() const {
		lazy_parse();
		return m_path;
	}

	// Set path
	HttpUrl& path(const std::string& p) {
		if (p.length() > 8000)
			throw parse_error("Path is longer than 8000 characters '" + p + "'");
		lazy_parse();
		std::string o(normalize_path(p));
		if (m_path != o) {
			m_path = o;
			m_built = false;
		}
		return *this;
	}

	class KeyVal {
	public:
		// Default constructor
		KeyVal() {}

		// Construct with provided Key and Value strings
		KeyVal(const std::string& key, const std::string& val)
			: m_key(key)
			, m_val(val) {}

		// Construct with provided Key string, val will be empty
		KeyVal(const std::string& key)
			: m_key(key) {}

		// Equality test operator
		bool operator==(const KeyVal& q) const { return m_key == q.m_key && m_val == q.m_val; }

		// Swap this with q
		void swap(KeyVal& q) {
			std::swap(m_key, q.m_key);
			std::swap(m_val, q.m_val);
		}

		// Get key
		const std::string& key() const { return m_key; }

		// Set key
		void key(const std::string& k) { m_key = k; }

		// Get value
		const std::string& val() const { return m_val; }

		// Set value
		void val(const std::string& v) { m_val = v; }

		// Output key value pair
		friend std::ostream& operator<<(std::ostream& o, const KeyVal& kv) {
			o << "<key(" << kv.m_key << ") val(" << kv.m_val << ")> ";
			return o;
		}

	private:
		std::string m_key;
		std::string m_val;
	};

	// Define Query as vector of Key Value pairs
	typedef std::vector<KeyVal> Query;

	// Get a reference to the query vector for read only access
	const Query& query() const {
		lazy_parse();
		return m_query;
	}

	// Get a reference to a specific Key Value pair in the query vector for read only access
	const KeyVal& query(size_t i) const {
		lazy_parse();
		if (i >= m_query.size())
			throw std::out_of_range("Invalid HttpUrl query index (" + std::to_string(i) + ")");
		return m_query[i];
	}

	// Get a reference to the query vector for a writable access
	Query& set_query() {
		lazy_parse();
		m_built = false;
		return m_query;
	}

	// Get a reference to specific Key Value pair in the query vector for a writable access
	KeyVal& set_query(size_t i) {
		lazy_parse();
		if (i >= m_query.size())
			throw std::out_of_range("Invalid HttpUrl query index (" + std::to_string(i) + ")");
		m_built = false;
		return m_query[i];
	}

	// Set the query vector to the Query vector q
	HttpUrl& set_query(const Query& q) {
		lazy_parse();
		if (q != m_query) {
			m_query = q;
			m_built = false;
		}
		return *this;
	}

	// Append KeyVal kv to the query
	HttpUrl& add_query(const KeyVal& kv) {
		lazy_parse();
		m_built = false;
		m_query.push_back(kv);
		return *this;
	}

	// Append key val pair to the query
	HttpUrl& add_query(const std::string& key, const std::string& val) {
		lazy_parse();
		m_built = false;
		m_query.emplace_back(key, val);
		return *this;
	}

	// Append key with empty val to the query
	HttpUrl& add_query(const std::string& key) {
		lazy_parse();
		m_built = false;
		m_query.emplace_back(key);
		return *this;
	}

	// Get the fragment
	const std::string& fragment() const {
		lazy_parse();
		return m_fragment;
	}

	// Set the fragment
	HttpUrl& fragment(const std::string& f) {
		if (f.length() > 256)
			throw parse_error("Fragment is longer than 256 characters '" + f + "'");
		lazy_parse();
		if (m_fragment != f) {
			m_fragment = f;
			m_built = false;
		}
		return *this;
	}

	// Output
	std::ostream& output(std::ostream& o) const {
		lazy_parse();
		if (!m_built)
			build_url();
		o << "HttpUrl:{url(" << m_url << ")";
		if (!m_scheme.empty())
			o << " scheme(" << m_scheme << ")";
		if (!m_user.empty())
			o << " user_info(" << m_user << ")";
		if (m_ip_v != -1)
			o << " host(" << m_host << ") IPv(" << (int)m_ip_v << ")";
		if (!m_port.empty())
			o << " port(" << m_port << ")";
		if (!m_path.empty())
			o << " path(" << m_path << ")";
		if (!m_query.empty()) {
			std::stringstream str;
			str << " query(";
			for (const auto& q : m_query)
				str << q;
			std::string s(str.str());
			o << s.substr(0, s.length() - 1) << ")";
		}
		if (!m_fragment.empty())
			o << "fragment(" << m_fragment << ") ";
		o << "}";
		return o;
	}

	// Output strean operator
	friend std::ostream& operator<<(std::ostream& o, const HttpUrl& u) { return u.output(o); }

private:
	// Copy assignment
	void assign(const HttpUrl& url) {
		m_parse = url.m_parse;
		m_built = url.m_built;
		if (m_parse) {
			m_scheme = url.m_scheme;
			m_user = url.m_user;
			m_host = url.m_host;
			m_ip_v = url.m_ip_v;
			m_port = url.m_port;
			m_path = url.m_path;
			m_query = url.m_query;
			m_fragment = url.m_fragment;
		}
		if (!m_parse || m_built)
			m_url = url.m_url;
	}

	// Move assignment
	void assign(HttpUrl&& url) {
		m_parse = url.m_parse;
		m_built = url.m_built;
		if (m_parse) {
			m_scheme = std::move(url.m_scheme);
			m_user = std::move(url.m_user);
			m_host = std::move(url.m_host);
			m_ip_v = std::move(url.m_ip_v);
			m_port = std::move(url.m_port);
			m_path = std::move(url.m_path);
			m_query = std::move(url.m_query);
			m_fragment = std::move(url.m_fragment);
		}
		if (!m_parse || m_built)
			m_url = std::move(url.m_url);
	}

	void build_url() const {
		lazy_parse();
		std::stringstream url;
		if (!m_scheme.empty())
			url << m_scheme << ":";
		if (!m_host.empty()) {
			url << "//";
			if (!m_user.empty())
				url << encode(m_user, 0x05) << '@';
			if (m_ip_v == 0 || m_ip_v == 4)
				url << m_host;
			else if (m_ip_v == 6)
				url << "[" << m_host << "]";
			else
				url << "[v" << std::hex << (int)m_ip_v << std::dec << '.' << m_host << "]";
			if (!m_port.empty())
				if (!((m_scheme == "http" && m_port == "80") ||
						(m_scheme == "https" && m_port == "443")))
					url << ":" << m_port;
		} else {
			if (!m_user.empty())
				throw HttpUrl::build_error("User info defined, but host is empty");
			if (!m_port.empty())
				throw HttpUrl::build_error("Port defined, but host is empty");
			if (!m_path.empty()) {
				const char *b = m_path.data(), *e = b + m_path.length(),
						   *p = find_first_of(b, e, ":/");
				if (p != e && *p == ':')
					throw HttpUrl::build_error(
						"The first segment of the relative path can't contain ':'");
			}
		}
		if (!m_path.empty()) {
			if (m_path[0] != '/' && !m_host.empty())
				throw HttpUrl::build_error("Path must start with '/' when host is not empty");
			url << encode(m_path, 0x0F);
		}
		if (!m_query.empty()) {
			url << "?";
			auto it = m_query.begin(), end = m_query.end();
			if (it->key().empty())
				throw HttpUrl::build_error("First query entry has no key");
			url << encode_query_key(it->key(), 0x1F);
			if (!it->val().empty())
				url << "=" << encode_query_val(it->val(), 0x1F);
			while (++it != end) {
				if (it->key().empty())
					throw HttpUrl::build_error("A query entry has no key");
				url << "&" << encode_query_key(it->key(), 0x1F);
				if (!it->val().empty())
					url << "=" << encode_query_val(it->val(), 0x1F);
			}
		}
		if (!m_fragment.empty())
			url << "#" << encode(m_fragment, 0x1F);
		m_built = false;
		m_url = url.str();
	}
	void lazy_parse() const {
		if (!m_parse)
			parse_url();
	}

	void parse_url() const {
		if (m_url.empty()) {
			const_cast<HttpUrl*>(this)->clear();
			m_parse = m_built = true;
			return;
		}
		if (m_url.length() > 8000)
			throw parse_error("URI is longer than 8000 characters");

		const char *s = m_url.data(), *e = s + m_url.length();
		std::int8_t ip_v = -1;
		const char *scheme_b, *scheme_e, *user_b, *user_e, *host_b, *host_e, *port_b, *port_e,
			*path_b, *path_e, *query_b, *query_e, *fragment_b, *fragment_e;
		scheme_b = scheme_e = user_b = user_e = host_b = host_e = port_b = port_e = path_b =
			path_e = query_b = query_e = fragment_b = fragment_e = nullptr;

		const char *b = s, *p = find_first_of(b, e, ":/?#");
		if (p == e) {
			if (!is_chars(b, p, 0x2F))
				throw parse_error("Path '" + std::string(b, p) + "' in '" +
									   std::string(s, e - s) + "' is invalid");
			path_b = b;
			path_e = e;
		} else {
			// get schema if any
			if (*p == ':') {
				if (!is_scheme(b, p))
					throw parse_error("Scheme in '" + std::string(s, e - s) + "' is invalid");
				scheme_b = b;
				scheme_e = p;
				p = find_first_of(b = p + 1, e, "/?#");
			}
			// get authority if any
			if (p != e && *p == '/' && (e - b) > 1 && b[0] == '/' && b[1] == '/') {
				const char* ea = find_first_of(b += 2, e, "/?#"); // locate end of authority
				p = find_char(b, ea, '@');
				// get user info if any
				if (p != ea) {
					if (!is_chars(b, p, 0x05))
						throw parse_error(
							"User info in '" + std::string(s, e - s) + "' is invalid");
					user_b = b;
					user_e = p;
					b = p + 1;
				}
				// Get IP literal if any
				if (*b == '[') {
					// locate end of IP literal
					p = find_char(++b, ea, ']');
					if (*p != ']')
						throw parse_error("Missing ] in '" + std::string(s, e - s) + "'");
					// decode IPvFuture protocol version
					if (*b == 'v') {
						if (is_hexdigit(*++b)) {
							ip_v = get_hex_digit(*b);
							if (is_hexdigit(*++b)) {
								ip_v = (ip_v << 8) | get_hex_digit(*b);
							}
						}
						if (ip_v == -1 || *b++ != '.' || !is_chars(b, p, 0x05))
							throw parse_error(
								"Host address in '" + std::string(s, e - s) + "' is invalid");
					} else if (is_ipv6(b, p)) {
						ip_v = 6;
					} else
						throw parse_error(
							"Host address in '" + std::string(s, e - s) + "' is invalid");
					host_b = b;
					host_e = p;
					b = p + 1;
				} else {
					p = find_char(b, ea, ':');
					if (is_ipv4(b, p))
						ip_v = 4;
					else if (is_reg_name(b, p))
						ip_v = 0;
					else
						throw parse_error(
							"Host address in '" + std::string(s, e - s) + "' is invalid");
					host_b = b;
					host_e = p;
					b = p;
				}
				// get port if any
				if (b != ea && *b == ':') {
					if (!is_port(++b, ea))
						throw parse_error("Port '" + std::string(b, ea - b) + "' in '" +
											   std::string(s, e - s) + "' is invalid");
					port_b = b;
					port_e = ea;
				}
				b = ea;
			}
			p = find_first_of(b, e, "?#");
			if (!is_chars(b, p, 0x2F))
				throw parse_error("Path '" + std::string(b, p) + "' in '" +
									   std::string(s, e - s) + "' is invalid");
			path_b = b;
			path_e = p;
			if (p != e && *p == '?') {
				p = find_char(b = p + 1, e, '#');
				query_b = b;
				query_e = p;
			}
			if (p != e && *p == '#') {
				if (!is_chars(p + 1, e, 0x3F))
					throw parse_error("Fragment '" + std::string(p + 1, e) + "' in '" +
										   std::string(s, e - s) + "' is invalid");
				fragment_b = p + 1;
				fragment_e = e;
			}
		}
		std::string _scheme, _user, _host, _port, _path, _query, _fragment;
		Query query_v;

		if (scheme_b)
			_scheme = normalize_scheme(scheme_b, scheme_e);
		if (user_b)
			_user = decode(user_b, user_e);
		if (host_b) {
			_host = decode(host_b, host_e);
			if (ip_v == 0)
				_host = normalize_reg_name(_host);
			else if (ip_v == 6)
				_host = normalize_IPv6(_host);
		}
		if (port_b)
			_port = std::string(port_b, port_e - port_b);
		if (path_b)
			_path = normalize_path(decode(path_b, path_e));
		if (query_b) {
			_query = std::string(query_b, query_e);
			p = b = query_b;
			while (p != query_e) {
				p = find_first_of(b, query_e, "=;&");
				if (!is_chars(b, p, 0x3F))
					throw parse_error("Query key '" + std::string(b, p) + "' in '" +
										   std::string(s, e - s) + "' is invalid");
				std::string key(decode_plus(b, p)), val;
				if (p != query_e) {
					if (*p == '=') {
						p = find_first_of(b = p + 1, query_e, ";&");
						if (!is_chars(b, p, 0x3F))
							throw parse_error("Query value '" + std::string(b, p) + "' in '" +
												   std::string(s, e - s) + "' is invalid");
						val = decode_plus(b, p);
					}
					b = p + 1;
				}
				query_v.emplace_back(key, val);
			}
		}
		if (fragment_b)
			_fragment = decode(fragment_b, fragment_e);

		m_scheme = _scheme;
		m_user = _user;
		m_host = _host;
		m_ip_v = ip_v;
		m_port = _port;
		m_path = _path;
		m_query = query_v;
		m_fragment = _fragment;
		m_parse = true;
		m_built = false;
	}

	mutable std::string m_scheme;
	mutable std::string m_user;
	mutable std::string m_host;
	mutable std::string m_port;
	mutable std::string m_path;
	mutable Query m_query;
	mutable std::string m_fragment;
	mutable std::string m_url;
	mutable bool m_parse;
	mutable bool m_built;
	mutable std::int8_t m_ip_v;
};

} // namespace http
} // namespace knet

#endif // URL_HPP
