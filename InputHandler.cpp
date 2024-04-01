/**
 * @file InputHandler.cpp
 * @brief InputHandler class implementation
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "InputHandler.hpp"

shared_ptr<Message> InputHandler::handle_input(string& input) {

    // regex patterns for input parameters validation
    regex username_channelID_pattern(R"([A-Za-z0-9-]{1,20})");
    regex secret_pattern(R"([A-Za-z0-9-]{1,128})");
    regex display_name_pattern(R"([\x21-\x7E]{1,20})");
    regex message_content_pattern(R"([\x20-\x7E]{1,1400})");

    // handle empty input
    if (input.empty()) return nullptr;

    if (input[0] == '/') {
        // command
        istringstream iss(input.substr(1)); // removes '/'
        string command, arg;
        iss >> command;
        vector<string> args;

        if (command == "help") {
            cout << "\nList of commands:\n";
            cout << "\t/help - display this message\n";
            cout << "\t/auth <username> <secret> <display_name> - authenticate\n";
            cout << "\t/join <channelID> - join a channel\n";
            cout << "\t/rename <new_display_name> - change display name\n";
            cout << "\t/exit - exit the application\n";
            return nullptr;
        }
        else {
            // parse the parameters into the vector
            while (iss >> arg) {
                args.push_back(arg);
            }

            // auth <username> <secret> <display_name>
            if (command == "auth" && args.size() == 3) {
                // check the parameters validity with regex
                if (!regex_match(args[0], username_channelID_pattern)) {
                    cerr << "ERR: Username is not valid\n";
                    return nullptr;
                }
                if (!regex_match(args[1], secret_pattern)) {
                    cerr << "ERR: Secret is not valid\n";
                    return nullptr;
                }
                if (!regex_match(args[2], display_name_pattern)) {
                    cerr << "ERR: Display name is not valid\n";
                    return nullptr;
                }
                // store the display name
                this->display_name = args[2];
                shared_ptr<Message> auth = make_shared<MsgAUTH>(args[0], args[1], this->display_name, this->msgID_sent);
                this->msgID_sent++;
                //cout << "InputHandler: AUTH message ready\n"; // DEBUG
                return auth;
            }
            // join <channelID>
            else if (command == "join" && args.size() == 1) {
                // check the parameter validity with regex
                if (!regex_match(args[0], username_channelID_pattern)) { // comment when testing on reference server
                    cerr << "ERR: Channel ID is not valid\n";
                    return nullptr;
                }
                shared_ptr<Message> join = make_shared<MsgJOIN>(args[0], this->display_name, this->msgID_sent);
                this->msgID_sent++;
                //cout << "InputHandler: JOIN message ready\n"; // DEBUG
                return join;
            }
            // rename <new_display_name>
            else if (command == "rename" && args.size() == 1) {
                // check the parameter validity with regex
                if (!regex_match(args[0], display_name_pattern)) {
                    cerr << "ERR: Display name is not valid\n";
                    return nullptr;
                }
                // update the display name
                this->display_name = args[0];
                return nullptr;
            }
            // exit
            else if (command == "exit" && args.size() == 0) {
                shared_ptr<Message> bye = make_shared<MsgBYE>(this->msgID_sent);
                this->msgID_sent++;
                //cout << "InputHandler: BYE message ready\n"; // DEBUG
                return bye;
            }
            else {
                cerr << "ERR: Unknown or malformed command\n";
                return nullptr; 
            }
        }
    } else { // message
        // check the message content validity with regex
        if (!regex_match(input, message_content_pattern)) {
            cerr << "ERR: Message content is not valid\n";
            return nullptr;
        }
        shared_ptr<Message> msg = make_shared<MsgMSG>(this->display_name, input, this->msgID_sent);
        this->msgID_sent++;
        //cout << "InputHandler: MSG message ready\n"; // DEBUG
        return msg;
    }
}

