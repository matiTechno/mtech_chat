#include <iostream>
#include <asio.hpp>
#include <memory>
#include <set>
#include <deque>
#include <vector>

using asio::ip::tcp;

class Participant
{
public:
    virtual ~Participant() = default;
    virtual void deliver(const std::vector<char>& msg) = 0;
};

class Chat_room
{
public:
    void join(std::shared_ptr<Participant> participant)
    {
        std::vector<char> welcome_msg(255, 0);
        auto str = "\nWelcome to matiTechno chat server! (guest room)\nactive users: "
                + std::to_string(participants.size());
        memcpy(welcome_msg.data(), str.data(), str.size());

        participants.insert(participant);
        participant->deliver(welcome_msg);

        std::string new_user = "New user joined the room!\0";
        std::memcpy(welcome_msg.data(), new_user.data(), new_user.size() + 1);
        deliver(nullptr, welcome_msg);
    }

    void leave(std::shared_ptr<Participant> participant)
    {
        participants.erase(participant);
    }

    void deliver(std::shared_ptr<Participant> participant, const std::vector<char>& msg)
    {
        for(auto& to_p: participants)
            if(to_p != participant)
                to_p->deliver(msg);
    }

private:
    std::set<std::shared_ptr<Participant>> participants;
};

class Chat_session: public Participant, public std::enable_shared_from_this<Chat_session>
{
public:
    Chat_session(tcp::socket&& socket, Chat_room& room):
        socket(std::move(socket)),
        room(room),
        read_buff(255, 0)
    {}

    void deliver(const std::vector<char>& msg) override
    {
        bool write_in_progress = msgs_to_send.size();
        msgs_to_send.push_back(msg);
        if(!write_in_progress)
            send_to_client();
    }

    void start()
    {
        room.join(shared_from_this());
        get_from_client();
    }

private:
    tcp::socket socket;
    Chat_room& room;
    std::deque<std::vector<char>> msgs_to_send;
    std::vector<char> read_buff;

    void get_from_client()
    {
        asio::async_read(socket, asio::buffer(read_buff),
                         [this](asio::error_code ec, std::size_t)
        {
            if(!ec)
            {
                room.deliver(shared_from_this(), read_buff);
                get_from_client();
            }
            else
                room.leave(shared_from_this());
        });
    }

    void send_to_client()
    {
        asio::async_write(socket, asio::buffer(msgs_to_send.front()),
                          [this](asio::error_code ec, std::size_t)
        {
            if(!ec)
            {
                msgs_to_send.pop_front();
                if(msgs_to_send.size())
                    send_to_client();
            }
            else
                room.leave(shared_from_this());
        });
    }
};

class Server
{
public:
    Server(asio::io_service& service, const tcp::endpoint& endpoint):
        acceptor(service, endpoint),
        socket(service)
    {
        accept();
    }

private:
    tcp::acceptor acceptor;
    tcp::socket socket;
    Chat_room chat_room;

    void accept()
    {
        acceptor.async_accept(socket, [this](asio::error_code ec)
        {
            if(!ec)
                std::make_shared<Chat_session>(std::move(socket), chat_room)->start();

            accept();
        });
    }
};

int main()
{
    try
    {
        asio::io_service service;
        tcp::endpoint endpoit(tcp::v4(), 6969);
        Server server(service, endpoit);
        (void)server;
        service.run();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
