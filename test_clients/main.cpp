
#include "libuv_tcp_handle.h"
#include "libuv_udp_handle.h"

#include "libuv_async_handle.h"

#include <thread>

#include <string>
#include <iostream>

class UserThread
{
private:
    io_simplify::libuv::Loop* _loop;
    io_simplify::libuv::AsyncHandle* _async;
    io_simplify::Endpoint _endpoint;

    std::thread _thread;

    void process_user_input()
    {
        std::string user_cmd;

        io_simplify::libuv::TcpHandle tcp_handle(_loop);
        io_simplify::libuv::UdpHandle udp_handle(_loop);

        uv_udp_send_t udp_send_req;
        uv_write_t tcp_write_req;
        uv_connect_t tcp_conn_req;
            
        do
        {
            std::cout << "please input command:" << std::endl;

            std::cin >> user_cmd;

            if (user_cmd == "quit")
            {
                _async->Async([this, &udp_handle, &tcp_handle]() {
                    udp_handle.AsyncClose();
                    tcp_handle.AsyncClose();
                    _async->AsyncClose();
                    _loop->Stop();

                    });
                
                break;
            }

            if (user_cmd.size() > 6)
            {
                if(user_cmd.substr(0, 5) == "usend")
                {
                    //uv_udp_send_t* udp_send_req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
                    _async->Async([&udp_handle, &udp_send_req, &user_cmd]() {
                        uv_buf_t buf = uv_buf_init(user_cmd.data() + 6, user_cmd.size() - 6);
                        int res = udp_handle.Send(&udp_send_req, &buf, 1, [] (uv_udp_send_t* req, int status) {
                            std::cout << "udp send status: " << uv_strerror(status) << "(" << status << ")" << std::endl;
                            //free(req);
                        });

                        if (res < 0)
                        {
                            std::cout << "udp send failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                        }
                    });
                    continue;
                }

                if(user_cmd.substr(0, 5) == "tsend")
                {
                    _async->Async([&tcp_handle, &tcp_write_req, &user_cmd]() {
                        uv_buf_t buf = uv_buf_init(user_cmd.data() + 6, user_cmd.size() - 6);
                        tcp_handle.Write(&tcp_write_req, &buf, 1, [] (uv_write_t* req, int status) {
                            std::cout << "tcp write status: " << uv_strerror(status) << "(" << status << ")" << std::endl;
                        });
                    });
                    continue;
                }
            }

            if (user_cmd == "connect.udp.server")
            {
                int res = udp_handle.Connect(_endpoint);
                if (res < 0)
                {
                    std::cout << "udp_handle connect failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                }
                
                continue;
            }

            if (user_cmd == "connect.tcp.server")
            {
                int res = tcp_handle.Connect(&tcp_conn_req, _endpoint, [] (uv_connect_t* req, int status) {
                    if (status < 0)
                    {
                        std::cout << "callback, tcp_handle connect failed: " << uv_strerror(status) << "(" << status << ")" << std::endl;
                    }
                });
                if (res < 0)
                {
                    std::cout << "tcp_handle connect failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                }
                
                continue;
            }

            std::cout << "unrecognized command: " << user_cmd << std::endl;
        } while (true);
    }

public:
    UserThread(io_simplify::libuv::Loop* loop, io_simplify::libuv::AsyncHandle* async, const io_simplify::Endpoint& endpoint)
        : _loop(loop)
        , _async(async)
        , _endpoint(endpoint)
        , _thread(std::thread(&UserThread::process_user_input, this))
    {
    }

    ~UserThread()
    {
        _thread.join();
    }
};

int main(int argc, char** argv) 
{
    io_simplify::libuv::Loop loop;

    io_simplify::libuv::AsyncHandle async_handle(&loop);

    int res = 0;
    do
    {
        if (argc < 3)
        {
            std::cout << "usage: \n     " << argv[0] << " ip port" << std::endl;
            break;
        }

        io_simplify::Endpoint endpoint {argv[1], (uint16_t)atoi(argv[2])};

        UserThread user_thread(&loop, &async_handle, endpoint);

        res = loop.Run();
        
    } while (false);
    
    std::cout << "main exit: " << uv_strerror(res) << "(" << res << ")" << std::endl;

    return res;
}
