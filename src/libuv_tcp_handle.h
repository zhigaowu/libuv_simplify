#ifndef IO_SIMPLIFY_LIBUV_TCP_HANDLE_H
#define IO_SIMPLIFY_LIBUV_TCP_HANDLE_H

#include "libuv_loop.h"

#include "libuv_handle.h"

namespace io_simplify {

    namespace libuv {

        class TcpHandle : public Handle<uv_tcp_t>
        {
        public:
            using CallbackListen = std::function<void(int)>;
            using CallbackConnect = std::function<void(uv_connect_t*, int)>;

        private:
            uv_stream_t* _stream;

        private:
            CallbackListen _callback_listen;
            CallbackConnect _callback_connect;

        private:
            CallbackAlloc _callback_alloc;

        private:
            CallbackRead _callback_read;
            CallbackWritten _callback_written;

        private:
            static void callback_uv_listen(uv_stream_t* stream, int status)
            {
                TcpHandle* server_handle = (TcpHandle*)(stream->data);

                server_handle->_callback_listen(status);
            }

            static void callback_uv_connect(uv_connect_t* req, int status)
            {
                TcpHandle* server_handle = (TcpHandle*)(req->handle->data);

                server_handle->_callback_connect(req, status);
            }

            static void callback_uv_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
            {
                TcpHandle* server_handle = (TcpHandle*)(handle->data);

                server_handle->_callback_alloc(suggested_size, buf);
            }

            static void callback_uv_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
            {
                TcpHandle* server_handle = (TcpHandle*)(stream->data);

                server_handle->_callback_read(nread, buf);
            }

            static void callback_uv_written(uv_write_t* req, int status)
            {
                TcpHandle* server_handle = (TcpHandle*)(req->handle->data);

                server_handle->_callback_written(req, status);
            }

        public:
            explicit TcpHandle(Loop* loop)
                : Handle<uv_tcp_t>(loop)
                , _stream((uv_stream_t*)(Handle<uv_tcp_t>::uv))

                , _callback_listen()
                , _callback_connect()

                , _callback_alloc()
                , _callback_read()
            {
                Handle<uv_tcp_t>::status = uv_tcp_init(loop->uv, Handle<uv_tcp_t>::uv);
            }

            TcpHandle(Loop* loop, unsigned int flags)
                : Handle<uv_tcp_t>(loop)
                , _stream((uv_stream_t*)(Handle<uv_tcp_t>::uv))

                , _callback_listen()
                
                , _callback_alloc()

                , _callback_read()
                , _callback_written()
            {
                Handle<uv_tcp_t>::status = uv_tcp_init_ex(loop->uv, Handle<uv_tcp_t>::uv, flags);
            }

            ~TcpHandle()
            {
            }

            int NoDelay(int enable)
            {
                return uv_tcp_nodelay(Handle<uv_tcp_t>::uv, enable);
            }

            int KeepAlive(int enable, unsigned int seconds)
            {
                return uv_tcp_keepalive(Handle<uv_tcp_t>::uv, enable, seconds);
            }

            int Bind(const Endpoint& endpoint, unsigned int flags = 0)
            {
                struct sockaddr_in addr;
                int res = uv_ip4_addr(endpoint.address.c_str(), endpoint.port, &addr);
                do
                {
                    if (res < 0)
                    {
                        break;
                    }
                    
                    res = uv_tcp_bind(Handle<uv_tcp_t>::uv, (const struct sockaddr*)&addr, flags);
                } while (false);
                
                return res;
            }

            int Bind(const struct sockaddr *addr, unsigned int flags = 0)
            {
                return uv_tcp_bind(Handle<uv_tcp_t>::uv, addr, flags);
            }

            int Listen(const CallbackListen& callback_listen, int backlog = 0)
            {
                _callback_listen = callback_listen;

                return uv_listen(_stream, backlog, callback_uv_listen);
            }

            int Accept(TcpHandle* client_handle)
            {
                return uv_accept(_stream, client_handle->_stream);
            }

            int Connect(uv_connect_t *req, const Endpoint& endpoint, const CallbackConnect& callback_connect)
            {
                struct sockaddr_in addr;
                int res = uv_ip4_addr(endpoint.address.c_str(), endpoint.port, &addr);
                do
                {
                    if (res < 0)
                    {
                        break;
                    }
                    
                    _callback_connect = callback_connect;

                    res = uv_tcp_connect(req, Handle<uv_tcp_t>::uv, (const struct sockaddr*)&addr, callback_uv_connect);
                } while (false);
                
                return res;
            }

            int Connect(uv_connect_t *req, const struct sockaddr *addr, const CallbackConnect& callback_connect)
            {
                _callback_connect = callback_connect;
                    
                return uv_tcp_connect(req, Handle<uv_tcp_t>::uv, addr, callback_uv_connect);
            }

            int GetEndpoint(Endpoint& endpoint)
            {
                struct sockaddr sock_address;
                int sock_address_len = sizeof(sock_address);

                int res = uv_tcp_getpeername(Handle<uv_tcp_t>::uv, &sock_address, &sock_address_len);
                if (0 == res)
                {
                    struct sockaddr_in* sock_address_sin = (struct sockaddr_in*)(&sock_address);
                    endpoint.port = ntohs(sock_address_sin->sin_port);

                    endpoint.address.resize(32, 0);

                    res = uv_inet_ntop(sock_address.sa_family, &(sock_address_sin->sin_addr), endpoint.address.data(), endpoint.address.size());
                }
                
                return res;
            }

            int StartRead(
                const CallbackRead& callback_read, 
                const CallbackAlloc& callback_alloc = [] (size_t suggested_size, uv_buf_t *buf) {
                    buf->base = (char*)malloc(suggested_size);
                    buf->len = suggested_size;
                })
            {
                _callback_alloc = callback_alloc;
                _callback_read = callback_read;

                return uv_read_start(_stream, callback_uv_alloc, callback_uv_read);
            }

            void StopRead()
            {
                /*
                    This function is idempotent and may be safely called on a stopped stream.
                    This function will always succeed; hence, checking its return value is unnecessary. 
                    A non-zero return indicates that finishing releasing resources may be pending on the next input event on that TTY on Windows, and does not indicate failure.
                */
                uv_read_stop(_stream);
            }

            int Write(uv_write_t* req,
                       const uv_buf_t* bufs,
                       unsigned int nbufs,
                       const CallbackWritten& callback_written)
            {
                _callback_written = callback_written;

                return uv_write(req, _stream, bufs, nbufs, callback_uv_written);
            }

        private:
            TcpHandle() = delete;

            TcpHandle(const TcpHandle&) = delete;
            TcpHandle& operator=(const TcpHandle&) = delete;

            TcpHandle(TcpHandle&&) = delete;
            TcpHandle& operator=(TcpHandle&&) = delete;
        };
    }
}

#endif
