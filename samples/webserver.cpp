
#include "http/http_server.hpp"

using namespace knet::http;

int main(int argc, char* argv[]) {

	kLogIns.add_sink<klog::ConsoleSink<std::mutex, true> >();
	HttpServer<> webSrv;

	webSrv.register_router("/", [](  HttpRequestPtr req) {
		return HttpResponse("welcome my server\n");
	});

	webSrv.register_router("/index", [](  HttpRequestPtr req) {
		return HttpResponse("hello , server");
	});

	webSrv.start(8888, "0.0.0.0");

	//co_sched.Start();


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
