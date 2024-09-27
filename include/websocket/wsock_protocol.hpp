#pragma once
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************


#include "knet.hpp"
 
using namespace knet::tcp; 
namespace knet {
namespace websocket {

struct MinFrameHead {

	uint32_t fin : 1;
	uint32_t rsv1 : 1;
	uint32_t rsv2 : 1;
	uint32_t rsv3 : 1;
	uint32_t opcode : 4;
	uint32_t mask : 1;
	uint32_t len : 7;
	uint64_t xlen;
};

enum  OpCode {
 	ERROR_FRAME = 0xff,
	CONTINUATION = 0x0,
	TEXT_FRAME = 0x1,
	BINARY_FRAME = 0x2,
	WSOCK_CLOSE = 8,
	PING = 9,
	PONG = 0xa,
};

struct WsFrame {
	unsigned int header_size;
	bool fin;
	bool mask;
	enum opcode_type {
		CONTINUATION = 0x0,
		TEXT_FRAME = 0x1,
		BINARY_FRAME = 0x2,
		WSOCK_CLOSE = 8,
		PING = 9,
		PONG = 0xa,
	} opcode;
	int N0;
	uint64_t N;
	uint8_t masking_key[4];
};


 template <class WsConn> 
struct WSockHandler {

	std::function<void(std::shared_ptr<WsConn>)> open = nullptr;
	std::function<void(std::shared_ptr<WsConn>, const std::string & )> message = nullptr;
	std::function<void(std::shared_ptr<WsConn>)> close = nullptr;
};
enum class WSockStatus { WSOCK_INIT, WSOCK_CONNECTING, WSOCK_OPEN, WSOCK_CLOSING, WSOCK_CLOSED };

 

#ifndef ntohll
#define htonll(x) \
	((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) \
	((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

using WSMessageHandler = std::function<void(std::string_view  )>;
struct WSMessageReader {

	MinFrameHead head = {0};
	WSMessageReader(uint32_t bufSize = 1024 * 8)
		: buffer_size(bufSize) {}

	uint64_t buffer_size = 4096;
	uint64_t left_length = 0;
	uint64_t payload_length = 0;

	uint32_t status = 0; 

	uint32_t read( const char* pData, uint32_t dataLen, WSMessageHandler handler) {
 
		knet_dlog("reading websocket message {} status {}", dataLen, status);
		if (dataLen < sizeof(uint16_t)) {
			return 0;
		}

		if (status == 0) {

			uint8_t* vals = (uint8_t*)pData;
 


			knet_dlog("fin is     : {}" , ((vals[0] & 0x80) == 0x80) );
			knet_dlog("opcode is  : {}" , (vals[0] & 0x0f) );
			knet_dlog("length is  : {}" , (vals[1] & 0x7F) );
			knet_dlog("mask is    : {}" , ((vals[1] & 0x80) == 0x80) );

			head.fin = ((vals[0] & 0x80) == 0x80);
			head.opcode = (vals[0] & 0x0f);
			head.len = (vals[1] & 0x7F);
			head.mask = ((vals[1] & 0x80) == 0x80);

			uint32_t payloadMark = vals[1] & 0x7f;
			uint32_t headSize = sizeof(uint16_t);

			uint32_t maskSize = head.mask ? sizeof(uint32_t) : 0;

			if (payloadMark < 126) {
				this->payload_length = payloadMark;
				if (handler) {
					knet_dlog("payload length is {} head size {} , mask size {}", payload_length, headSize , maskSize); 
					if (head.mask)
					{
						std::string payload ; 
						payload.reserve(payload_length); 

						uint8_t * maskKey = (uint8_t *)pData + headSize; 
						uint32_t payloadPos = headSize + maskSize; 
						for(uint32_t i = 0; i < payload_length; i++) {	
							payload[i] = pData[payloadPos+ i] ^ maskKey[i%4];  
						}
						handler(std::string_view(payload.data(), payload_length) );
					} else {

						handler(std::string_view((const char*)(pData + headSize + maskSize), payload_length) );
					}			 
					
				}
		 
				return payload_length + headSize + maskSize;

			} else if (payloadMark == 126) {
				if (dataLen < headSize + sizeof(uint16_t)) {
					return 0;
				}
				headSize += sizeof(uint16_t);
				payload_length = *(uint16_t*)((uint16_t*)pData + 1);
				payload_length = ntohs(payload_length);

				if (payload_length + headSize + maskSize > buffer_size) {

					left_length = payload_length + headSize + maskSize - buffer_size;
					if (handler) {

						auto ploadLen = (buffer_size - headSize) > payload_length
											? payload_length
											: (buffer_size - headSize); 
						if (head.mask)
						{
							std::string payload ; 
							payload.reserve(ploadLen); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < ploadLen; i++) {	
								payload[i]= pData[payloadPos+ i] ^ maskKey[i%4];  
							}

							handler(std::string_view(payload.data(), ploadLen) );
						} else {
							handler(std::string_view(pData + headSize+ maskSize, ploadLen) );
						}
						
					}
					//this->status = MessageStatus::MESSAGE_CHUNK;
					return dataLen;
				} else {
					if (handler) {

						if (head.mask)
						{
							std::string payload ; 
							payload.reserve(payload_length); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < payload_length; i++) {	
								payload[i] = pData[payloadPos+ i] ^ maskKey[i%4];  
							}
							handler(std::string_view(payload.data(), payload_length) );
						} else {
							handler(std::string_view(pData + headSize + maskSize, payload_length) );
						}						
					}
					//status = MessageStatus::MESSAGE_NONE;
					return payload_length + headSize + maskSize;
				}

				// std::cout << "length is " << payload_length << std::endl;
			} else if (payloadMark == 127) {
				if (dataLen < headSize + sizeof(uint64_t)) {
					return 0;
				}

				headSize += sizeof(uint64_t);
				payload_length = *(uint64_t*)((uint16_t*)pData + 1);

				payload_length = ntohll(payload_length);

				if (payload_length + headSize + maskSize > buffer_size) {
					left_length = payload_length + headSize + maskSize - buffer_size;

					if (handler) {
						handler(std::string_view(pData + headSize, buffer_size - headSize) );
					}
				//	this->status = MessageStatus::MESSAGE_CHUNK;
					return dataLen;
				} else {
					if (handler) {

						if (head.mask)
						{
							std::string payload ; 
							payload.reserve(payload_length); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < payload_length; i++) {	
								payload[i]	=  pData[payloadPos+ i] ^ maskKey[i%4];  
							}
							handler(std::string_view(payload.data(), payload_length) );
						} 
						else {
							handler(std::string_view(pData + headSize, payload_length) );
						}
						
					}
				//	status = MessageStatus::MESSAGE_NONE;
					return payload_length + headSize + maskSize;
				}
				// std::cout << "length is " << payloadLen << std::endl;
			}
		} else if (status == 1) {

			if (left_length < dataLen) {				
				handler(std::string_view(pData, left_length) );
				auto leftLen = left_length;
				left_length = 0;
			//	status = MessageStatus::MESSAGE_NONE;
				payload_length = 0;
				return leftLen;
			} else {
				handler(std::string_view(pData, dataLen) );
				left_length -= dataLen;
			//	status = MessageStatus::MESSAGE_CHUNK;
				return dataLen;
			}
		}

		return 0;
	}
};
} // namespace ws
} // namespace knet
