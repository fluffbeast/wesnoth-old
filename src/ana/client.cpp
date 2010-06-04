#include <iostream>

#include "getopt_pp.h"

#include "ana.hpp"

using namespace GetOpt;
using namespace ana;

const port DEFAULT_PORT = "30303";

const char* const DEFAULT_ADDRESS = "127.0.0.1";
const char* const DEFAULT_NAME    = "Unnamed";
const char* const DEFAULT_PROXYA  = "none";


void show_help()
{
    std::cout << "Valid options are:\n"
        "\t-p --port        [optional]    Set port. Default=" << DEFAULT_PORT << std::endl <<
        "\t-a --address     [optional]    Set address. Default=" << DEFAULT_ADDRESS << std::endl <<
        "\t-n --name        [recommended] Set name. Default=" << DEFAULT_NAME << std::endl <<
        "\t-x --proxyaddr   [optional]    Set proxy address." << std::endl <<
        "\t-y --proxyport   [optional]    Set proxy port." << std::endl <<
        "\t-t --auth        [optional]    Set proxy authentication type use [none,basic,digest]. Default=" << DEFAULT_PROXYA << std::endl <<
        "\t-u --user        [optional]    Set proxy authentication user name." << std::endl <<
        "\t-w --password    [optional]    Set proxy authentication password." << std::endl;
    ;
}

struct connection_info
{
    connection_info() :
        pt(DEFAULT_PORT),
        address(DEFAULT_ADDRESS),
        name(DEFAULT_NAME),
        proxyaddr(),
        proxyport(),
        auth("none"),
        user(),
        password()
    {
    }

    bool use_proxy() const
    {
        return (auth == "none" || auth == "basic" || auth == "digest") && proxyaddr != "" && proxyport != "";
    }

    ana::proxy::authentication_type get_ana_auth_type() const
    {
        if (auth == "digest")
            return ana::proxy::digest;
        else if (auth == "basic")
            return ana::proxy::basic;
        else
            return ana::proxy::none;
    }

    port        pt;
    std::string address;
    std::string name;
    std::string proxyaddr;
    std::string proxyport;
    std::string auth;
    std::string user;
    std::string password;
};

class ChatClient : public ana::listener_handler,
                   public ana::send_handler,
                   public ana::connection_handler
{
    public:
        ChatClient(connection_info ci) :
            conn_info_( ci ),
            continue_(true),
            client_( ana::client::create(ci.address, ci.pt) ),
            name_(ci.name)
        {
        }

        std::string get_name(const std::string& msg)
        {
            size_t pos = msg.find(" ");

            return msg.substr(pos+1);
        }

        void parse_command(const std::string& msg)
        {
            if (msg[1] == 'n') //Lame: assume name command
            {
                name_ = get_name(msg);
            }
        }

        void run_input()
        {
            std::string msg;
            do
            {
                std::cout << name_ << " : ";
                std::getline(std::cin, msg);

                if (msg[0] == '/')
                    parse_command(msg);

                if (msg == "/quit")
                    std::cout << "\nExiting.\n";
                else
                    if (msg.size() > 0)
                        client_->send( ana::buffer( msg ), this);

            } while ( (msg != "/quit") && continue_);
        }

        void run()
        {
            try
            {

                if ( ! conn_info_.use_proxy() )
                    client_->connect( this );
                else
                    client_->connect_through_proxy(conn_info_.get_ana_auth_type(),
                                                   conn_info_.proxyaddr,
                                                   conn_info_.proxyport,
                                                   this);

                client_->set_listener_handler( this );
                client_->run();

                std::cout << "Available commands: \n"  <<
                             "    '/quit'      : Quit. \n"
                             "    '/who'       : List connected users. \n" <<
                             "    '/name name' : Change name." << std::endl;

                run_input();

            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }

            delete client_;
        }

    private:

        virtual void handle_connect( ana::error_code error, client_id server_id )
        {
            if ( error )
                std::cerr << "Error connecting." << std::endl;
            else
            {
                std::cout << "\nConnected.\n";
                client_->send( ana::buffer( std::string("/name ") + name_) , this);
            }
        }

        virtual void handle_disconnect( ana::error_code error, client_id server_id)
        {
            std::cerr << "\nServer disconnected. Enter to exit." << std::endl;
            continue_ = false;
        }

        virtual void handle_message( ana::error_code error, client_id, ana::detail::read_buffer msg)
        {
            if (! error)
            {
                std::cout << std::endl << msg->string()
                          << std::endl << name_ << " : ";
                std::cout.flush();
            }
        }

        virtual void handle_send( ana::error_code error, client_id client)
        {
            if ( error )
                std::cout << "Error. Timeout?" << std::endl;
        }

        connection_info     conn_info_;
        bool                continue_;
        ana::client*        client_;
        std::string         name_;
};

int main(int argc, char **argv)
{
    GetOpt_pp options(argc, argv);

    if (options >> OptionPresent('h', "help"))
        show_help();
    else
    {
        connection_info ci;

        options >> Option('p', "port", ci.pt)
                >> Option('a', "address", ci.address)
                >> Option('n', "name", ci.name)
                >> Option('x', "proxyaddr", ci.proxyaddr)
                >> Option('y', "proxyport", ci.proxyport)
                >> Option('t', "auth", ci.auth)
                >> Option('u', "user", ci.user)
                >> Option('w', "password", ci.password);

        std::cout << "Main client app.: Starting client" << std::endl;

        ChatClient client(ci);
        client.run();
    }
}
