#ifndef _CLIENTWEBSOCKET_HPP
#define _CLIENTWEBSOCKET_HPP


#include <functional>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#ifdef _WIN32
#include <wincrypt.h>
#endif


namespace WSPP {

// abstract client websocket
class AbstractClientWebSocket {
public:
    typedef std::function<void(void)> open_handler;
    typedef std::function<void(void)> close_handler; // TODO: code, reason
    typedef std::function<void(const std::string& message, int opCode)> message_handler;
    typedef std::function<void(const std::string& msg)> error_handler; // TODO: errorCode
    typedef std::function<void(const std::string& data)> pong_handler;

    virtual size_t poll() = 0;
    virtual size_t run() = 0;
    virtual bool stopped() = 0;
    virtual void connect(asio::error_code& ec) = 0;
    virtual void close(uint16_t code, const std::string& reason, asio::error_code& ec) = 0;
    virtual void send_text(const std::string& message, asio::error_code& ec) = 0;
    virtual void send_binary(const std::string& data, asio::error_code& ec) = 0;
    virtual void ping(const std::string& data, asio::error_code& ec) = 0;

    virtual void set_open_handler(open_handler f) = 0;
    virtual void set_close_handler(close_handler f) = 0;
    virtual void set_message_handler(message_handler f) = 0;
    virtual void set_error_handler(error_handler f) = 0;
    virtual void set_pong_handler(pong_handler f) = 0;

    virtual ~AbstractClientWebSocket(){}
};

namespace detail {
    // FIXME: this is a complete mess. there has to be a better way to detect if config is ssl-enabled
    static void warn(const char* fmt, ...)
    {
        va_list args;
        va_start (args, fmt);
        vfprintf (stderr, fmt, args);
        va_end (args);
    }

    template<class T>
    void init_ssl(websocketpp::client<T>&, const std::string& cert_store, bool validate_cert)
    {
    }

    template<>
    void init_ssl<websocketpp::config::asio_tls_client>(
        websocketpp::client<websocketpp::config::asio_tls_client>& client,
        const std::string& cert_store, bool validate_cert)
    {
        typedef asio::ssl::context SSLContext;
        typedef std::shared_ptr<SSLContext> SSLContextPtr;
        std::string store_path = cert_store; // make a copy for capture

        client.set_tls_init_handler([validate_cert, store_path] (std::weak_ptr<void>) -> SSLContextPtr {
            SSLContextPtr ctx = std::make_shared<SSLContext>(SSLContext::sslv23);
            asio::error_code ec;
            ctx->set_options(SSLContext::default_workarounds |
                             SSLContext::no_sslv2 |
                             SSLContext::no_sslv3 |
                             SSLContext::no_tlsv1 |
                             SSLContext::no_tlsv1_1 |
                             SSLContext::single_dh_use, ec);
            if (ec) warn("Error in ssl init: options: %s\n", ec.message().c_str());
            if (validate_cert) {
                if (!store_path.empty()) {
                    ctx->load_verify_file(store_path, ec);
                    if (ec) warn("Error in ssl init: load store: %s\n", ec.message().c_str());
                }
                if (store_path.empty() || !!ec) {
#ifdef _WIN32
                    // try to load certs from windows ca store
                    HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
                    if (hStore) {
                        X509_STORE* store = X509_STORE_new();
                        PCCERT_CONTEXT cert = NULL;
                        while ((cert = CertEnumCertificatesInStore(hStore, cert)) != NULL) {
                            X509 *x509 = d2i_X509(NULL,
                                                  (const unsigned char **)&cert->pbCertEncoded,
                                                  cert->cbCertEncoded);
                            if(x509) {
                                X509_STORE_add_cert(store, x509);
                                X509_free(x509);
                            }
                        }

                        CertFreeCertificateContext(cert);
                        CertCloseStore(hStore, 0);

                        SSL_CTX_set_cert_store(ctx->native_handle(), store);
                    } else {
                        warn("Error in ssl init: could not open windows ca store\n");
                    }
#else
                    // try openssl default location
                    ctx->set_default_verify_paths(ec);
                    if (ec) warn("Error in ssl init: paths: %s\n", ec.message().c_str());
#endif
                }
                ctx->set_verify_mode(asio::ssl::verify_peer, ec);
                if (ec) warn("Error in ssl init: mode: %s\n", ec.message().c_str());
            }
            return ctx;
        });
    }
} // namespace WSPP::detail

// templated client websocket
template<class CONFIG>
class ClientWebSocket final : public AbstractClientWebSocket {
    typedef asio::io_service Service;
    typedef asio::ssl::context SSLContext;
    typedef std::shared_ptr<SSLContext> SSLContextPtr;
    typedef websocketpp::client<CONFIG> Client;
    typedef typename Client::connection_ptr Connection;
    typedef typename Client::message_ptr message_ptr;

