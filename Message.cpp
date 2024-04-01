/**
 * @file Message.cpp
 * @brief Message class implementation
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "Message.hpp"


//   1 byte       2 bytes      
// +--------+--------+--------+
// |  0x00  |  Ref_MessageID  |
// +--------+--------+--------+
//
vector<uint8_t> MsgCONFIRM::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));
    return msg;
}


//   1 byte       2 bytes      
// +--------+--------+--------+-----~~-----+---+-------~~------+---+----~~----+---+
// |  0x02  |    MessageID    |  Username  | 0 |  DisplayName  | 0 |  Secret  | 0 |
// +--------+--------+--------+-----~~-----+---+-------~~------+---+----~~----+---+
//
vector<uint8_t> MsgAUTH::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));

    for (char c : this->username) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    for (char c : this->display_name) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    for (char c : this->secret) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    return msg;
}

// AUTH {Username} AS {DisplayName} USING {Secret}\r\n
string MsgAUTH::TCP_msg() {
    return "AUTH " + this->username + " AS " + this->display_name + " USING " + this->secret + "\r\n";
}


//   1 byte       2 bytes      
// +--------+--------+--------+-----~~-----+---+-------~~------+---+
// |  0x03  |    MessageID    |  ChannelID | 0 |  DisplayName  | 0 |
// +--------+--------+--------+-----~~-----+---+-------~~------+---+
//
vector<uint8_t> MsgJOIN::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));

    for (char c : this->channelID) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    for (char c : this->display_name) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    return msg;
}

// JOIN {ChannelID} AS {DisplayName}\r\n
string MsgJOIN::TCP_msg() {
    return "JOIN " + this->channelID + " AS " + this->display_name + "\r\n";
}


//   1 byte       2 bytes      
// +--------+--------+--------+-------~~------+---+--------~~---------+---+
// |  0x04  |    MessageID    |  DisplayName  | 0 |  MessageContents  | 0 |
// +--------+--------+--------+-------~~------+---+--------~~---------+---+
//
vector<uint8_t> MsgMSG::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));

    for (char c : this->display_name) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    for (char c : this->message_content) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    return msg;
}

// MSG FROM {DisplayName} IS {MessageContent}\r\n
string MsgMSG::TCP_msg() {
    return "MSG FROM " + this->display_name + " IS " + this->message_content + "\r\n";
}


//   1 byte       2 bytes
// +--------+--------+--------+-------~~------+---+--------~~---------+---+
// |  0xFE  |    MessageID    |  DisplayName  | 0 |  MessageContents  | 0 |
// +--------+--------+--------+-------~~------+---+--------~~---------+---+
//
vector<uint8_t> MsgERR::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));

    for (char c : this->display_name) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    for (char c : this->message_content) {
        msg.push_back(c);
    }
    msg.push_back(0x00);
    return msg;
}

// ERR FROM {DisplayName} IS {MessageContent}\r\n
string MsgERR::TCP_msg() {
    return "ERR FROM " + this->display_name + " IS " + this->message_content + "\r\n";
}


//   1 byte       2 bytes      
// +--------+--------+--------+
// |  0xFF  |    MessageID    |
// +--------+--------+--------+
//
vector<uint8_t> MsgBYE::UDP_msg() {
    vector<uint8_t> msg;
    msg.push_back(this->type);
    // messageID sent in network byte order
    msg.push_back(static_cast<uint8_t>(this->messageID >> 8));
    msg.push_back(static_cast<uint8_t>(this->messageID & 0xFF));
    return msg;
}

// BYE\r\n
string MsgBYE::TCP_msg() {
    return "BYE\r\n";
}
