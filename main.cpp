/**
 * @file main.cpp
 * @brief Main file for the client application
 * 
 * Handles CLI arguments, sets up the client based on the chosen transport protocol, sets up the input handler 
 * and processes incoming messages and user input. Handles signals and exits gracefully.
 * 
 * @author Adam Valík <xvalik05@vutbr.cz>
 * 
*/

#include "Client.hpp"
#include "TCPClient.hpp"
#include "UDPClient.hpp"
#include "InputHandler.hpp"
#include "Message.hpp"

#include <csignal>
#include <atomic>
#include <unordered_map>

// global interrupt flag
atomic<bool> interrupt(false);

// signal handler sets the interrupt flag when SIGINT is received
void signal_handler(int signum) {
    interrupt.store(true);
}

// main function
int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);

    // parse CLI arguments (edge cases of argument processing will not be a part of evaluation)
    unordered_map<string, string> args = {
        {"-t", ""},     // protocol type
        {"-s", ""},     // server IP/hostname
        {"-p", "4567"}, // server port
        {"-d", "250"},  // UDP confirmation timeout
        {"-r", "3"}     // maximum number of UDP retransmissions
    };

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-h") == 0) {
            cout << "\nUsage:\n";
            cout << "\t./ipk24chat-client -t [tcp|udp] -s [server IP/hostname] ";
            cout << "[-p port] [-d UDP confirmation timeout] [-r max UDP retransmissions]\n\n";
            return EXIT_SUCCESS;
        }
        args[argv[i]] = argv[i + 1];
    }

    // create client based on the chosen transport protocol
    unique_ptr<Client> client;
    if (strcmp(args["-t"].c_str(), "tcp") == 0) {
        client = make_unique<TCPClient>(args["-t"], args["-s"], stoi(args["-p"]), stoi(args["-d"]), stoi(args["-r"]));
    } else {
        client = make_unique<UDPClient>(args["-t"], args["-s"], stoi(args["-p"]), stoi(args["-d"]), stoi(args["-r"]));
    }

    // create input handler
    unique_ptr<InputHandler> input_handler = make_unique<InputHandler>();
    string line;

    // setup poll
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; 
    fds[0].events = POLLIN;
    fds[1].fd = client->get_sock();        
    fds[1].events = POLLIN;

    // main loop - process incoming messages and user input until:
    while ( client->get_state() != ClientState::ERROR_EXIT // ERR before connection 
            && client->get_state() != ClientState::ERROR   // ERR from client
            && !client->get_err_received()                 // ERR from server
            && !interrupt.load()                           // signal interrupt
            && client->get_state() != ClientState::END )   // BYE received
    {                                               

        //cout << "Client: waiting on poll()\n"; // DEBUG

        int ret = poll(fds, 2, -1); // wait indefinitely for stdin/socket input
    
        if (ret == -1) {
            if (errno == EINTR) {
                continue; // interrupted by signal, check quit flag
            }
            client->set_err_msg("poll()");
            cerr << "ERR: poll\n";
            client->set_state(ClientState::ERROR);
            break;
        }

        if (fds[0].revents & POLLIN) {
            // handle stdin input
            getline(cin, line);

            // exit command in the start state - just exit, no bye msg
            if (line == "/exit" && client->get_state() == ClientState::START) {
                break;
            }

            auto msg = input_handler->handle_input(line);
            if (msg != nullptr) {
                //cout << "Client: pushing client message\n"; // DEBUG
                client->push_client_msg(msg);
            }

            // eof was encountered
            if (cin.eof()) {
                string exit = "/exit"; // prepare BYE message
                fds[0].fd = -1; // remove stdin from poll set
                auto msg = input_handler->handle_input(exit); 
                client->push_client_msg(msg);
            }
        }

        if (fds[1].revents & POLLIN) {
            // handle incoming message
            //cout << "Client: handle incoming message\n"; // DEBUG
            client->receive_msg();
        }


        // process stdin inputs and incoming messages from queues
        client->process_client_messages();
        client->process_server_messages();
    }

    // send ERR message if there was an error during the client-server communication
    if (client->get_state() == ClientState::ERROR) {
        //cout << "Client: sending ERR msg\n"; // DEBUG
        client->send_msg(make_shared<MsgERR>(input_handler->get_display_name(), client->get_error_msg(), input_handler->get_msgID_sent()));
        input_handler->inc_msgID_sent();
    }

    // send BYE message if there was an error on the client side, error received from the server or signal interrupt (bye for end/eof was already sent)
    if (client->get_state() == ClientState::ERROR || client->get_err_received() || (interrupt.load() && client->get_state() != ClientState::START) ){
        //cout << "Client: sending BYE msg\n"; // DEBUG
        client->send_msg(make_shared<MsgBYE>(input_handler->get_msgID_sent()));
    }

    //cout << "Client: gracefully exiting\n"; // DEBUG

    // EXIT_FAILURE if there was an error on either the client or server side, otherwise EXIT_SUCCESS
    return (client->get_state() == ClientState::ERROR || client->get_state() == ClientState::ERROR_EXIT || client->get_err_received()) ? EXIT_FAILURE : EXIT_SUCCESS;
}