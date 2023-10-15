#define ASIO_STANDALONE
#define WIN32_LEAN_AND_MEAN


#include <string>
#include <mutex>
#include <stdint.h>
#include <asio.hpp>
#include <websocketpp/uri.hpp>
#include "ClientWebSocket.hpp"
#include "c-wspp.h"


struct wspp_ws {
    WSPP::AbstractClientWebSocket* impl;
};


wspp_ws* wspp_new(const char* uri_string)
{
    wspp_ws* ws = new wspp_ws();
    websocketpp::uri uri(uri_string);
    if (uri.get_secure()) {
        ws->impl = new WSPP::SecureClientWebSocket(uri_string);
    } else {
        ws->impl = new WSPP::PlainClientWebSocket(uri_string);
    }
    return ws;
}

void wspp_delete(wspp_ws* ws)
{
    delete ws->impl;
    ws->impl = nullptr;
    delete ws;
}

uint64_t wspp_poll(wspp_ws* ws)
{
    return (uint64_t)ws->impl->poll();
}

uint64_t wspp_run(wspp_ws* ws)
{
    return (uint64_t)ws->impl->run();
}

bool wspp_stopped(wspp_ws* ws)
{
    return ws->impl->stopped();
}

wspp_error wspp_connect(wspp_ws* ws)
{
    asio::error_code ec;
    ws->impl->connect(ec);
    if (ec)
        return WSPP_UNKNOWN;
    return WSPP_OK;
}

wspp_error wspp_close(wspp_ws* ws, uint16_t code, const char* reason)
{
    asio::error_code ec;
    ws->impl->close(code, reason, ec);
    if (ec)
        return WSPP_UNKNOWN;
    return WSPP_OK;
}

wspp_error wspp_send_text(wspp_ws* ws, const char* message)
{
    asio::error_code ec;
    ws->impl->send_text(message, ec);
    if (ec)
        return WSPP_UNKNOWN;
    return WSPP_OK;
}

wspp_error wspp_send_binary(wspp_ws* ws, const void* data, uint64_t len)
{
    asio::error_code ec;
    ws->impl->send_binary(std::string((const char*)data, len), ec);
    if (ec)
        return WSPP_UNKNOWN;
    return WSPP_OK;
}

wspp_error wspp_ping(wspp_ws* ws, const void* data, uint64_t len)
{
    asio::error_code ec;
    ws->impl->ping(std::string((const char*)data, len), ec);
    if (ec)
        return WSPP_UNKNOWN;
    return WSPP_OK;
}

void wspp_set_open_handler(wspp_ws* ws, OnOpenCallback f)
{
    ws->impl->set_open_handler([f]() {
        if (f)
            f();
    });
}

void wspp_set_close_handler(wspp_ws* ws, OnCloseCallback f)
{
    ws->impl->set_close_handler([ws, f]() { // TODO: code, reason
        if (f)
            f();
    });
}

void wspp_set_message_handler(wspp_ws* ws, OnMessageCallback f)
{
    ws->impl->set_message_handler([f](const std::string& message, int opCode) {
        if (f)
            f(message.c_str(), message.length(), opCode);
    });
}

void wspp_set_error_handler(wspp_ws* ws, OnErrorCallback f)
{
    // TODO: fail_handler instead?
    ws->impl->set_error_handler([f](const std::string& msg) { // TODO: errorCode, message
        if (f)
            f(msg.c_str());
    });
}

void wspp_set_pong_handler(wspp_ws* ws, OnPongCallback f)
{
    ws->impl->set_pong_handler([ws, f](const std::string& data) {
        if (f)
            f(data.c_str(), data.length());
    });
}
