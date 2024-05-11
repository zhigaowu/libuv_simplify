#ifndef IO_SIMPLIFY_LIBUV_HANDLE_H
#define IO_SIMPLIFY_LIBUV_HANDLE_H

#include "libuv_loop.h"

#include <string>
#include <functional>

#include <stdint.h>

namespace io_simplify {

    struct Endpoint
    {
        std::string address = {};
        uint16_t port;
    };

    namespace libuv {

        using CallbackAlloc = std::function<void(size_t, uv_buf_t*)>;

        /*
            nread is > 0 if there is data available or < 0 on error. 
            When we’ve reached EOF, nread will be set to UV_EOF. 
            When nread < 0, the buf parameter might not point to a valid buffer; in that case buf.len and buf.base are both set to 0.

            NOTE: nread might be 0, which does not indicate an error or EOF. This is equivalent to EAGAIN or EWOULDBLOCK under read(2).
        */
        using CallbackRead = std::function<void(ssize_t, const uv_buf_t*)>;

        using CallbackWritten = std::function<void(uv_write_t*, int)>;

        using CallbackReceived = std::function<void(ssize_t, const uv_buf_t*, const struct sockaddr*, unsigned)>;
        using CallbackSent = std::function<void(uv_udp_send_t*, int)>;

        using CallbackHandleClosed = std::function<void()>;

        template<typename uv_object_type>
        class Handle : public Base<uv_object_type>
        {
            CallbackHandleClosed _callback_handle_closed;

        private:
            static void callback_uv_close(uv_handle_t* handle)
            {
                Handle* handle_type = (Handle*)(handle->data);

                handle_type->_callback_handle_closed();
            }

        public:
            Loop* loop;

            uv_handle_t* uv_handle;

        public:
            explicit Handle(Loop* loop_ref)
                : Base<uv_object_type>()

                , _callback_handle_closed()

                , loop(loop_ref)
                , uv_handle((uv_handle_t*)(Base<uv_object_type>::uv))
            {
                uv_handle->data = this;
            }

            /*
                Request handle to be closed. close_cb will be called asynchronously after this call. 
                This MUST be called on each handle before memory is released. 
                Moreover, the memory can only be released in close_cb or after it has returned.

                Handles that wrap file descriptors are closed immediately but close_cb will still be deferred to the next iteration of the event loop. 
                It gives you a chance to free up any resources associated with the handle.

                In-progress requests, like uv_connect_t or uv_write_t, are cancelled and have their callbacks called asynchronously with status=UV_ECANCELED.
                close_cb can be NULL in cases where no cleanup or deallocation is necessary.
            */
            void Close(const CallbackHandleClosed& callback_handle_closed = nullptr)
            {
                if (0 == Base<uv_object_type>::status)
                {
                    _callback_handle_closed = callback_handle_closed;

                    uv_close(uv_handle, _callback_handle_closed ? callback_uv_close : nullptr);
                }
            }

            virtual ~Handle()
            {
            }

        private:
            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;

            Handle(Handle&&) = delete;
            Handle& operator=(Handle&&) = delete;
        };
    }
}

#endif
