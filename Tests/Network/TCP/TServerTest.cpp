#include "../../../cpp/NetworkServer.hpp"

int main()
{
    hsd::tcp::server server{hsd::tcp::protocol_type::ipv4, 54000, "0.0.0.0"};
    hsd::tcp::received_state code;
    int i = -3;

    while(true)
    {
        auto [buf, code] = server.receive();
        
        if(code == hsd::tcp::received_state::ok)
        {
            hsd::io::print("CLIENT> {}\n", buf.data());
            server.respond("Good\n");
        }
        
        if(buf.to_string() == "exit")
            break;

        if(code != hsd::tcp::received_state::ok)
            break;
    }
}