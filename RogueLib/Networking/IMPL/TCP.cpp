/**
 * Copyright (c) 2020 RogueLogix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "TCP.hpp"

namespace RogueLib::Networking::IMPL {
    std::vector<std::uint8_t> TCPSocket::read() {
        std::uint64_t size;
        boost::asio::read(*socket, boost::asio::buffer(&size, 8));
        size = be64toh(size);
        std::vector<std::uint8_t> readBuffer = getVector(size);
        assert(readBuffer.size() == size);
        boost::asio::read(*socket, boost::asio::buffer(readBuffer));
        return readBuffer;
    }

   TCPSocket::TCPSocket
            (std::unique_ptr<boost::asio::io_context> service, std::unique_ptr<boost::asio::ip::tcp::socket> socket) :
            IMPL(Socket::UnderlyingType::TCP_IP) {
        this->socket = std::move(socket);
    }

    void TCPSocket::send(std::vector<std::uint8_t> &data) {
        std::uint64_t size = data.size();
        size = htobe64(size);
        boost::asio::write(*socket, boost::asio::buffer(&size, 8));
        boost::asio::write(*socket, boost::asio::buffer(data));
    }

    TCPSocket::~TCPSocket() {
        socket->shutdown(socket->shutdown_both);
    }
}