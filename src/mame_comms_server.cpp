#include "mame_comms_server.hpp"

#include <iostream>
#include <exception>
#include <stdexcept>
#include <thread>
#include <stop_token>

using mame::comms_server;
using mame::server_config;

void comms_server::start() {
    try {
        this->bind_socket();

        // Kick off a thread to listen for incoming connections.
        _conn_thread = std::make_unique<std::jthread>(&comms_server::listen_for_conn, this);

    } catch (std::runtime_error &re) {
        std::cerr << re.what() << std::endl;
    }
}