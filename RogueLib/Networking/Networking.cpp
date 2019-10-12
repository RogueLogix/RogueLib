#include "Networking.hpp"

#include <RogueLib/Threading/WorkQueue.hpp>
#include <RogueLib/Threading/Thread.hpp>
#include <RogueLib/Exceptions/Exceptions.hpp>
#include <RogueLib/GenericBinary/BinaryTranslation.hpp>

#define USING_ROGUELIB_GENERICBINARY

#include <RogueLib/GenericBinary/AutoSerializable.hpp>
#include <shared_mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <utility>

namespace RogueLib::Networking {
    namespace asio = boost::asio;
    namespace ip = boost::asio::ip;

    class Channel::IMPL {
        const std::uint64_t id;
        std::shared_ptr<Connection::IMPL> connection;

        std::function<void(std::vector<std::uint8_t>)> dataCallback;
        Threading::WorkQueue incomingQueue;
        std::atomic_uint64_t threadsReady = 0;

        std::atomic_bool closed = false;
    public:

        IMPL(std::uint64_t id, std::shared_ptr<Connection::IMPL> connection);

        ~IMPL();

        void receiveData(std::vector<std::uint8_t> data);

        void setDataCallback(std::function<void(std::vector<std::uint8_t>)> function);

        void write(std::vector<std::uint8_t> data);

        void close();
    };

    static thread_local std::uint64_t socketNum = -1;

    enum ConnectionOps {
        CreateChannelID = 0,
        AttemptOpenChannel = 1,
        CloseChannel = 2,
        AddSocket = 3,
    };
    struct ConnectionConnectionRequest : public GenericBinary::AutoSerializable {
        ConnectionOps SERIALIZABLE(op);
        std::int64_t SERIALIZABLE(id) = 0;
        bool SERIALIZABLE(isResponse) = false;
        std::vector<uint8_t>SERIALIZABLE(data);
    };
    struct ChannelOpenDescription : public GenericBinary::AutoSerializable {
        std::vector<std::uint8_t>SERIALIZABLE(data);
        std::uint64_t SERIALIZABLE(id);

    };

    class Connection::IMPL {
        friend class Channel::IMPL;

        friend class Client::IMPL;

        friend class Server::IMPL;

        std::weak_ptr<Connection::IMPL> selfptr;

        const std::uint64_t id;    struct ChannelOpenDescription : public GenericBinary::AutoSerializable {
        std::vector<std::uint8_t>SERIALIZABLE(data);
        std::uint64_t SERIALIZABLE(id);

    };

        std::atomic_uint64_t nextSocketID = -1;
        std::atomic_uint64_t nextChannelID = 0;

        std::map<std::uint64_t, std::pair<std::condition_variable, ConnectionConnectionRequest*>*> waitingRequests;
        std::mutex waitingRequestsMutex;
        Threading::WorkQueue incomingRequestsQueue;

        std::vector<std::shared_ptr<ip::tcp::socket>> sockets;
        std::shared_mutex socketsMutex;

        std::map<std::uint64_t, std::weak_ptr<Channel::IMPL>> channels;
        std::shared_mutex channelsMutex;

        std::function<bool(std::vector<std::uint8_t> description, Channel channel)> channelRequestCallback;
        std::atomic_bool refuseChannelRequests;

        Threading::WorkQueue outgoingQueue;

        const bool server;

        void startReadThread(std::uint64_t id);

        void readThread(std::uint64_t id);

        void connectionRequest(std::vector<std::uint8_t> data);

        std::uint64_t sendConnectionRequest(ConnectionConnectionRequest request);

        ConnectionConnectionRequest waitForConnectionResponse(std::uint64_t requestID);

        std::uint64_t requestOrCreateChannelID();

        void closeChannel(std::uint64_t channelID);

    public:

        IMPL(std::uint64_t connectionID, std::shared_ptr<IMPL> client = {});

        void addSocket(std::shared_ptr<ip::tcp::socket> socket);

        void write(std::vector<std::uint8_t> data, std::uint64_t channel);

        Channel requestChannel(std::vector<std::uint8_t> connectionDescription);

        void
        setChannelRequestCallback(std::function<bool(std::vector<std::uint8_t> description, Channel channel)> function);
    };


    class Client::IMPL {

    };

}
namespace RogueLib::Networking {

