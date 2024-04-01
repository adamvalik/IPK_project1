/**
 * @file InputHandler.hpp
 * @brief InputHandler class header
 * 
 * A class for handling user input, parsing it and creating messages based on the input.
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include "Message.hpp"

#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>

using namespace std;

class InputHandler {
    uint16_t msgID_sent; // keeps track of the message ID for the upcoming message
    string display_name; // stores the client's display name
    
    public:
        // constructor, initializes the message ID to 0
        InputHandler() : msgID_sent(0) {};
        ~InputHandler() {};

        // getters
        int get_msgID_sent() { return this->msgID_sent; } // for sending messages not through the input handler (BYE/ERR)
        string get_display_name() { return this->display_name; }
        
        /**
         * @brief Increment the message ID for the upcoming message from the client
         */
        void inc_msgID_sent() { this->msgID_sent++; }

        /**
         * @brief Parse the user input and create a message based on the input
         * 
         * Also checks the input for validity using regex patterns
         * 
         * @param input User input from stdin
         * @return shared_ptr<Message> A message created based on the input
         */
        shared_ptr<Message> handle_input(string& input);
};

#endif // CLIENTMSGHANDLER_HPP