#pragma once
#include <vector>


namespace knet{ 

	namespace pipe{ 
		enum PipeMsgType { PIPE_MSG_SHAKE_HAND = 1, PIPE_MSG_HEART_BEAT, PIPE_MSG_DATA };

		struct PipeMsgHead {
			uint32_t length : 23;
			uint32_t type : 9;

			PipeMsgHead(uint16_t t = 0, uint32_t len = 0)
				: length(len)
				  , type(t) {}
		};

		struct PipeMsg {

			std::vector<char> buffer;
			PipeMsgHead* head = nullptr;

			PipeMsg(size_t size = 512) {
				size = size < 128 ? 128 : size;
				buffer.reserve(size);
				head = (PipeMsgHead*)&(*buffer.begin());
				memset((char*)head, 0, sizeof(PipeMsgHead));
				buffer.resize(sizeof(PipeMsgHead));
				dlog("buffer size is {}", buffer.size());
			}

			template <class T>
				void fill(PipeMsgType type, const T& val) {
					fill(type, (const char*)&val, sizeof(T));
				}

			void fill(PipeMsgType type, const std::string& buf) { fill(type, buf.c_str(), buf.length()); }
			void fill(PipeMsgType type, const std::string_view& buf) {
				fill(type, buf.data(), buf.size());
			}

			bool fill(PipeMsgType type, const char* buf, uint32_t len) {
				head->type = static_cast<uint32_t>(type);
				std::copy(buf, buf + len, std::back_inserter(buffer));
				head->length = len;
				return true;
			}

			// bool fill(const std::string& buf) { return fill(buf.c_str(), buf.length()); }

			// bool fill(const char* buf, uint32_t len) {
			// 	std::copy(buf, buf + len, std::back_inserter(buffer));
			// 	head->length = len ;
			// 	return true;
			// }

			template <class T>
				bool append(T& val) {
					std::copy((const char*)&val, (const char*)&val + sizeof(T), std::back_inserter(buffer));
					head->length += sizeof(T);
					dlog("body size is {}", head->length);
					return true;
				}

			bool append(const std::string& val) {
				std::copy(val.begin(), val.end(), std::back_inserter(buffer));
				head->length += val.length();
				dlog("body size is {}", head->length);
				return true;
			}

			bool append(const char* pData, uint32_t len) {
				std::copy((const char*)&pData, pData + len, std::back_inserter(buffer));
				head->length += len;
				dlog("body size is {}", head->length);
				return true;
			}

			std::string_view to_string_view() const {
				return std::string_view((const char*)head, buffer.size());
			}

			std::string to_string() const { return std::string((const char*)head, buffer.size()); }
			const char * data() const {
				return buffer.data(); 
			}

			uint32_t length() const { return buffer.size(); }
			// const char* begin() const { return &(*buffer.begin()); }
			// const char* end() const { return (const char*)&(*buffer.begin()) + head->length; }
		};

		template <uint32_t size>
			struct PipeMessage {
				char data[size] = {0};
				
				PipeMsgHead  * head = nullptr; 			 

				PipeMessage(uint32_t type = 0  ){
					head = (PipeMsgHead *) data; 
					head->type = type; 
				}

				void append( const std::string & msg ){
					append(msg.data() , msg.length()); 
				}

				void append(const char * buf, uint32_t len ){
					if (len > 0  && len <= size -  sizeof(PipeMsgHead)  )
					{
						memcpy((char *) &data[0] + ( sizeof(PipeMsgHead) +head->length )  , buf , len ); 
						head->length += len; 
					} 
				}
				void set_type(uint32_t type ){
					head->type = type; 
				}
 
				uint32_t message_type()const {
					return head->type; 
				}
				uint32_t payload_length() const {
					return head->length; 
				}

				uint32_t length() const { return sizeof(PipeMsgHead) + head->length; }
				const char* begin() const { return (const char*)data; }
				const char* end() const { return (const char*)data + head->length; }
			};
 
	}

}
