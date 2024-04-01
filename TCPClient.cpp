/**
 * @file TCPClient.cpp
 * @brief TCPClient class implementation
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "TCPClient.hpp"

TCPClient::TCPClient(const string& transp, const string& server, int port, int timeout, int max_retransmissions) : Client(transp, server, port, timeout, max_retransmissions) {
    this->delimiter = {'\r', '\n'};

    // connect
    if (this->state == ClientState::ERROR_EXIT) {
        // if there was an error in the Client constructor, do not continue
        return;
    }
    if (connect(this->sock, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr)) < 0) {
        cerr << "ERR: Failed to connect to server\n";
        this->state = ClientState::ERROR_EXIT;
        return;
    }
    //cout << "Client: Connected to server\n"; // DEBUG
}

TCPClient::~TCPClient() {
    //shutdown(this->sock, SHUT_RDWR);
    close(this->sock);
}


void TCPClient::send_msg(shared_ptr<Message> msg) {

    //cout << "Client: Sending message to server: " << msg->TCP_msg(); // DEBUG
    ssize_t bytestx = send(this->sock, msg->TCP_msg().c_str(), msg->TCP_msg().length(), 0);
    if (bytestx < 0) {
        cerr << "ERR: Failed to send message to server\n";
        this->state = ClientState::ERROR;
        return;
    }
    // bye message closes the connection
    if (msg->get_type() == MessageType::BYE) {
        this->state = ClientState::END;
    }
    // auth message changes the state to authenticate
    if (this->state == ClientState::START) {
        this->state = ClientState::AUTHENTICATE;
    }
}


void TCPClient::receive_msg() {
    ssize_t bytesrx;
    static vector<uint8_t> received; // static to keep the message between calls

    // receive message
    memset(this->buffer, 0, BUFFER_SIZE);
    bytesrx = recv(this->sock, this->buffer, BUFFER_SIZE, 0);
    if (bytesrx < 0) {
        cerr << "ERR: Failed to receive message from server\n";
        return;
    }
    if (bytesrx == 0) {
        //cout << "INFO: Server closed the connection\n"; // DEBUG
        this->state = ClientState::END;
        return;
    }

    // append the received message to the vector
    received.insert(received.end(), this->buffer, this->buffer + bytesrx);

    // find the delimiter
    auto iter = search(received.begin(), received.end(), this->delimiter.begin(), this->delimiter.end());

    // process the received messages
    while (iter != received.end()) {
        // extract message
        vector<uint8_t> message(received.begin(), iter);
        this->push_server_msg(message);

        // erase message from the vector
        received.erase(received.begin(), iter + delimiter.size());

        // find the next delimiter
        iter = search(received.begin(), received.end(), delimiter.begin(), delimiter.end());
    }
}


void TCPClient::process_client_messages() {
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

// helper function for converting string to uppercase
string str_toupper(string str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

void TCPClient::process_server_messages() {
    while (!this->server_msg_queue.empty()) {
        //cout << "Client: processing server messages\n"; // DEBUG
    
        // get the current message (FIFO)
        vector<uint8_t> msg_bytes = this->server_msg_queue.front();
        
        // pop the message from the queue
        this->server_msg_queue.pop();

        // skip empty messages
        if (msg_bytes.empty()) {
            continue;
        }

        // convert vector<uint8_t> to string
        string msg(msg_bytes.begin(), msg_bytes.end());

        // parse the message
        istringstream iss(msg);
        string msg_type;
        iss >> msg_type;

        // according to RFC5234, the rule names are case insensitive
        msg_type = str_toupper(msg_type);

        // process the message based on its type
        if (msg_type == "REPLY") {
            // ignore unwanted reply messages
            if (this->waiting_on_reply) {
                //cout << "Client: Received reply\n"; // DEBUG
                string result, token_is, message_content;
                iss >> result;

                if (iss >> token_is && str_toupper(token_is) == "IS") {
                    getline(iss, message_content); 
                }
                else {
                    cerr << "ERR: Invalid REPLY message\n";
                    this->error_msg = "Invalid REPLY message";
                    this->set_state(ClientState::ERROR);
                    return;
                }

                if (str_toupper(result) == "NOK") { // !REPLY
                    cerr << "Failure:" << message_content << "\n";
                }
                else if (str_toupper(result) == "OK") {
                    cerr << "Success:" << message_content << "\n";
                    if (this->get_curr_msg()->get_type() == MessageType::AUTH) {
                        // successfully authenticated, go to open state
                        this->auth = true;
                        this->set_state(ClientState::OPEN);
                    }
                }
                else {
                    // unknown result, set the error state
                    cerr << "ERR: Unknown result\n";
                    this->error_msg = "Unknown result";
                    this->set_state(ClientState::ERROR);
                    return;
                }

                this->client_msg_queue.pop(); // remove the message being replied to from the client_queue
                this->waiting_on_reply = false; // allow sending another message
                process_client_messages(); // handle client messages that came while waiting for reply 
            }
        }
        else if (msg_type == "ERR") {
            //cout << "Client: Received error\n"; // DEBUG
            // ERR FROM DisplayName: MessageContent\n
            string token_from, display_name, token_is, message_content;

            if (iss >> token_from && str_toupper(token_from) == "FROM") {
                iss >> display_name;
            }
            else {
                cerr << "ERR: Invalid ERR message\n";
                this->error_msg = "Invalid ERR message";
                this->set_state(ClientState::ERROR);
                return;
            }

            if (iss >> token_is && str_toupper(token_is) == "IS") {
                getline(iss, message_content); 
            }
            else {
                cerr << "ERR: Invalid ERR message\n";
                this->error_msg = "Invalid ERR message";
                this->set_state(ClientState::ERROR);
                return;
            }

            cerr << "ERR FROM " << display_name << ":" << message_content << "\n";
            this->err_received = true;
        }
        else if (msg_type == "MSG") {
            //cout << "Client: Received message\n"; // DEBUG
            // DisplayName: MessageContent\n
            string token_from, display_name, token_is, message_content;

            if (iss >> token_from && str_toupper(token_from) == "FROM") {
                iss >> display_name;
            }
            else {
                cerr << "ERR: Invalid MSG message\n";
                this->error_msg = "Invalid MSG message";
                this->set_state(ClientState::ERROR);
                return;
            }

            if (iss >> token_is && str_toupper(token_is) == "IS") {
                getline(iss, message_content); 
            }
            else {
                cerr << "ERR: Invalid MSG message\n";
                this->error_msg = "Invalid MSG message";
                this->set_state(ClientState::ERROR);
                return;
            }

            cout << display_name << ":" << message_content << "\n";
        }
        else if (msg_type == "BYE") {
            //cout << "Client: Received bye\n"; // DEBUG
            this->set_state(ClientState::END);
        }
        else {
            // unknown message type, set the error state
            cerr << "ERR: Unknown message type\n";
            this->error_msg = "Unknown message type";
            this->set_state(ClientState::ERROR);
        }
    }
}



