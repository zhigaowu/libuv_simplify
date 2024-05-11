#ifndef IO_SIMPLIFY_LIBUV_LOOP_H
#define IO_SIMPLIFY_LIBUV_LOOP_H

#include "libuv_base.h"

namespace io_simplify {

    namespace libuv {

        class Loop : public Base<uv_loop_t>
        {
        public:
            Loop()
                : Base<uv_loop_t>()
            {
                Base<uv_loop_t>::status = uv_loop_init(Base<uv_loop_t>::uv);
            }

            ~Loop()
            {
                uv_loop_close(Base<uv_loop_t>::uv);
            }

            int Run(uv_run_mode mode = UV_RUN_DEFAULT)
            {
                return uv_run(Base<uv_loop_t>::uv, mode);
            }

            void Stop()
            {
                uv_stop(Base<uv_loop_t>::uv);
            }

        private:
            Loop(const Loop&) = delete;
            Loop& operator=(const Loop&) = delete;

            Loop(Loop&&) = delete;
            Loop& operator=(Loop&&) = delete;
        };
    }
}

#endif
