# c-wspp

C Wrapper for WebSocket++ (Client only)

This is being used to implement a fake websocket-sharp to get recent SSL/TLS into older Mono projects.

## API

* `wspp_new(const char* uri) -> wspp_ws*` - create new websocket for URI (ws:// or wss://)
* `wspp_delete(wspp_ws*)` - close websocket and free memory
* `wspp_poll(wspp_ws*) -> uint64_t` - poll network and dispatch events non-blocking, returns number of handled events
* `wspp_run(wspp_ws*) -> uint64_t` - poll network and dispatch events blocking, returns number of handled events - this is preferred
* `wspp_stopped(wspp_ws*) -> bool` - returns true if the io service was shut down
* `wspp_connect(wspp_ws*) -> wspp_error` - connect to the uri. do this before poll/run. returns an error code
* `wspp_close(uint16_t code, const char* reason)` - disconnect the websocket. still need to poll/run until disconnected
* `wspp_send_text(wspp_ws*, const char* text) -> wspp_error` - send a text frame. text is utf8
* `wspp_send_binary(wspp_ws*, const void* data, uint64_t len) -> wspp_error` - send a binary frame
* `wspp_ping(wspp_ws*, const void* data, uint64_t len) -> wspp_error` - send a ping
* `wspp_set_open_handler(wspp_ws*, OnOpenCallback)` - set the open callback
* `wspp_set_close_handler(wspp_ws*, OnCloseCallback)` - set the close callback
* `wspp_set_message_handler(wspp_ws*, OnMessageCallback)` - set the message callback
* `wspp_set_error_handler(wspp_ws*, OnErrorCallback)` - set the error callback
* `wspp_set_pong_handler(wspp_ws*, OnErrorCallback)` - set the pong callback
* `enum wspp_error`
  * `WSPP_OK` - success
  * `WSPP_INVALID_STATE` - a function was called on a bad context - i.e. send_* while not connected
  * `WSPP_UNKNOWN` - unspecified or unknown error occured
* `OnOpenCallback`: `void(void)`
* `OnCloseCallback`: `void(void)` - TODO: code, reason
* `OnMessageCallback`: `void(const char* data, uint64_t len, uint32_t opCode)`
* `OnPongCallback`: `void(const char* data, uint64_t len)`

## TODO

* Better errors
* Submodule ASIO
* Testing
