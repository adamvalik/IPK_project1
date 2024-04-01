/**
 * @file Message.hpp
 * @brief Message class header
 * 
 * A parent class for all message types, contains common attributes and methods for all message types.
 * This file also contains derived classes for each message type, which have their implementation of 
 * constructing TCP and UDP variant of the outgoing message.
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <stdint.h>
#include <vector>
#include <string>
#include <arpa/inet.h>

using namespace std;

/**
 * @brief Enum for message types
 */
enum MessageType: uint8_t {
    CONFIRM = 0x00,
    REPLY   = 0x01,
    AUTH    = 0x02,
    JOIN    = 0x03,
    MSG     = 0x04,
    ERR     = 0xFE,
    BYE     = 0xFF
};

/**
 * @class Message
 * @brief A parent class for all message types
 * 
 * Contains common attributes and methods for all message types. Each derived class has its own implementation
 * of constructing TCP and UDP variant of the outgoing message.
 * 
 */
class Message {
    protected:
        MessageType type;
        uint16_t messageID;

    public:
        Message(uint16_t messageID) : messageID(messageID) {};
        virtual ~Message() {};

        /**
         * @brief Construct the UDP message header byte by byte
         * 
         * @return vector<uint8_t> UDP message to send
         */
        virtual vector<uint8_t> UDP_msg() { return {}; };

        /**
         * @brief Construct the TCP message based on the specified ABNF [RFC5234] grammar
         * 
         * @return string TCP message to send
         */
        virtual string TCP_msg() { return ""; };

        // getters
        uint16_t get_msgID() { return this->messageID; };
        MessageType get_type() { return this->type; };
};

/**
 * @class MsgCONFIRM
 * @brief A derived class for a confirmation message
 */
class MsgCONFIRM : public Message {
    public:
        MsgCONFIRM(uint16_t messageID) : Message(messageID) { this->type = MessageType::CONFIRM; };
        vector<uint8_t> UDP_msg() override;
};

/**
 * @class MsgAUTH
 * @brief A derived class for an authenticate message
 */
class MsgAUTH : public Message {
    string username; 
    string secret;
    string display_name;

    public:
        MsgAUTH(string username, string secret, string display_name, uint16_t messageID) : Message(messageID), username(username), secret(secret), display_name(display_name) { this->type = MessageType::AUTH; }
        vector<uint8_t> UDP_msg() override;
        string TCP_msg() override;
};

/**
 * @class MsgJOIN
 * @brief A derived class for a join message
 */
class MsgJOIN : public Message {
    string channelID;
    string display_name;

    public:
        MsgJOIN(string channelID, string display_name, uint16_t messageID) : Message(messageID), channelID(channelID), display_name(display_name) { this->type = MessageType::JOIN; };
        vector<uint8_t> UDP_msg() override;
        string TCP_msg() override;
};

/**
 * @class MsgMSG
 * @brief A derived class for a message to the server
 */
class MsgMSG : public Message {
    string display_name;
    string message_content;

    public:
        MsgMSG(string display_name, string message_content, uint16_t messageID) : Message(messageID), display_name(display_name), message_content(message_content) { this->type = MessageType::MSG; };
        vector<uint8_t> UDP_msg() override;
        string TCP_msg() override;
};

/**
 * @class MsgERR
 * @brief A derived class for an error message
 */
class MsgERR : public Message {
    string display_name;
    string message_content;

    public:
        MsgERR(string display_name, string message_content, uint16_t messageID) : Message(messageID), display_name(display_name), message_content(message_content) { this->type = MessageType::ERR; };
        vector<uint8_t> UDP_msg() override;
        string TCP_msg() override;
};

/**
 * @class MsgBYE
 * @brief A derived class for a goodbye message
 */
class MsgBYE : public Message {
    public:
        MsgBYE(uint16_t messageID) : Message(messageID) { this->type = MessageType::BYE; };
        vector<uint8_t> UDP_msg() override;
        string TCP_msg() override;
};

#endif // MESSAGE_HPP