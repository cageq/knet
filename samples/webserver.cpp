
#include "http/http_server.hpp"

using namespace knet::http;

int main(int argc, char *argv[])
{

	KNetLogIns.add_console();
	HttpServer<> webSrv;

	webSrv.register_router("/", [](auto & req, auto & rsp)
						   { return rsp.write("welcome my website"); });

	
	webSrv.register_router("/index", [](const HttpRequest &req, HttpResponse &  rsp) {

		std::cout << "path " << req.path() << std::endl;
		std::cout << "query " << req.query() << std::endl; 
		std::cout << " req " << req.to_string() << std::endl;  
		return rsp.write("hello server", 200); 
	});

	webSrv.register_router("/context", [](auto ctx)
						   { return ctx->write("hello server"); });

	webSrv.start(8888, "0.0.0.0");
	char c = getchar();
	while (c)
	{

		if (c == 'q')
		{
			printf("quiting ...\n");
			break;
		}
		c = getchar();
	}

	return 0;
};
