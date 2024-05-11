#ifndef IO_SIMPLIFY_LIBUV_MUTEX_H
#define IO_SIMPLIFY_LIBUV_MUTEX_H

#include "libuv_base.h"

#include <functional>
#include <list>

namespace io_simplify {

    namespace libuv {

        class Mutex : public Base<uv_mutex_t>
        {
        public:
            Mutex()
                : Base<uv_mutex_t>()
            {
                Base<uv_mutex_t>::status = uv_mutex_init(Base<uv_mutex_t>::uv);
            }

            void Lock()
            {
                uv_mutex_lock(Base<uv_mutex_t>::uv);
            }

            void Unlock()
            {
                uv_mutex_unlock(Base<uv_mutex_t>::uv);
            }

            ~Mutex()
            {
                if (0 == Base<uv_mutex_t>::status)
                {
                    uv_mutex_destroy(Base<uv_mutex_t>::uv);
                }
            }

        private:
            Mutex(const Mutex&) = delete;
            Mutex& operator=(const Mutex&) = delete;

            Mutex(Mutex&&) = delete;
            Mutex& operator=(Mutex&&) = delete;
        };
    }
}

#endif