    Channel::IMPL::IMPL(std::uint64_t id, std::shared_ptr<Connection::IMPL> connection) :
            id(id), connection(std::move(connection)) {
    }

    Channel::IMPL::~IMPL() {
        connection->closeChannel(id);
    }

    void Channel::IMPL::receiveData(std::vector<std::uint8_t> data) {
        ROGUELIB_STACKTRACE
        incomingQueue.enqueue(boost::bind<void>([](Channel::IMPL* impl, std::vector<std::uint8_t> data) {
            impl->threadsReady--;
            if (impl->threadsReady == 0) {
                impl->incomingQueue.addQueueThread();
            }
            impl->dataCallback(std::move(data));
            impl->threadsReady++;
        }, this, std::move(data)));
    }

    void Channel::IMPL::setDataCallback(std::function<void(std::vector<std::uint8_t>)> function) {
        ROGUELIB_STACKTRACE
        dataCallback = std::move(function);
        auto currentQueue = incomingQueue;
        incomingQueue = Threading::WorkQueue();
        threadsReady++;
        incomingQueue.addQueueThread();
        threadsReady++;
        currentQueue.addQueueThread();

        // yes this is implementation defined behavior, in this case it waits for all items to complete
        currentQueue.enqueue({}).wait();
    }

    void Channel::IMPL::write(std::vector<std::uint8_t> data) {
        ROGUELIB_STACKTRACE
        if(closed){
            throw Exceptions::InvalidState(ROGUELIB_EXCEPTION_INFO, "Channel closed");
        }
        connection->write(std::move(data), id);
    }

    void Channel::IMPL::close() {
        closed = true;
    }
}
namespace RogueLib::Networking {

    Connection::IMPL::IMPL(std::uint64_t connectionID, std::shared_ptr<IMPL> client)
            : server(!client), id(connectionID) {
        incomingRequestsQueue.addQueueThread();
    }

    void Connection::IMPL::startReadThread(std::uint64_t id) {
        ROGUELIB_STACKTRACE
        Threading::Thread(boost::bind<void>([](Connection::IMPL* impl, std::uint64_t id) {
            impl->readThread(id);
        }, this, id)).start();
    }

    void Connection::IMPL::readThread(std::uint64_t id) {
        ROGUELIB_STACKTRACE
        std::shared_ptr<ip::tcp::socket> socket;
        {
            std::shared_lock lk(socketsMutex);
            socket = sockets[socketNum];
        }

        std::vector<uint8_t> longBytes;
        longBytes.resize(9);

        while (true) {
            try {
                std::uint64_t channelID;
                {
                    auto* dataPtr = longBytes.data();
                    std::uint64_t leftToRead = 9;
                    while (leftToRead > 0) {
                        auto read = socket->read_some(asio::buffer(dataPtr, leftToRead));
                        leftToRead -= read;
                        dataPtr += read;
                    }
                    channelID = GenericBinary::fromBinary<std::uint64_t>(longBytes);
                }
                std::uint64_t length;
                {
                    auto* dataPtr = longBytes.data();
                    std::uint64_t leftToRead = 9;
                    while (leftToRead > 0) {
                        auto read = socket->read_some(asio::buffer(dataPtr, leftToRead));
                        leftToRead -= read;
                        dataPtr += read;
                    }
                    length = GenericBinary::fromBinary<std::uint64_t>(longBytes);
                }
                std::vector<std::uint8_t> data;
                data.resize(length);
                {
                    auto* dataPtr = data.data();
                    std::uint64_t leftToRead = data.size();
                    while (leftToRead > 0) {
                        auto read = socket->read_some(asio::buffer(dataPtr, leftToRead));
                        leftToRead -= read;
                        dataPtr += read;
                    }
                }
                if (channelID == -1) {
                    // ok, so this is from the other connection to this connection;
                    incomingRequestsQueue.enqueue(
                            boost::bind<void>([](Connection::IMPL* impl, std::vector<std::uint8_t> data) {
                                impl->connectionRequest(std::move(data));
                            }, this, std::move(data)));
                }
                auto iter = channels.find(channelID);
                if (iter == channels.end()) {
                    continue;
                }
                auto channel = iter->second.lock();
                if (!channel) {
                    channels.erase(channelID); // remove a dead channel
                    continue;
                }
                channel->receiveData(std::move(data));
            } catch (boost::system::system_error& e) {
                if (e.code() == asio::error::eof) {
                    break;
                }
            }
        }

    }

