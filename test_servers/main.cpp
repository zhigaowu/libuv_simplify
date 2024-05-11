
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

    void run_tcp_server(io_simplify::libuv::TcpHandle& tcp_handle) 
    {
        int res = tcp_handle.Listen([&tcp_handle] (int status) 
        {
            std::cout << "connection arrived: " << uv_strerror(status) << "(" << status << ")" << std::endl;

            if (0 == status)
            {
                io_simplify::libuv::TcpHandle* client_handle = new io_simplify::libuv::TcpHandle(tcp_handle.loop);
                if (client_handle)
                {
                    int res = tcp_handle.Accept(client_handle);
                    if (res >= 0)
                    {
                        io_simplify::Endpoint endpoint;

                        client_handle->GetEndpoint(endpoint);

                        std::cout << "connection accept: " << endpoint.address << ":" << endpoint.port << std::endl;

                        uv_buf_t read_buf = uv_buf_init((char*)malloc(128), 128);
                        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));

                        client_handle->StartRead(
                            [client_handle, endpoint, read_buf, req] (ssize_t nread, const uv_buf_t *buf) {
                                if (nread > 0)
                                {
                                    std::cout << "data received: [" << std::string(buf->base, buf->base + nread) << "] from: " << endpoint.address << ":" << endpoint.port << std::endl;

                                    uv_buf_t write_buf = uv_buf_init("hello", 5);
                                    client_handle->Write(req, &write_buf, 1, [](uv_write_t* req, int status) {
                                        std::cout << "write status: " << uv_strerror(status) << "(" << status << ")" << std::endl;
                                    });
                                }
                                else if (nread < 0)
                                {
                                    std::cout << "data read failed: " << uv_strerror(nread) << "(" << nread << ")" << std::endl;

                                    client_handle->StopRead();
                                    client_handle->Close([client_handle, read_buf, req] () {
                                        free(req);
                                        free(read_buf.base);

                                        delete client_handle;
                                    });
                                }
                            },
                            [read_buf] (size_t suggested_size, uv_buf_t* buff) {
                                buff->base = read_buf.base;
                                buff->len = read_buf.len;
                            });
                    }
                    else
                    {
                        std::cout << "connection accept failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                    }
                }
            }
        });
        
        if (res < 0)
        {
            std::cout << "tcp_handle.Listen faield: " << uv_strerror(res) << "(" << res << ")" << std::endl;
        }
    }

    void run_udp_server(io_simplify::libuv::UdpHandle& udp_handle) 
    {
        uv_buf_t read_buf = uv_buf_init((char*)malloc(128), 128);
        uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));

        int res = udp_handle.StartReceive(
        [&udp_handle, read_buf, req] (ssize_t nread, const uv_buf_t *buf, const struct sockaddr* sa_addr, unsigned int flags) {
            if (nread > 0)
            {
                const struct sockaddr_in* sa_addr_in = (const struct sockaddr_in*)sa_addr;
                char address[32] = {0};
                uv_ip4_name(sa_addr_in, address, sizeof(address));

                std::cout << "data received: [" << std::string(buf->base, buf->base + nread) << "] from: " << address << ":" << ntohs(sa_addr_in->sin_port) << std::endl;

                uv_buf_t write_buf = uv_buf_init("hello", 5);
                udp_handle.Send(req, &write_buf, 1, sa_addr, [](uv_udp_send_t* req, int status) {
                    std::cout << "send status: " << uv_strerror(status) << "(" << status << ")" << std::endl;
                });
            }
            else if (nread < 0)
            {
                std::cout << "data received failed: " << uv_strerror(nread) << "(" << nread << ")" << std::endl;

                udp_handle.StopReceive();
                udp_handle.AsyncClose([read_buf, req] () {
                    free(req);
                    free(read_buf.base);
                });
            }
        },
        [read_buf] (size_t suggested_size, uv_buf_t* buff) {
            buff->base = read_buf.base;
            buff->len = read_buf.len;
        });

        if (res < 0)
        {
            std::cout << "udp_handle.StartReceive faield: " << uv_strerror(res) << "(" << res << ")" << std::endl;
        }
    }


    void process_user_input()
    {
        std::string user_cmd;

        io_simplify::libuv::TcpHandle tcp_handle(_loop);
        io_simplify::libuv::UdpHandle udp_handle(_loop);
            
        do
        {
            std::cout << "please input command:" << std::endl;

            std::cin >> user_cmd;

            if (user_cmd == "quit")
            {
                _async->Async([this, &udp_handle, &tcp_handle]() {
                    udp_handle.Close();
                    tcp_handle.Close();
                    _async->Close();
                    _loop->Stop();

                    });
                
                break;
            }

            if (user_cmd == "run.udp.server")
            {
                int res = udp_handle.Bind(_endpoint);
                if (res < 0)
                {
                    std::cout << "udp_handle bind failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                    continue;
                }

                _async->Async([this, &udp_handle]() {
                    run_udp_server(udp_handle);
                    });
                
                continue;
            }

            if (user_cmd == "run.tcp.server")
            {
                int res = tcp_handle.Bind(_endpoint);
                if (res < 0)
                {
                    std::cout << "tcp_handle bind failed: " << uv_strerror(res) << "(" << res << ")" << std::endl;
                    continue;
                }

                _async->Async([this, &tcp_handle]() {
                    run_tcp_server(tcp_handle);
                    });
                
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
