#ifndef IO_SIMPLIFY_LIBUV_UDP_HANDLE_H
#define IO_SIMPLIFY_LIBUV_UDP_HANDLE_H

#include "libuv_loop.h"

#include "libuv_handle.h"

namespace io_simplify {

    namespace libuv {

        class UdpHandle : public Handle<uv_udp_t>
        {
        private:
            CallbackAlloc _callback_alloc;

        private:
            CallbackReceived _callback_received;
            CallbackSent _callback_sent;

        private:
            static void callback_uv_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
            {
                UdpHandle* server_handle = (UdpHandle*)(handle->data);

                server_handle->_callback_alloc(suggested_size, buf);
            }

            static void callback_uv_received(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
            {
                UdpHandle* server_handle = (UdpHandle*)(handle->data);

                server_handle->_callback_received(nread, buf, addr, flags);
            }

            static void callback_uv_sent(uv_udp_send_t* req, int status)
            {
                UdpHandle* server_handle = (UdpHandle*)(req->handle->data);

                server_handle->_callback_sent(req, status);
            }

        public:
            explicit UdpHandle(Loop* loop)
                : Handle<uv_udp_t>(loop)

                , _callback_alloc()
                , _callback_received()
            {
                Handle<uv_udp_t>::status = uv_udp_init(loop->uv, Handle<uv_udp_t>::uv);
            }

            UdpHandle(Loop* loop, unsigned int flags)
                : Handle<uv_udp_t>(loop)
                
                , _callback_alloc()

                , _callback_received()
                , _callback_sent()
            {
                Handle<uv_udp_t>::status = uv_udp_init_ex(loop->uv, Handle<uv_udp_t>::uv, flags);
            }

            ~UdpHandle()
            {
            }

            // ttl – 1 through 255.
            int SetTTL(int ttl)
            {
                return uv_udp_set_ttl(Handle<uv_udp_t>::uv, ttl);
            }

            // ttl – 1 through 255.
            int SetMulticastTTL(int ttl)
            {
                return uv_udp_set_multicast_ttl(Handle<uv_udp_t>::uv, ttl);
            }

            // Set the multicast interface to send or receive data on.
            int SetMulticastInterface(const char* interface_addr)
            {
                return uv_udp_set_multicast_interface(Handle<uv_udp_t>::uv, interface_addr);
            }

            // Makes multicast packets loop back to local sockets.
            // on – 1 for on, 0 for off.
            int SetMulticastLoopback(int on)
            {
                return uv_udp_set_multicast_loop(Handle<uv_udp_t>::uv, on);
            }

            // on – 1 for on, 0 for off.
            int SetBroadcast(int on)
            {
                return uv_udp_set_broadcast(Handle<uv_udp_t>::uv, on);
            }

            int Bind(const Endpoint& endpoint, unsigned int flags = UV_UDP_REUSEADDR)
            {
                struct sockaddr_in addr;
                int res = uv_ip4_addr(endpoint.address.c_str(), endpoint.port, &addr);
                do
                {
                    if (res < 0)
                    {
                        break;
                    }
                    
                    res = uv_udp_bind(Handle<uv_udp_t>::uv, (const struct sockaddr*)&addr, flags);
                } while (false);
                
                return res;
            }

            int Bind(const struct sockaddr *addr, unsigned int flags = UV_UDP_REUSEADDR)
            {
                return uv_udp_bind(Handle<uv_udp_t>::uv, addr, flags);
            }

            int JoinMulticastGroup(const char* multicast_addr, const char* interface_addr)
            {
                return uv_udp_set_membership(Handle<uv_udp_t>::uv, multicast_addr, interface_addr, UV_JOIN_GROUP);
            }

            int LeaveMulticastGroup(const char* multicast_addr, const char* interface_addr)
            {
                return uv_udp_set_membership(Handle<uv_udp_t>::uv, multicast_addr, interface_addr, UV_LEAVE_GROUP);
            }

            int JoinMulticastSourceGroup(const char* multicast_addr, const char* interface_addr, const char* source_addr)
            {
                return uv_udp_set_source_membership(Handle<uv_udp_t>::uv, multicast_addr, interface_addr, source_addr, UV_JOIN_GROUP);
            }

            int LeaveMulticastSourceGroup(const char* multicast_addr, const char* interface_addr, const char* source_addr)
            {
                return uv_udp_set_source_membership(Handle<uv_udp_t>::uv, multicast_addr, interface_addr, source_addr, UV_LEAVE_GROUP);
            }

            int Connect(const Endpoint& endpoint)
            {
                struct sockaddr_in addr;
                int res = uv_ip4_addr(endpoint.address.c_str(), endpoint.port, &addr);
                do
                {
                    if (res < 0)
                    {
                        break;
                    }
                    
                    res = uv_udp_connect(Handle<uv_udp_t>::uv, (const struct sockaddr*)&addr);
                } while (false);
                
                return res;
            }

            int Connect(const struct sockaddr *addr)
            {
                return uv_udp_connect(Handle<uv_udp_t>::uv, addr);
            }

            int GetEndpoint(Endpoint& endpoint)
            {
                struct sockaddr sock_address;
                int sock_address_len = sizeof(sock_address);

                int res = uv_udp_getpeername(Handle<uv_udp_t>::uv, &sock_address, &sock_address_len);
                if (0 == res)
                {
                    struct sockaddr_in* sock_address_sin = (struct sockaddr_in*)(&sock_address);
                    endpoint.port = ntohs(sock_address_sin->sin_port);

                    endpoint.address.resize(32, 0);

                    res = uv_inet_ntop(sock_address.sa_family, &(sock_address_sin->sin_addr), endpoint.address.data(), endpoint.address.size());
                }
                
                return res;
            }

            int StartReceive(
                const CallbackReceived& callback_received, 
                const CallbackAlloc& callback_alloc = [] (size_t suggested_size, uv_buf_t *buf) {
                    buf->base = (char*)malloc(suggested_size);
                    buf->len = suggested_size;
                })
            {
                _callback_alloc = callback_alloc;
                _callback_received = callback_received;

                return uv_udp_recv_start(Handle<uv_udp_t>::uv, callback_uv_alloc, callback_uv_received);
            }

            void StopReceive()
            {
                uv_udp_recv_stop(Handle<uv_udp_t>::uv);
            }

            int Send(uv_udp_send_t* req,
                       const uv_buf_t* bufs,
                       unsigned int nbufs,
                       const CallbackSent& callback_sent)
            {
                _callback_sent = callback_sent;

                return uv_udp_send(req, Handle<uv_udp_t>::uv, bufs, nbufs, nullptr, callback_uv_sent);
            }

            int Send(uv_udp_send_t* req,
                       const uv_buf_t* bufs,
                       unsigned int nbufs,
                       const struct sockaddr* addr,
                       const CallbackSent& callback_sent)
            {
                _callback_sent = callback_sent;

                return uv_udp_send(req, Handle<uv_udp_t>::uv, bufs, nbufs, addr, callback_uv_sent);
            }

        private:
            UdpHandle() = delete;

            UdpHandle(const UdpHandle&) = delete;
            UdpHandle& operator=(const UdpHandle&) = delete;

            UdpHandle(UdpHandle&&) = delete;
            UdpHandle& operator=(UdpHandle&&) = delete;
        };
    }
}

#endif
