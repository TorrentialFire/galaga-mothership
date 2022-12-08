#ifndef _WIN32_MAME_COMMS_SRV_H_
#define _WIN32_MAME_COMMS_SRV_H_

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include "mame_comms_server.hpp"

using mame::server_config;
using mame::comms_server;

namespace mame {

    typedef struct addrinfo addrinfo_t;

    class win32_mame_comms_server : public comms_server {
    public:
        win32_mame_comms_server(const server_config &config) : comms_server(config) {
            _svr_sock = INVALID_SOCKET;
        };
        ~win32_mame_comms_server() override {
            _conn_thread->request_stop();       // Request that the connection thread terminate.
            _conn_thread->join();               // Join the thread and wait for it to exit.
            if (_svr_sock != INVALID_SOCKET) {  // Cleanup socket
                closesocket(_svr_sock);
            }

            _client_thread->request_stop();
            _client_thread->join();
            if (_c_sock != INVALID_SOCKET) {
                closesocket(_c_sock);
            }
            WSACleanup();
        }

        void bind_socket() override;
        void listen_for_conn() override;
        void read_from_mame_client() override;
    private:
        SOCKET _svr_sock;
        SOCKET _c_sock;
    };
}

#endif