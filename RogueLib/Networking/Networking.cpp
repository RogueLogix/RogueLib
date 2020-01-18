#include "Networking.hpp"

#include <RogueLib/Threading/WorkQueue.hpp>
#include <RogueLib/Threading/Thread.hpp>
#include <RogueLib/Exceptions/Exceptions.hpp>
#include <RogueLib/ROBN/ROBNTranslation.hpp>

#define USING_ROGUELIB_ROBN

#include <RogueLib/ROBN/AutoSerializable.hpp>
#include <RogueLib/Threading/cpponmulticore/autoresetevent.h>
#include <shared_mutex>
#include <condition_variable>
#include <queue>

#include <boost/asio.hpp>
#include <utility>
#include <boost/asio/ip/tcp.hpp>

#include <infiniband/verbs.h>

namespace RogueLib::Networking {
    namespace Transmission {
        class Socket {
        public:
            class IMPL;

        private:
            std::shared_ptr<IMPL> impl;

        public:
            enum class UnderlyingType {
                TCP_IP = 1,
//                INFINIBAND_RDMA = 2,
//                LINUX_SHARED_MEMORY = 3,
            };

            UnderlyingType underlyingType();

            Endpoint connectedEndpoint();

            bool connect(Endpoint endpoint);

            void isOpen();

            void send(std::vector<std::uint8_t> data);

            void setDataCallback(std::function<std::vector<std::uint8_t>&(std::uint64_t size)> function);
        };

        class Socket::IMPL {
        public:
            virtual void send(std::vector<std::uint8_t> data) = 0;

            virtual void setDataCallback(std::function<std::vector<std::uint8_t>&(std::uint64_t size)> function) = 0;

            virtual  ~IMPL() = 0;
        };

        class TCPSocket : public Socket::IMPL {
            std::unique_ptr<boost::asio::ip::tcp::socket> socket;
            Threading::Thread readThread;
            std::function<std::vector<std::uint8_t>&(std::uint64_t size)> getVectorCallback;
            std::function<void(std::vector<std::uint8_t>)> readDonCallback;
            Threading::AutoResetEvent hasCallbackEvent;

            void readThreadFunc() {
                hasCallbackEvent.wait();
                while (true) {
                    std::uint64_t size;
                    boost::asio::read(*socket, boost::asio::buffer(&size, 8));
                    std::vector<std::uint8_t>& readBuffer = getVectorCallback(size);
                    boost::asio::read(*socket, boost::asio::buffer(readBuffer));
                    break;
                }
            }

        public:
            TCPSocket(std::unique_ptr<boost::asio::ip::tcp::socket> socket) {
                this->socket = std::move(socket);
                readThread = Threading::Thread{[&]() {
                    this->readThreadFunc();
                }};
            }

            void send(std::vector<std::uint8_t> data) override {

            }

            void setDataCallback(std::function<std::vector<std::uint8_t>&(std::uint64_t size)> function) override {
                getVectorCallback = function;
                hasCallbackEvent.signal();
            }

            ~TCPSocket() override {
                socket->shutdown(socket->shutdown_both);
                readThread.join();
            }

        };

        // TODO infiniband transport
        class InfinibandSocket : public Socket::IMPL {
        public:
            InfinibandSocket() {


            }

            void send(std::vector<std::uint8_t> data) override {

            }

            void setDataCallback(std::function<std::vector<std::uint8_t>&(std::uint64_t size)> function) override {

            }

            ~InfinibandSocket() override {

            }
        };

        // TODO, shared memory transport
        class SharedMemorySocket : public Socket::IMPL {

        };

        class Connector {
        public:
            Connector(Endpoint bind);

            void startListen();


        };
    }
}