    void Connection::IMPL::addSocket(std::shared_ptr<ip::tcp::socket> socket) {
        ROGUELIB_STACKTRACE
        std::unique_lock lk(socketsMutex);
        sockets.emplace_back(socket);
        outgoingQueue.addQueueThread();
        startReadThread(sockets.size() - 1);
    }

    void Connection::IMPL::write(std::vector<std::uint8_t> data, std::uint64_t channel) {
        ROGUELIB_STACKTRACE
        outgoingQueue.enqueue(
                boost::bind<void>([](std::vector<std::uint8_t> data, std::uint64_t channel, IMPL* thi) {
                    ROGUELIB_STACKTRACE
                    if (socketNum == -1) {
                        socketNum = ++(thi->nextSocketID);
                        std::shared_lock lk(thi->socketsMutex);
                        if (socketNum >= thi->sockets.size()) {
                            // this really shouldn't be able to happen
                            throw Exceptions::InvalidState(ROGUELIB_EXCEPTION_INFO, "Socket ID out of range.");
                        }
                    }
                    std::shared_ptr<ip::tcp::socket> socket;
                    {
                        std::shared_lock lk(thi->socketsMutex);
                        socket = thi->sockets[socketNum];
                    }
                    {
                        auto channelBytes = GenericBinary::toBinary<std::uint64_t>(channel);
                        auto bytesLeft = 9; // it must be 9 bytes, that's literally how i wrote toBinary
                        auto* dataPtr = channelBytes.data();
                        while (bytesLeft > 0) {
                            auto bytesWritten = socket->write_some(asio::buffer(dataPtr, bytesLeft));
                            bytesLeft -= bytesWritten;
                            dataPtr += bytesWritten;
                        }
                    }
                    {
                        auto lengthBytes = GenericBinary::toBinary<std::uint64_t>(data.size());
                        auto bytesLeft = 9; // it must be 9 bytes, that's literally how i wrote toBinary
                        auto* dataPtr = lengthBytes.data();
                        while (bytesLeft > 0) {
                            auto bytesWritten = socket->write_some(asio::buffer(dataPtr, bytesLeft));
                            bytesLeft -= bytesWritten;
                            dataPtr += bytesWritten;
                        }
                    }
                    {
                        auto bytesLeft = data.size();
                        auto* dataPtr = data.data();
                        while (bytesLeft > 0) {
                            auto bytesWritten = socket->write_some(asio::buffer(dataPtr, bytesLeft));
                            bytesLeft -= bytesWritten;
                            dataPtr += bytesWritten;
                        }
                    }
                }, std::move(data), channel, this));
    }

    Channel Connection::IMPL::requestChannel(std::vector<std::uint8_t> connectionDescription) {
        ROGUELIB_STACKTRACE
        Channel channel;

        std::uint64_t id = requestOrCreateChannelID();

        channel.impl = std::make_shared<Channel::IMPL>(id, selfptr.lock());

        channels[id] = channel.impl;

        ChannelOpenDescription description;
        description.data = connectionDescription;
        description.id = id;

        ConnectionConnectionRequest request;
        request.op = AttemptOpenChannel;
        request.data = GenericBinary::toBinary(description);

        auto response = waitForConnectionResponse(sendConnectionRequest(request));

        auto opened = GenericBinary::fromBinary<bool>(response.data);

        if (!opened) {
            throw Exceptions::InvalidState(ROGUELIB_EXCEPTION_INFO, "Failed to open channel");
        }

        return channel;

    }

    void Connection::IMPL::setChannelRequestCallback(
            std::function<bool(std::vector<std::uint8_t> description, Channel channel)> function) {
        ROGUELIB_STACKTRACE

    }

