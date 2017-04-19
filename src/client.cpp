#include <iostream>
#include <asio.hpp>
#include <string>
#include <deque>
#include <vector>
#include <future>
#include <fstream>

#include "sound_system.hpp"

using asio::ip::tcp;

// message specification for now:
// max_size == 255 == 254 + 0
// todo: clean this string construction mess

void save_config(const std::string& client_name, int volume);
void load_config(std::string& client_name, int& volume);

class Client
{
public:
    Client(asio::io_service& service, tcp::resolver::iterator endp_it, Sound_system& ssys, Sample* sample):
        service(service),
        socket(service),
        buffer(255, 0),
        ssys(ssys),
        sample(sample)
    {
        connect(endp_it);
    }

    void write(const std::vector<char>& msg)
    {
        service.post([this, msg]
        {
            bool is_busy = msgs_to_write.size();
            msgs_to_write.push_back(std::move(msg));
            if(!is_busy)
                write_impl();
        });
    }

private:
    asio::io_service& service;
    tcp::socket socket;
    std::vector<char> buffer;
    std::deque<std::vector<char>> msgs_to_write;
    Sound_system& ssys;
    Sample* sample;

    void connect(tcp::resolver::iterator endp_it)
    {
        asio::async_connect(socket, endp_it, [this](std::error_code ec, tcp::resolver::iterator)
        {
            if(!ec)
                listen();
            else
            {
                // handle error
            }
        });
    }

    void listen()
    {

        asio::async_read(socket, asio::buffer(buffer), [this](std::error_code ec, std::size_t)
        {
            if(!ec)
            {
                if(sample)
                    ssys.play_sample(*sample);
                std::cout << buffer.data() << std::endl;
                listen();
            }
            else
            {
                // handle error
            }
        });
    }

    void write_impl()
    {
        asio::async_write(socket, asio::buffer(msgs_to_write.front()), [this](std::error_code ec, std::size_t)
        {
            if(!ec)
            {
                msgs_to_write.pop_front();
                if(msgs_to_write.size())
                    write_impl();
            }
            else
            {
                // handle error
            }
        });
    }
};

int main(int argc, char** argv)
{
    try
    {
        Sound_system ssys;
        std::cout << "##############" << std::endl;

        std::string client_name = "guest: ";
        int volume = 128;
        load_config(client_name, volume);
        if(argc > 1)
        {
            client_name = std::string(argv[1]) + ": ";
            save_config(client_name, volume);
        }

        std::unique_ptr<Sample> msg_sound;
        try
        {
            msg_sound = std::make_unique<Sample>("res/sfx_sounds_button6.wav", volume);
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        std::cout << "set new name by passing it as an argument when invoking program\n"
                     "type ~<0-128> to set sound volume\n"
                     "all changes are saved to res/client_config\n"
                     "(relative to application dir)\n"
                     "your name: " + std::string(client_name, 0, client_name.size() - 2) << std::endl;

        asio::io_service service;
        tcp::resolver resolver(service);
        // change localhost to server name
        auto endp_it = resolver.resolve({"matitechno.hopto.org", "6969"});
        Client client(service, endp_it, ssys, &*msg_sound);
        auto fut = std::async(std::launch::async, [&service](){service.run();});
        (void)fut;

        std::vector<char> msg(255, 0);
        std::memcpy(msg.data(), client_name.data(), client_name.size());
        while(std::cin.getline(msg.data() + client_name.size(), 255 - client_name.size()))
        {
            if(*(msg.data() + client_name.size()) == '~')
            {
                msg_sound->set_volume(std::atoi(msg.data() + client_name.size() + 1));
                std::cout << "new volume set: " << msg_sound->get_volume() << std::endl;
                save_config(client_name, msg_sound->get_volume());
            }
            else
                client.write(msg);
        }
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}

void save_config(const std::string& client_name, int volume)
{
    std::ofstream file("res/client_config");
    if(file.is_open())
        file << std::string(client_name, 0, client_name.size() - 2) << '\n' << volume;
    else
        std::cout << "save error: res/client_config" << std::endl;
}

void load_config(std::string& client_name, int& volume)
{
    std::ifstream file("res/client_config");
    if(file.is_open())
    {
        if(!(file >> client_name >> volume))
        {
            std::cout << "format error: res/client_configr" << std::endl;
        }
        else
            client_name += ": ";
    }
    else
        std::cout << "file open error: res/client_config" << std::endl;
}
