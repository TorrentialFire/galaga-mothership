#ifndef _MAME_COMMS_SRV_H_
#define _MAME_COMMS_SRV_H_

#define MAME_COMMS_SERVER_PORT 2159
#define DEFAULT_BUFLEN 512

#define port_t uint16_t

#include <memory>
#include <thread>
#include <string>
#include <future>

/**
 * Instantiate the platform specific implementation of the comms_server.
 * 
 * // TODO - Everything needs to be limited to one instance of mame. ???
 * 
 * Spawn a thread, bind the socket, listen for connections, and spawn client threads.
*/

namespace mame {

    typedef struct server_config {
        port_t port;
    } server_config;

    class comms_server {
    public:
        virtual ~comms_server() = default;
        static std::unique_ptr<comms_server> init(const server_config &config);
        void start();

    protected:
        comms_server(const server_config &config) {
            _config = { .port = config.port };
        }
    
        
        virtual void bind_socket() = 0;
        virtual void listen_for_conn() = 0;
        virtual void read_from_mame_client() = 0;

        server_config _config;
        std::unique_ptr<std::jthread> _conn_thread;
        std::unique_ptr<std::jthread> _client_thread;
    private:

    };
}

#endif