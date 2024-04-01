/**
 * @file UDPClient.cpp
 * @brief UDP client class implementation 
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "UDPClient.hpp"


UDPClient::UDPClient(const string& transp, const string& server, int port, int timeout, int max_retransmissions) : Client(transp, server, port, timeout, max_retransmissions) {
    response_addr_len = sizeof(response_addr);

    // set the confirmation timeout
    struct timeval tv = {
        .tv_sec = this->timeout / 1000,
        .tv_usec = (this->timeout % 1000) * 1000
    };
    setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

UDPClient::~UDPClient() {
    close(this->sock);
}


bool UDPClient::msgID_seen(uint16_t msgID) {
    return this->seen_msg_ids.find(msgID) != this->seen_msg_ids.end();
}

void UDPClient::mark_msgID_as_seen(uint16_t msgID) {
    this->seen_msg_ids.insert(msgID);
}


void UDPClient::send_msg(shared_ptr<Message> msg) {

    ssize_t bytestx, bytesrx;
    int retransmissions = 0;
    bool retransmit;
    bool confirmed = false;

    while (!confirmed) {
        //if (retransmissions > 0) { cout << "INFO: Retransmission " << retransmissions << "\n"; } // DEBUG
        retransmit = false;
    
        // send message
        if (this->state == ClientState::START) {
            // auth message is sent to the specified port
            //cout << "Client: Sending auth message to specified port\n"; // DEBUG
            bytestx = sendto(this->sock, msg->UDP_msg().data(), msg->UDP_msg().size(), 0, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr));
        } else {
            // other messages are sent to the dynamically assigned port
            //cout << "Client: Sending message to dyn port\n"; // DEBUG
            bytestx = sendto(this->sock, msg->UDP_msg().data(), msg->UDP_msg().size(), 0, (struct sockaddr*)&this->response_addr, this->response_addr_len);
        }
        // if (bytestx < 0) {
        //     if (retransmissions < this->max_retransmissions) {
        //         retransmissions++;
        //         continue;
        //     }
        //     else {
        //         cerr << "ERR: Failed to send the message\n";
        //         this->set_state(ClientState::ERROR);
        //         return;
        //     }
        // }

        while (!retransmit) {
            // receive confirmation
            memset(this->buffer, 0, BUFFER_SIZE);
            //cout << "Client: Waiting for confirmation\n"; // DEBUG
            bytesrx = recvfrom(this->sock, this->buffer, BUFFER_SIZE, 0, (struct sockaddr*)&this->response_addr, &this->response_addr_len);
            //cout << "Client: Received " << bytesrx << " bytes\n"; // DEBUG
            if (bytesrx < 0) {
                if ((errno == EAGAIN || errno == EWOULDBLOCK) && retransmissions < this->max_retransmissions) {
                    retransmissions++;
                    retransmit = true;
                    continue;
                }
                else {
                    //cout << "INFO: Server is not responding\n"; // DEBUG
                    this->set_state(ClientState::END);
                    return;
                }
            }
            if (this->buffer[0] == MessageType::CONFIRM) {
                // check the messageID
                uint16_t msgID = (this->buffer[1] << 8) | this->buffer[2];
                if (msgID != msg->get_msgID()) {
                    continue;
                }
                if (msg->get_type() == MessageType::BYE) {
                    // bye message is confirmed, go to end state
                    this->state = ClientState::END;
                }
                if (this->state == ClientState::START) {
                    //cout << "Client: Auth message confirmed from port: " << ntohs(this->response_addr.sin_port) << "\n"; // DEBUG
                    // auth message is confirmed by the server, go to authenticate state
                    this->state = ClientState::AUTHENTICATE;
                }
                confirmed = true;
                break;
            }
            else { // received something else than confirmation, process it
                //cout << "INFO: Received something else than confirmation\n"; // DEBUG
                vector<uint8_t> message_data(bytesrx);
                memcpy(message_data.data(), this->buffer, bytesrx);
                this->server_msg_queue.push(message_data);
                process_server_messages();
            }
        }
    }
}


void UDPClient::receive_msg() {
    memset(this->buffer, 0, BUFFER_SIZE);
    ssize_t bytesrx = recvfrom(this->sock, this->buffer, BUFFER_SIZE, 0, (struct sockaddr*)&this->response_addr, &this->response_addr_len);
    if (bytesrx < 0) {
        cerr << "ERR: Failed to receive message\n";
        return;
    }
    
    //cout << "Client: Message came from port: " << ntohs(this->response_addr.sin_port) << "\n"; // DEBUG

    // convert char buffer to vector<uint8_t>
    vector<uint8_t> message_data(bytesrx);
    memcpy(message_data.data(), this->buffer, bytesrx);
    this->server_msg_queue.push(message_data);
}


void UDPClient::process_client_messages() {
    // send messages only when not waiting on a reply
    while (!this->waiting_on_reply && !this->client_msg_queue.empty()) { 
        //cout << "Client: processing client messages\n"; // DEBUG

        auto msg = this->client_msg_queue.front();

        if (msg == nullptr) {
            this->client_msg_queue.pop();
            continue;
        }
        if (msg->get_type() != MessageType::AUTH && !this->auth) {
            cerr << "ERR: You need to authenticate first\n";
            this->client_msg_queue.pop();
            continue;
        }
        if ((msg->get_type() == MessageType::MSG || msg->get_type() == MessageType::JOIN) && this->state != ClientState::OPEN) {
            cerr << "ERR: Cannot send message in non-open state\n";
            this->client_msg_queue.pop();
            continue;
        }
        if (msg->get_type() == MessageType::AUTH && this->auth) { 
            cerr << "ERR: No need to authenticate, already authenticated\n"; 
            this->client_msg_queue.pop();
            continue;
        }

        // send the message
        this->send_msg(msg);

        if (msg->get_type() == MessageType::AUTH || msg->get_type() == MessageType::JOIN) {
            //cout << "Client: Waiting on reply\n"; // DEBUG
            this->waiting_on_reply = true;
        }
        else {
            //cout << "Client: Not waiting on reply -> popping\n"; // DEBUG
            this->client_msg_queue.pop();
        }
    }    
}


void UDPClient::process_server_messages() {
    while (!this->server_msg_queue.empty()) {
        //cout << "Client: Processing server messages\n"; // DEBUG

        // get the current message (FIFO)
        vector<uint8_t> msg = this->server_msg_queue.front();

        // in case of a message shorter than 3 bytes, skip it
        if (msg.size() < 3) {
            this->server_msg_queue.pop();
            continue;
        }

        // extract the messageID
        uint16_t msgID = (msg[1] << 8) | msg[2];
        uint16_t ref_msgID;

        // if the messageID was not seen, process the message (packet duplication)
        if (!this->msgID_seen(msgID) || msg[0] == MessageType::CONFIRM) {
            // if confirm is recieved here, means that some of the messages sent is confirmed again
            if (msg[0] != MessageType::CONFIRM) {
                this->mark_msgID_as_seen(msgID);
            }

            size_t idx = 3; // skip the message type and messageID
            string display_name, message_content;
            // process the message based on its type
            switch (msg[0]) {
                case MessageType::REPLY:
                    // ignore unwanted reply messages
                    if (!this->waiting_on_reply) {
                        break;
                    }
                    
                    //cout << "Client: Received reply\n"; // DEBUG
                    ref_msgID = (msg[4] << 8) | msg[5];
                    if (this->get_curr_msgID() != ref_msgID) {
                        cerr << "ERR: Received reply for wrong message\n";
                        this->error_msg = "Received reply for wrong message";
                        this->set_state(ClientState::ERROR);
                        break;
                    }

                    message_content = string(msg.begin() + 6, msg.end());

                    if (msg[3] == 0x00) { // !REPLY
                        cerr << "Failure: " << message_content << "\n";
                    }
                    else if (msg[3] == 0x01) {
                        cerr << "Success: " << message_content << "\n";
                        if (this->get_curr_msg()->get_type() == MessageType::AUTH) {
                            // successfully authenticated, go to open state
                            this->auth = true;
                            this->set_state(ClientState::OPEN);
                        }
                    }
                    else {
                        cerr << "ERR: Unknown reply type\n";
                        this->error_msg = "Unknown reply type";
                        this->set_state(ClientState::ERROR);
                        break;
                    }
                    this->client_msg_queue.pop(); // remove the message being replied to from the client_queue
                    this->waiting_on_reply = false; // allow sending another message
                    break;

                case MessageType::CONFIRM:
                    // recieved confirmation for a message that was already confirmed
                    break;

                case MessageType::ERR:
                    //cout << "Client: Received error\n"; // DEBUG
                    // ERR FROM DisplayName: MessageContent\n
                    while (idx < msg.size() && msg[idx] != 0) {
                        display_name += static_cast<char>(msg[idx]);
                        ++idx;
                    }
                    ++idx; 
                    while (idx < msg.size() && msg[idx] != 0) {
                        message_content += static_cast<char>(msg[idx]);
                        ++idx;
                    }
                    cerr << "ERR FROM " << display_name << ": " << message_content << "\n";
                    this->err_received = true;
                    break;

                case MessageType::MSG:
                    //cout << "Client: Received message\n"; // DEBUG
                    // DisplayName: MessageContent\n
                    while (idx < msg.size() && msg[idx] != 0) {
                        display_name += static_cast<char>(msg[idx]);
                        ++idx;
                    }
                    ++idx; 
                    while (idx < msg.size() && msg[idx] != 0) {
                        message_content += static_cast<char>(msg[idx]);
                        ++idx;
                    }
                    cout << display_name << ": " << message_content << "\n";
                    break;

                case MessageType::BYE:
                    //cout << "Client: Received bye\n"; // DEBUG
                    this->set_state(ClientState::END);
                    break;

                default:
                    // unknown message type, set the error state but also confirm it
                    cerr << "ERR: Unknown message type\n";
                    this->error_msg = "Unknown message type";
                    this->set_state(ClientState::ERROR);
                    break;    
            }
        }

        // either way, pop the message from the queue and confirm the delivery
        this->server_msg_queue.pop();

        if (msg[0] != MessageType::CONFIRM) {
            // confirm the message (not the confirm message though)
            //cout << "Client: Sending confirmation for messageID " << msgID << "\n"; // DEBUG
            MsgCONFIRM confirm(msgID);
            sendto(this->sock, confirm.UDP_msg().data(), confirm.UDP_msg().size(), 0, (struct sockaddr*)&this->response_addr, this->response_addr_len);
        }
        if (msg[0] == MessageType::REPLY && this->state != ClientState::ERROR) { 
            // got reply, handle client messages that came while waiting for reply
            process_client_messages(); 
        }
    }
}

