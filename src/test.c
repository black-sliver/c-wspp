#include <c-wspp.h>

// test file - TODO: move this to test/

int main(int argc, char** argv)
{
    wspp_ws* ws = wspp_new("ws://localhost:38281");
    wspp_connect(ws);
    for(;;) {
        wspp_poll(ws);
    }
    return 0;
}
