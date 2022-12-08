#include "win32_mame_comms_server.hpp"

#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

using mame::server_config;
using mame::comms_server;
using mame::win32_mame_comms_server;

/**
 * Construct a windows platform specific mame comms server.
*/
std::unique_ptr<comms_server> comms_server::init(const server_config &config) {
    return std::make_unique<win32_mame_comms_server>(config);
}

void win32_mame_comms_server::bind_socket() {
    addrinfo_t *sock_info = NULL;
    addrinfo_t hints = {0};
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int get_addr_info_result = getaddrinfo(
        NULL, 
        std::to_string(this->_config.port).c_str(), 
        &hints, 
        &sock_info
    );
    if (get_addr_info_result != 0) {
        // TODO - Logging
        freeaddrinfo(sock_info);
        auto err_msg = std::string("getaddrinfo failed with " + std::to_string(get_addr_info_result));
        throw std::runtime_error(err_msg);
    }

    _svr_sock = socket(
        sock_info->ai_family,
        sock_info->ai_socktype,
        sock_info->ai_protocol  
    );
    if (_svr_sock == INVALID_SOCKET) {
        // TODO - Logging
        freeaddrinfo(sock_info);
        auto err_msg = std::string("Error creating socket; socket couldn't be created. WSAGetLastError(): " + std::to_string(WSAGetLastError()) + ".\n");
        throw std::runtime_error(err_msg);
    }

    int bind_result = bind(
        _svr_sock, 
        sock_info->ai_addr, 
        (int)(sock_info->ai_addrlen)
    );
    if (bind_result == SOCKET_ERROR) {
        freeaddrinfo(sock_info);
        closesocket(_svr_sock);
        auto err_msg = std::string("Error binding socket; socket couldn't be bound. WSAGetLastError(): " + std::to_string(WSAGetLastError()));
        throw std::runtime_error(err_msg);
    }

    freeaddrinfo(sock_info);
}

void win32_mame_comms_server::listen_for_conn() {
    try {
        if (listen(_svr_sock, 1) == SOCKET_ERROR) {
            auto err_msg = std::string("Failed to listen on socket! WSAGetLastError(): " + std::to_string(WSAGetLastError()));
            throw std::runtime_error(err_msg);
        }

        //while (!stopper.stop_requested()) {
        SOCKET c_sock = accept(_svr_sock, NULL, NULL);

        if (c_sock == INVALID_SOCKET) {
            auto err_msg = std::string("Incoming connection failed! WSAGetLastError(): " + std::to_string(WSAGetLastError()));
            throw std::runtime_error(err_msg);
        }

        // For now, just set a class member and kick off the recv thread.
        // TODO - handle mame client connection(s)?
        this->_c_sock = c_sock;
        _client_thread = std::make_unique<std::jthread>(&win32_mame_comms_server::read_from_mame_client, this);
        
        //}
    } catch (std::runtime_error &re) {
        std::cerr << re.what() << std::endl;
    }
}

void win32_mame_comms_server::read_from_mame_client() {
    const int recv_buf_len = DEFAULT_BUFLEN - 1;
    char recv_buf[DEFAULT_BUFLEN];
    int recv_result;

    do {
        recv_result = recv(this->_c_sock, recv_buf, recv_buf_len, 0);
        if (recv_result > 0) {
            recv_buf[recv_result] = '\0'; // null terminate the buffer at the number of bytes read on receive
            std::cout << "[Server]: " << recv_buf << "\n";
        } else if (recv_result == 0) {
            std::cout << "Connection closed.\n";
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
        }
    } while (recv_result > 0);
}