#pragma  once

#include <memory>
#include <vector>
#include <functional>

namespace RogueLib::Networking {

    class Channel {
        friend class Connection;
        friend class Client;
        friend class Server;
        class IMPL;

        std::shared_ptr<IMPL> impl;

    public:
        void write(std::vector<std::uint8_t> data);

        /**
         * function is called when data is received with each data block
         * may be called in parallel, not called from system thread
         * may throw exception back, but it will be ignored
         * holding the thread is fine, callback handler will spawn a new one as needed
         */
        void setDataCallback(std::function<void(std::vector<std::uint8_t>)> function);

        /**
         * should the callback only be called on one thread
         * default
         */
        void singleThreadCallback(bool value);
    };

    class SyncChannel{
        class IMPL;
    public:
        std::vector<std::uint8_t> write(std::vector<std::uint8_t> data);

        void setDataCallback(std::function<std::vector<std::uint8_t>(std::vector<std::uint8_t>)> function);
    };

    class Connection {
        friend class Channel;
        friend class Client;
        friend class Server;
        class IMPL;

        std::shared_ptr<IMPL> impl;

    public:
        Connection(Connection& connection){
            this->impl = connection.impl;
        }

        Channel requestChannel(std::vector<std::uint8_t> connectionDescription);

        void setChannelRequestCallback(std::function<bool(std::vector<std::uint8_t> description, Channel channel)> function);
    };

    class Client {
        friend class Channel;
        friend class Connection;
        friend class Server;
        class IMPL;

        std::shared_ptr<IMPL> impl;
    public:
        Client(std::uint32_t ipv4, std::uint16_t port);

        Client(__int128 ipv6, std::uint16_t port);

        Client(std::string domain, std::uint16_t port);

        Client(std::string domain);

        Connection connect();
    };

    class Server {
        friend class Channel;
        friend class Connection;
        friend class Client;
        class IMPL;

        std::shared_ptr<IMPL> impl;

    public:
        explicit Server(std::uint16_t port);

        explicit Server(std::uint32_t ipv4Listen, std::uint16_t port);

        explicit Server(__int128 ipv6Listen, std::uint16_t port);

        Server(std::string domain, std::uint16_t port);

        Server(std::string domain);

        // todo, more constructors for multi-ip listening

        void setNewConnectionHandler(std::function<void(Connection connection)> function);
    };
}