/**
 * @file Client.cpp
 * @brief Client class implementation 
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "Client.hpp"

// constructor common for both TCP and UDP clients
Client::Client(const string& protocol, const string& server, int port, int timeout, int max_retransmissions) : transp(protocol), server(server), port(port), timeout(timeout), max_retransmissions(max_retransmissions) {
    this->state = ClientState::START;
    this->waiting_on_reply = false;
    this->err_received = false;
    this->auth = false;

    // socket
    this->socktype = strcmp(protocol.c_str(), "tcp") == 0 ? SOCK_STREAM : SOCK_DGRAM;
    this->sock = socket(AF_INET, this->socktype, 0);
    if (this->sock < 0) {
        cerr << "ERR: Failed to create socket\n";
        this->state = ClientState::ERROR_EXIT;
        return;
    }

    // server adrress
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_port = htons(port);

    // convert IP address
    if (inet_pton(AF_INET, server.c_str(), &this->server_addr.sin_addr) <= 0) {
        // not a valid IP address, might be a hostname

        // prepare for getaddrinfo
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = this->socktype;

        // resolve the domain name
        int status = getaddrinfo(server.c_str(), to_string(port).c_str(), &hints, &res);
        if (status != 0) {
            cerr << "ERR: getaddrinfo: " << gai_strerror(status) << std::endl;
            this->state = ClientState::ERROR_EXIT;
            return;
        }
        memcpy(&this->server_addr, res->ai_addr, sizeof(this->server_addr));
        freeaddrinfo(res); 
    }
    //cout << "INFO: Server socket: " << inet_ntoa(this->server_addr.sin_addr) << " : " << ntohs(this->server_addr.sin_port) << "\n"; // DEBUG
}
