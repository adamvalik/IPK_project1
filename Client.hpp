/**
 * @file Client.hpp
 * @brief Client class header
 * 
 * A parent class for TCPClient and UDPClient classes, contains common attributes and methods for both classes.
 * Contains ClientState enum, which is used for implementation of Client FSM.
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Message.hpp"

#include <iostream>
#include <cstring>
#include <sstream>
#include <memory>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <queue>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

#define BUFFER_SIZE 1500

// not having to write std:: everywhere
using namespace std;

/**
 * @brief States of the client FSM
 */
enum ClientState {
    START,
    AUTHENTICATE,
    OPEN,
    END,
    ERROR,
    ERROR_EXIT
};


/**
 * @class Client
 * @brief A parent class for TCPClient and UDPClient classes
 * 
 * Contains common attributes and methods for both classes. Used in main.cpp as a general client
 * regradless of the chosen transport protocol. Virtual methods are then implemented in TCPClient and UDPClient.
 * 
 */
class Client {
    protected:
        string transp; // tcp/udp
        string server; // server IP/hostname
        int port; 
        int timeout;
        int max_retransmissions;

        ClientState state;
        int sock; // socket
        int socktype; // SOCK_STREAM or SOCK_DGRAM
        
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];

        // queues for incoming and outgoing messages
        queue<shared_ptr<Message>> client_msg_queue;
        queue<vector<uint8_t>> server_msg_queue;

        bool waiting_on_reply; // flag for waiting on server reply to auth/join message
        bool err_received; // flag indicating that ERR message was received from the server
        bool auth; // flag indicating that the client was successfully authenticated

        string error_msg; 

    public:
        /**
         * @brief Construct a new Client object
         * 
         * Initializes the attributes, creates a socket and sets the server address 
         * based on the given IP address or resolves the domain name. 
         * (DNS resolution is done by getaddrinfo() function.
         * 
         * @param transp Either "tcp" or "udp"
         * @param server IP/hostname
         * @param port Server port
         * @param timeout UDP confirmation timeout in milliseconds
         * @param max_retransmissions Maximum number of retransmissions for UDP messages (after initial transmission)
         */
        Client(const string& transp, const string& server, int port, int timeout, int max_retransmissions);
        virtual void send_msg(shared_ptr<Message>) {};
        virtual void receive_msg() {};
        virtual void process_client_messages() {};
        virtual void process_server_messages() {};
        virtual ~Client() {};

        // getters and setters
        ClientState get_state() const { return this->state; }
        int get_sock() const { return this->sock; }
        int get_curr_msgID() { return this->client_msg_queue.front()->get_msgID(); }
        shared_ptr<Message> get_curr_msg() { return this->client_msg_queue.front(); }
        bool get_err_received() { return this->err_received; }
        string get_error_msg() { return this->error_msg; }
        bool is_auth() { return this->auth; }
        void set_err_msg(const string& msg) { this->error_msg = msg; }
        void set_state(ClientState state) { this->state = state; }

        /**
         * @brief Push a message to the client message queue
         * 
         * @param msg Message in a form of a shared pointer
         */
        void push_client_msg(shared_ptr<Message> msg) { this->client_msg_queue.push(msg); }

        /**
         * @brief Push a message to the server message queue
         * 
         * @param msg Message in a form of a vector of bytes
         */
        void push_server_msg(vector<uint8_t> msg) { this->server_msg_queue.push(msg); }
};

#endif // CLIENT_HPP