    Service _service;
    Client _client;
    Connection _connection;
    std::string _uri;

    open_handler _open_handler = nullptr;
    close_handler _close_handler = nullptr;
    message_handler _message_handler = nullptr;
    error_handler _error_handler = nullptr;
    pong_handler _pong_handler = nullptr;

    static void warn(const char* fmt, ...)
    {
        va_list args;
        va_start (args, fmt);
        vfprintf (stderr, fmt, args);
        va_end (args);
    }

public:
    ClientWebSocket(const std::string& uri_string)
    {
        _uri = uri_string;
    }

    virtual ~ClientWebSocket()
    {
        asio::error_code ec;
        close(1001, "Going away", ec);
    }

    virtual size_t poll() override
    {
        return _service.poll();
    }

    virtual size_t run() override
    {
        return _service.run();
    }

    virtual bool stopped() override
    {
        return _client.stopped();
    }

    virtual void connect(asio::error_code& ec) override
    {
#if 1
        _client.clear_access_channels(websocketpp::log::alevel::all);
        _client.set_access_channels(websocketpp::log::alevel::none | websocketpp::log::alevel::app);
        _client.clear_error_channels(websocketpp::log::elevel::all);
#endif
        _client.set_error_channels(websocketpp::log::elevel::warn|websocketpp::log::elevel::rerror|websocketpp::log::elevel::fatal);
        _client.set_pong_timeout(10000);
        _client.init_asio(&_service);

        detail::init_ssl(_client, "", true);

        _client.set_open_handler([this] (websocketpp::connection_hdl hdl) {
            if (_open_handler)
                _open_handler();
        });
        _client.set_close_handler([this] (websocketpp::connection_hdl hdl) {
            // TODO: code, reason
            if (_close_handler)
                _close_handler();
        });
        _client.set_message_handler([this] (websocketpp::connection_hdl hdl, message_ptr msg) {
            if (_message_handler)
                _message_handler(msg->get_payload(), (int)msg->get_opcode());
        });
        _client.set_fail_handler([this] (websocketpp::connection_hdl hdl) {
            // TODO: errorCode
            if (_error_handler) {
                asio::error_code ec;
                auto conn = _client.get_con_from_hdl(hdl, ec);
                if (ec)
                    _error_handler(ec.message());
                else if (conn->get_ec())
                    _error_handler(conn->get_ec().message());
                else if(!conn->get_response_msg().empty())
                    _error_handler(conn->get_response_msg());
                else
                    _error_handler("Unknown");
            }
        });
        _client.set_pong_handler([this] (websocketpp::connection_hdl hdl, std::string data) {
            if (_pong_handler)
                _pong_handler(data);
        });

        _connection = _client.get_connection(_uri, ec);
        if (ec) {
            //throw std::runtime_error("get_connection failed");
            return;
        }
        if (!_client.connect(_connection)) {
            ec = websocketpp::error::make_error_code(websocketpp::error::value::general);
        }
    }

    virtual void close(uint16_t code, const std::string& reason, asio::error_code& ec) override
    {
        _connection->close(code, reason, ec);
    }

    virtual void send_text(const std::string& message, asio::error_code& ec) override
    {
        _client.send(_connection, message, websocketpp::frame::opcode::text, ec);
        // TODO; error if not connected
    }

    virtual void send_binary(const std::string& data, asio::error_code& ec) override
    {
        _client.send(_connection, data, websocketpp::frame::opcode::binary, ec);
        // TODO; error if not connected
    }

    virtual void ping(const std::string& data, asio::error_code& ec) override
    {
        _client.ping(_connection, data, ec);
        // TODO: error if not connected
    }

    virtual void set_open_handler(open_handler f) override
    {
        _open_handler = f;
    }

    virtual void set_close_handler(close_handler f) override
    {
        _close_handler = f;
    }

    virtual void set_message_handler(message_handler f) override
    {
        _message_handler = f;
    }

    virtual void set_error_handler(error_handler f) override
    {
        _error_handler = f;
    }

    virtual void set_pong_handler(pong_handler f) override
    {
        _pong_handler = f;
    }
};

// implemented client websockets
typedef ClientWebSocket<websocketpp::config::asio_tls_client> SecureClientWebSocket;
typedef ClientWebSocket<websocketpp::config::asio_client> PlainClientWebSocket;

} // namespace WSPP

#endif
