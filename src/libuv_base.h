#ifndef IO_SIMPLIFY_LIBUV_BASE_H
#define IO_SIMPLIFY_LIBUV_BASE_H

#include <uv.h>

#include <stdlib.h>

namespace io_simplify {

    namespace libuv {

        template<typename uv_object_type>
        class Base
        {
            uv_object_type _object;

        public:
            using uv_object_t = uv_object_type;

        public:
            uv_object_t* uv;
            int status;

        public:
            Base()
                : _object()
                , uv(&_object)
                , status(0)
            {
            }

            virtual ~Base()
            {
                /*if (uv)
                {
                    free(uv);

                    uv = nullptr;
                }*/
            }

        private:
            Base(const Base&) = delete;
            Base& operator=(const Base&) = delete;

            Base(Base&&) = delete;
            Base& operator=(Base&&) = delete;
        };
    }
}



#endif
