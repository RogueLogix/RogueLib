#pragma  once

#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace RogueLib::Networking {

    class Server;

    enum class SendFlags : std::uint16_t {
        UNDEFINED = 0u,
        NO_PREFERENCES = UNDEFINED,
        USE_DEFAULTS = UNDEFINED,
        LATENCY = 1u,
        THROUGHPUT = 1u << 1u,
        RDMA = 1u << 2u,
        BUFFERLESS = RDMA,
        LOSSY = 1u << 3u,
        COMPRESS = 1u << 4u,
        NO_ENCRYPT = 1u << 5u,
        ENCRYPT = 1u << 6u,
        SPLIT = 1u << 7u,
        NO_SPLIT = 1u << 8u,
        RESERVED = static_cast<std::uint16_t>(std::uint16_t(-1) << 8u)
    };

    class Channel {
        friend class Connection;

        friend class Server;

        class IMPL;

        std::shared_ptr<IMPL> impl;

    public:
        void send(std::vector<std::uint8_t>& data, SendFlags flags);

        void send(std::vector<std::uint8_t>& data) {
            send(data, defaultFlags());
        }

        /**
         * function is called when data is received with each data block
         * may be called in parallel, not called from system thread
         * may throw exception back, but it will be ignored
         * holding the thread is fine, callback handler will spawn a new one as needed
         */
        virtual void setDataCallback(std::function<void(std::vector<std::uint8_t>)> function);

        void setDefaultSendFlags(SendFlags defaultFlags);

        SendFlags defaultFlags();
    };

    // todo
    class SyncChannel {
    public:
        std::vector<std::uint8_t> write(std::vector<std::uint8_t> data);

        void setDataCallback(std::function<std::vector<std::uint8_t>(std::vector<std::uint8_t>)> function);
    };

    struct Endpoint {
        // all addresses and ports are treated as a single endpoint
        // will be tried until one connects
        // domains may not contain ports
        std::vector<std::uint32_t> ipv4Addresses;
        std::vector<__int128> ipv6Addresses;
        // you can pass an ipv4 or ipv6 address as a string here, thats half of what its for
        std::vector<std::string> domains;
        std::vector<std::uint16_t> ports;
        std::vector<std::string> services;

        inline bool operator==(const Endpoint& other) const {
            // size checks
            if (
                    ipv4Addresses.size() != other.ipv4Addresses.size() ||
                    ipv6Addresses.size() != other.ipv6Addresses.size() ||
                    domains.size() != other.domains.size() ||
                    ports.size() != other.ports.size() ||
                    services.size() != other.services.size()
                    ) {
                return false;
            }

            // aight, content check time
            for (std::size_t i = 0; i < ipv4Addresses.size(); ++i) {
                if (ipv4Addresses[i] != other.ipv4Addresses[i]) {
                    return false;
                }
            }

            for (std::size_t i = 0; i < ipv6Addresses.size(); ++i) {
                if (ipv6Addresses[i] != other.ipv6Addresses[i]) {
                    return false;
                }
            }

            for (std::size_t i = 0; i < domains.size(); ++i) {
                if (domains[i] != other.domains[i]) {
                    return false;
                }
            }

            for (std::size_t i = 0; i < ports.size(); ++i) {
                if (ports[i] != other.ports[i]) {
                    return false;
                }
            }

            for (std::size_t i = 0; i < services.size(); ++i) {
                if (services[i] != other.services[i]) {
                    return false;
                }
            }

            // ok, everything is equal
            return true;
        }

        Endpoint(){
        }

        Endpoint(std::vector<std::string> urls);

        [[nodiscard]] std::vector<std::uint8_t> serialize() const;

        void deserialize(std::vector<std::uint8_t> bytes);
    };

    class Connection {
        friend class Channel;

        friend class Server;

        class IMPL;

        std::shared_ptr<IMPL> impl;

        Connection(Server* server);

        Connection(Endpoint initialRemoteEndpoint, Server* server);

    public:

        Connection() {
        }

        Connection(Endpoint initialRemoteEndpoint, std::vector<Endpoint> localEndpoints = {});

        Connection(Endpoint initialRemoteEndpoint, std::uint64_t sockets, std::vector<Endpoint> localEndpoints = {});

        Connection(const Connection& connection) {
            this->impl = connection.impl;
        }


        /**
         *
         * to avoid the channel reconfiguring its connections it is recommended to configure it before making/allowing
         * channels requests, which activates the connectoin
         *
         */
        Channel requestChannel(std::vector<std::uint8_t> connectionDescription);

        /**
         * ok to lock thread
         * @param function
         */
        void setChannelRequestCallback(std::function<void(std::vector<std::uint8_t>, Channel)> function);

        // todo, these modes are all just preference anyway, but nothing is implemented

        enum class OperationMode : std::uint64_t {
            NO_PREFERENCE = 0,
            LATENCY = 1,
            THROUGHPUT = 2,
        };

        void setLocalOpMode(OperationMode mode);

        void requestRemoteOpMode(OperationMode mode);

        void allowOtherSideToDecideOpMode(bool defferDecision, OperationMode localPreference);

        enum class NumaMode : std::uint64_t {
            NO_PREFERENCE = 0,
            USE_ALL_NODES = 1,
            STAY_IN_NODE = 2,
            FORCE_NEW_NODE = 3,
            SINGLE_NODE = 4,
            SINGLE_NODE_SEPARATE_CALLBACKS = 5
        };

        void setNUMAMode(NumaMode mode);

        void shouldLoadBalanceSockets(bool doLoadBalance);

        void setMaxSockets(std::uint64_t maxSockets);

        void setMaxSocketsPerEndpoint(std::uint64_t maxSockets);

        /**
         * by default, up to two are maintained to/from different endpoints
         * unless you need absolute reliability, you can just leave this alone
         * @param maxSockets
         */
        void setControlSocketCount(std::uint64_t maxSockets);
    };

    class Server {
        friend class Channel;

        friend class Connection;

        class IMPL;

        std::shared_ptr<IMPL> impl;

    public:
        void setNewConnectionHandler(std::function<void(Connection)> function);

        void bindEndpoint(Endpoint endpoint);

        void unbindEndpoint(Endpoint endpoint);

        void addClientEndpoint(Endpoint endpoint, std::uint64_t weight);

        Connection connect(Endpoint initialEndpoint);

//        void connectLoadBalancer(Endpoint loadBalancer);
    };

    /**
     * Variables used to query the capabilities of the linked library
     * extern variables used as this can change if using dynamic linking
     */
    namespace Capabilities{
        // defined in ./IMPL/Infiniband.cpp
        extern const bool infinibandEnabled;
        extern const bool sharedMemoryEnabled;
    }
}