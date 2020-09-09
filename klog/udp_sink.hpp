#pragma once
#include "log_sink.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

class UdpSink : public LogSink {

	public:
		bool init(const std::string& host = "127.0.0.1", uint16_t port = 8700) {
			sockfd = socket(AF_INET, SOCK_DGRAM, 0);
			if (sockfd < 0) {
				perror("open udp socket error\n");
			}

			server = gethostbyname(host.c_str());
			if (server == NULL) {
				fprintf(stderr, "ERROR, no such host as %s\n", host.c_str());
				return false;
			}

			memset( &serveraddr, 0, sizeof(serveraddr)) ; 
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr = inet_addr(host.c_str() ); 
			serveraddr.sin_port = htons(port);
			return true; 
		}

		int32_t write(const std::string& buf) {
			if (sockfd != -1)
			{
				return (int32_t)sendto(sockfd, buf.data(), buf.length(), 0, (struct sockaddr * ) &serveraddr, sizeof(serveraddr) );
			}
			return -1; 		
		}

	private:
		int sockfd =-1; 
		struct sockaddr_in serveraddr;
		struct hostent* server;
};

