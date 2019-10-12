#include <RogueLib/Threading/Thread.hpp>
#include <RogueLib/Exceptions/Exceptions.hpp>
#include <RogueLib/Networking/Networking.hpp>
#include <iostream>


using namespace RogueLib::Networking;

void client() {

}

void server() {
//    Server server(25565);
}

int main() {

    unsigned __int128 t = 1;
    t <<= 64;
    printf("%Lf\n", t);

    RogueLib::Threading::Thread serverThread(server);
    RogueLib::Threading::Thread clientThread(client);
    serverThread.start();
    clientThread.start();
    serverThread.join();
    clientThread.join();

}