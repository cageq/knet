
#include "http/http_server.hpp"

using namespace knet::http;

int main(int argc, char* argv[]) {

 	//KNetLogIns.add_console(); 
	HttpServer<> webSrv;

	webSrv.register_router("/", [](  auto  req, auto rsp) {
		return std::make_shared<HttpResponse> ("welcome my server\n");
	});

	webSrv.register_router("/index", [](  auto  req, auto  rsp) {

		std::cout << " req " << req->to_string() << std::endl; 
		return std::make_shared<HttpResponse>("hello , server");
	});

	webSrv.start(8888, "0.0.0.0"); 
	char c = getchar();
	while (c) {

		if (c == 'q') {
			printf("quiting ...\n");
			break;
		}
		c = getchar();
	}

	return 0;
};
