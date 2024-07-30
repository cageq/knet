#pragma once
#include <functional>
 
#include "http_request.hpp"
#include "http_response.hpp"
namespace knet
{
    namespace http
    {

        class HttpConnection; 
        class HttpContext
        {
        public:
 
            std::shared_ptr<HttpConnection> connection; 

            HttpRequestPtr  request; 

            HttpResponsePtr response; 
            
            std::function<void(HttpRequestPtr, HttpResponsePtr)> http_handler;
        };


        using HttpContextPtr = std::shared_ptr<HttpContext>; 

    }

}