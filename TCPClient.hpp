/**
 * @file TCPClient.hpp
 * @brief TCPClient class header
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

#include "Client.hpp"
#include "Message.hpp"

#include <algorithm>
#include <cctype>

using namespace std;

/**
 * @class TCPClient
 * @brief A class for handling TCP client communication
 * 
 * Inherits from the Client class, implements the virtual methods for TCP communication.
 * 
 */
class TCPClient : public Client {
    vector<uint8_t> delimiter; // delimiter for message splitting

    public:
        /**
         * @brief Construct a new TCPClient object
         * 
         * Inherits the parent constructor, in addition connects to the server.
         * 
         */
        TCPClient(const string& transp, const string& server, int port, int timeout, int max_retransmissions);
        ~TCPClient() override;

        /**
         * @brief Send a message to the server
         * 
         * If the message is of type BYE, sets the client state to END,
         * if the client state is START, sets the client state to AUTHENTICATE.
         * 
         * @param msg Message to send
         */
        void send_msg(shared_ptr<Message> msg) override;

        /**
         * @brief Receive a message from the server
         * 
         * Calls recv() to receive a data from the server, parses it by a delimeter and pushes the messages
         * to the server_message_queue.
         * 
         */
        void receive_msg() override;

        /**
         * @brief Process messages from the client stored in the queue
         * 
         * Sends the messages from the queue to the server if the client is not waiting for a response.
         * 
         */
        void process_client_messages() override;

        /**
         * @brief Process messages from the server stored in the queue
         * 
         * Parses the incoming messages from the queue and prints relevant information do stdout/stderr.
         * 
         */
        void process_server_messages() override;    
};

#endif // TCPCLIENT_HPP
