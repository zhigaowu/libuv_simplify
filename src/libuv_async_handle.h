#ifndef IO_SIMPLIFY_LIBUV_ASYNC_HANDLE_H
#define IO_SIMPLIFY_LIBUV_ASYNC_HANDLE_H

#include "libuv_loop.h"

#include "libuv_handle.h"
#include "libuv_mutex.h"

#include <list>

namespace io_simplify {

    namespace libuv {

        class AsyncHandle : public Handle<uv_async_t>
        {
        public:
            using CallbackAsync = std::function<void()>;
            using CallbackAsyncList = std::list<CallbackAsync>;

        private:
            Mutex _mutex;
            CallbackAsyncList _callback_async_list;

        private:
            void getCallbackAsyncList(CallbackAsyncList& callback_async_list)
            {
                _mutex.Lock();
                callback_async_list.swap(_callback_async_list);
                _mutex.Unlock();
            }

            static void callback_uv_async(uv_async_t* handle)
            {
                AsyncHandle* server_handle = (AsyncHandle*)(handle->data);

                CallbackAsyncList callback_async_list;
                server_handle->getCallbackAsyncList(callback_async_list);

                for (CallbackAsync& callback_async : callback_async_list)
                {
                    callback_async();
                }
            }

        public:
            AsyncHandle(Loop* loop)
                : Handle<uv_async_t>(loop)

                , _mutex()
                , _callback_async_list()
            {
                Handle<uv_async_t>::status = uv_async_init(loop->uv, Handle<uv_async_t>::uv, callback_uv_async);
            }

            int Async(const CallbackAsync& callback_async)
            {
                _mutex.Lock();
                _callback_async_list.push_back(callback_async);
                _mutex.Unlock();

                return uv_async_send(Handle<uv_async_t>::uv);
            }

            ~AsyncHandle()
            {
            }

        private:
            AsyncHandle() = delete;

            AsyncHandle(const AsyncHandle&) = delete;
            AsyncHandle& operator=(const AsyncHandle&) = delete;

            AsyncHandle(AsyncHandle&&) = delete;
            AsyncHandle& operator=(AsyncHandle&&) = delete;
        };
    }
}

#endif