    void Connection::IMPL::connectionRequest(std::vector<std::uint8_t> data) {
        auto request = GenericBinary::fromBinary<ConnectionConnectionRequest>(data);
        if (request.isResponse) {
            std::unique_lock lk(waitingRequestsMutex);
            auto iter = waitingRequests.find(request.id);
            if (iter != waitingRequests.end()) {
                *(iter->second->second) = request;
                iter->second->first.notify_all();
            }
        } else {
            switch (request.op) {
                case CreateChannelID: {
                    // only the server needs to care
                    // client should never get this request, and if it does its probably an error
                    if (server) {
                        ConnectionConnectionRequest returnRequest;
                        returnRequest.op = CreateChannelID;
                        returnRequest.id = request.id;
                        returnRequest.isResponse = true;
                        std::vector<std::uint64_t> ids;
                        ids.emplace_back(requestOrCreateChannelID());
                        returnRequest.data = GenericBinary::toBinary(ids);
                        sendConnectionRequest(request);
                    }
                    break;
                }
                case AttemptOpenChannel: {
                    ConnectionConnectionRequest response;
                    response.op = AttemptOpenChannel;
                    response.id = request.id;
                    response.isResponse = true;
                    while (!channelRequestCallback && !refuseChannelRequests) {
                        // yup, its a spinlock, this shouldn't hold for very long anyway
                    }
                    bool openedSucessfully = false;
                    auto openRequest = GenericBinary::fromBinary<ChannelOpenDescription>(request.data);
                    Channel channel;
                    channel.impl = std::make_shared<Channel::IMPL>(openRequest.id, selfptr.lock());
                    if (channelRequestCallback && !refuseChannelRequests) {
                        openedSucessfully = channelRequestCallback(openRequest.data, channel);
                    }
                    std::unique_lock lk(channelsMutex);
                    response.data = GenericBinary::toBinary(openedSucessfully);
                    if (openedSucessfully) {
                        channels[openRequest.id] = channel.impl;
                    }
                    sendConnectionRequest(response);
                    break;
                }
                case CloseChannel: {
                    std::unique_lock lk(channelsMutex);
                    auto closeID = GenericBinary::fromBinary<std::uint64_t>(request.data);
                    auto channelsIter = channels.find(closeID);
                    if(channelsIter != channels.end()){
                        auto channelPtr = channelsIter->second.lock();
                        channels.erase(closeID);
                        if(channelPtr){
                            channelPtr->close();
                        }
                    }
                    break;
                }
                case AddSocket: {

                    break;
                }
            }
        }
    }

    std::uint64_t Connection::IMPL::sendConnectionRequest(ConnectionConnectionRequest request) {
        static std::atomic_uint64_t nextRequestID = 1;
        if (request.id == 0) {
            request.id = nextRequestID++;
        }
        write(GenericBinary::toBinary(request), 0);
        return request.id;
    }

    ConnectionConnectionRequest Connection::IMPL::waitForConnectionResponse(std::uint64_t requestID) {
        std::unique_lock lk(waitingRequestsMutex);
        std::pair<std::condition_variable, ConnectionConnectionRequest*> pair;
        waitingRequests[requestID] = &pair;
        ConnectionConnectionRequest toReturn;
        pair.second = &toReturn;
        pair.first.wait(lk);
        waitingRequests.erase(requestID);
        return toReturn;
    }

    std::uint64_t Connection::IMPL::requestOrCreateChannelID() {
        if (server) {
            return ++nextChannelID;
        } else {
            ConnectionConnectionRequest request;
            request.op = CreateChannelID;
            request.data = GenericBinary::toBinary<std::uint64_t>(1);

            auto response = waitForConnectionResponse(sendConnectionRequest(request));

            auto ids = GenericBinary::fromBinary<std::vector<std::uint64_t>>(response.data);

            if (ids.empty()) {
                return 0;
            }

            return ids[0];
        }
    }

    void Connection::IMPL::closeChannel(std::uint64_t channelID) {
        ConnectionConnectionRequest closeRequest;
        closeRequest.op = CloseChannel;
        closeRequest.data = GenericBinary::toBinary(channelID);
        connectionRequest(GenericBinary::toBinary(closeRequest));
        sendConnectionRequest(closeRequest);
    }

}

namespace RogueLib::Networking {
    void Channel::write(std::vector<std::uint8_t> data) {
        ROGUELIB_STACKTRACE
        impl->write(std::move(data));
    }

    void Channel::setDataCallback(std::function<void(std::vector<std::uint8_t>)> function) {
        ROGUELIB_STACKTRACE
        impl->setDataCallback(std::move(function));
    }

    Channel Connection::requestChannel(std::vector<std::uint8_t> connectionDescription) {
        ROGUELIB_STACKTRACE
        return impl->requestChannel(std::move(connectionDescription));
    }

    void Connection::setChannelRequestCallback(
            std::function<bool(std::vector<std::uint8_t> description, Channel channel)> function) {
        ROGUELIB_STACKTRACE
        impl->setChannelRequestCallback(std::move(function));
    }
}