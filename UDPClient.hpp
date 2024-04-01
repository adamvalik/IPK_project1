/**
 * @file UDPClient.hpp
 * @brief UDP client class header 
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#ifndef UDPCLIENT_HPP
#define UDPCLIENT_HPP

#include "Client.hpp"
#include "Message.hpp"

#include <set>

using namespace std;

/**
 * @class UDPClient
 * @brief A class for handling UDP client communication
 * 
 * Inherits from the Client class, implements the virtual methods for UDP communication.
 * 
 */
class UDPClient : public Client {
    struct sockaddr_in response_addr; // holds the dynamically allocated server address
    socklen_t response_addr_len;
    set<uint16_t> seen_msg_ids; // set of message IDs that have been seen (in case of duplication)

    private:
        /**
         * @brief Check if the message ID has been seen before
         * 
         * @param msgID Message ID to check
         * @return true The message ID has been seen before
         * @return false The message ID has not been seen before
         */
        bool msgID_seen(uint16_t msgID);

        /**
         * @brief Mark the message ID as seen
         * 
         * @param msgID Message ID to mark as seen
         */
        void mark_msgID_as_seen(uint16_t msgID);

    public:
        /**
         * @brief Construct a new UDPClient object
         * 
         * Inherits the parent constructor, in addition sets the confirmation timeout to the socket.
         * 
         */
        UDPClient(const string& transp, const string& server, int port, int timeout, int max_retransmissions);
        ~UDPClient() override;

        /**
         * @brief Send a message to the server
         * 
         * If the sending fails (sendto() fails or the confirmation is not received), it retries sending the message 
         * up to max_retransmissions times. If the message is of type BYE, sets the client state to END, if the AUTH
         * message is confirmed, sets the client state to AUTHENTICATE. When some other message is recieved while 
         * waiting on the confirmation, the message is stored to process later.
         * 
         * @param msg Message to send
         */
        void send_msg(shared_ptr<Message> msg) override;

        /**
         * @brief Receive a message from the server
         * 
         * Calls recvfrom() to receive a data from the server and pushes the message to the server_message_queue.
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
         * Parses the incoming messages from the queue (in case the message ID was not seen therefore the message 
         * is not a duplicate) and prints relevant information do stdout/stderr, also confirms the message.
         * 
         */
        void process_server_messages() override;
};

#endif // UDPCLIENT_HPP
