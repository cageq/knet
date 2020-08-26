
#include <chrono>
#include "knet.hpp"

#include "http/http_client.hpp"

using namespace knet::http;
 

int main(int argc, char* argv[]) {
    HttpClient httpClient; 

    httpClient.get("http://127.0.0.1/index"); 

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}