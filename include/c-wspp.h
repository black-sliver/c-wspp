#ifndef _C_WSPP_H
#define _C_WSPP_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct wspp_ws wspp_ws;

typedef void (*OnOpenCallback)(void);
typedef void (*OnCloseCallback)(void); // TODO: code, reason
typedef void (*OnMessageCallback)(const char* data, uint64_t len, int32_t opCode);
typedef void (*OnErrorCallback)(void); // TODO: errorCode, message
typedef void (*OnPongCallback)(const char* data, uint64_t len);

enum wspp_error {
    WSPP_OK = 0,
    WSPP_INVALID_STATE = 1,
    WSPP_UNKNOWN = -1,
};

wspp_ws* wspp_new(const char* uri);
void wspp_delete(wspp_ws* ws);
uint64_t wspp_poll(wspp_ws* ws);
uint64_t wspp_run(wspp_ws* ws);
bool wspp_stopped(wspp_ws* ws);
// NOTE: there is a chance we can use asio::run instead of asio::poll, but that's for the future
wspp_error wspp_connect(wspp_ws* ws);
wspp_error wspp_close(wspp_ws* ws, uint16_t code, const char* reason);
wspp_error wspp_send_text(wspp_ws* ws, const char* message);
wspp_error wspp_send_binary(wspp_ws* ws, const void* data, uint64_t len);
wspp_error wspp_ping(wspp_ws* ws, const void* data, uint64_t len);
void wspp_set_open_handler(wspp_ws* ws, OnOpenCallback f);
void wspp_set_close_handler(wspp_ws* ws, OnCloseCallback f);
void wspp_set_message_handler(wspp_ws* ws, OnMessageCallback f);
void wspp_set_error_handler(wspp_ws* ws, OnErrorCallback f);
void wspp_set_pong_handler(wspp_ws* ws, OnPongCallback f);

#ifdef __cplusplus
}
#endif

#endif
