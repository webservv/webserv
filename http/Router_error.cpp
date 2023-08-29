#include "Router.hpp"

static const std::string	g_dir = "./document";
static const std::string    g_error_dir = g_dir + "/error.html";

void Router::makeErrorPage(void) {
    makeErrorResponse(404);
    if (resourceExists(g_error_dir)) {
        std::string data;
        readFile(g_error_dir, data);
        response.makeBody(data, data.length(), getMIME(g_error_dir));
    }
}

void Router::makeErrorResponse(int statusCode) {
    std::string reasonPhrase;
    std::string body;

    switch (statusCode) {
        case 400:
            reasonPhrase = "Bad Request";
            body = "The request could not be understood by the server due to malformed syntax.";
            break;
        case 401:
            reasonPhrase = "Unauthorized";
            body = "The request requires user authentication.";
            break;
        case 403:
            reasonPhrase = "Forbidden";
            body = "The server understood the request, but is refusing to fulfill it.";
            break;
        case 404:
            reasonPhrase = "Not Found";
            body = "The server has not found anything matching the Request-URI.";
            break;
        case 405:
            reasonPhrase = "Method Not Allowed";
            body = "The method specified in the Request-Line is not allowed for the resource identified by the Request-URI.";
            break;
        case 411:
            reasonPhrase = "Length Required";
            body = "The request did not specify the length of its content, which is required by the requested resource.";
            break;
        case 413:
            reasonPhrase = "Request Entity Too Large";
            body = "The server is refusing to process a request because the request entity is larger than the server is willing or able to process.";
            break;
        case 415:
            reasonPhrase = "Unsupported Media Type";
            body = "The server is refusing to service the request because the entity of the request is in a format not supported by the requested resource for the requested method.";
            break;
        case 500:
            reasonPhrase = "Internal Server Error";
            body = "The server encountered an unexpected condition which prevented it from fulfilling the request.";
            break;
        case 501:
            reasonPhrase = "Not Implemented";
            body = "The server does not support the functionality required to fulfill the request.";
            break;
        case 502:
            reasonPhrase = "Bad Gateway";
            body = "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.";
            break;
        case 503:
            reasonPhrase = "Service Unavailable";
            body = "The server is currently unable to handle the request due to a temporary overloading or maintenance of the server.";
            break;
        case 505:
            reasonPhrase = "HTTP Version Not Supported";
            body = "The server does not support, or refuses to support, the HTTP protocol version that was used in the request message.";
            break;
        default:
            statusCode = 500;
            reasonPhrase = "Internal Server Error";
            body = "An unexpected error occurred.";
            break;
    }

    response.makeStatusLine("HTTP/1.1", std::to_string(statusCode), reasonPhrase);
    response.makeBody(body, body.length(), "text/plain");
    haveResponse = true;
}
