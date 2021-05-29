#pragma once

#include "_NetworkDetail.hpp"
#include "Io.hpp"

namespace hsd
{
    namespace udp
    {
        namespace server_detail
        {
            using protocol_type = net::protocol_type;

            class socket
            {
            private:
                #if defined(HSD_PLATFORM_POSIX)
                i32 _listening = 0;
                #else
                SOCKET _listening = 0;
                #endif

                sockaddr_storage _peer_addr{};
                net::protocol_type _protocol;

            public:
                inline socket(protocol_type protocol = protocol_type::ipv4, 
                    const char* ip_addr = "0.0.0.0:54000")
                {
                    #if defined(HSD_PLATFORM_WINDOWS)
                    network_detail::init_winsock();
                    #endif

                    switch_to(protocol, ip_addr);
                }

                inline ~socket()
                {
                    close();
                }

                inline void close()
                {
                    #if defined(HSD_PLATFORM_POSIX)
                    ::close(_listening);
                    #else
                    ::closesocket(_listening);
                    #endif
                }

                inline i32 get_listening()
                {
                    return _listening;
                }

                inline void switch_to(net::protocol_type protocol, const char* ip_addr)
                {
                    close();
                    _protocol = protocol;
                    addrinfo* _result = nullptr;
                    addrinfo* _rp = nullptr;

                    auto _hints = addrinfo
                    {
                        .ai_flags = AI_PASSIVE,
                        .ai_family = static_cast<i32>(protocol),
                        .ai_socktype = net::socket_type::dgram,
                        .ai_protocol = 0,
                        .ai_addrlen = 0,
                        .ai_addr = nullptr,
                        .ai_canonname = nullptr,
                        .ai_next = nullptr
                    };

                    const char* _domain_port = 
                        cstring::find(ip_addr, ':');

                    if (_domain_port != nullptr)
                    {
                        char* _domain_addr = nullptr;
                        auto _domain_len = static_cast<usize>(
                            _domain_port - ip_addr
                        );

                        _domain_addr = new char[_domain_len + 1];
                        cstring::copy(_domain_addr, ip_addr, _domain_len);
                        _domain_addr[_domain_len] = static_cast<char>(0);
                        _domain_port += 1;

                        i32 _error_code = getaddrinfo(
                            _domain_addr, _domain_port, &_hints, &_result
                        );

                        if (_error_code != 0) 
                        {
                            fprintf(
                                stderr, "Error for getaddrinfo, code"
                                ": %s\n", gai_strerror(_error_code)
                            );
                            return;
                        }

                        delete[] _domain_addr;
                    }
                    else
                    {
                        i32 _error_code = getaddrinfo(
                            ip_addr, nullptr, &_hints, &_result
                        );
    
                        if (_error_code != 0) 
                        {
                            fprintf(
                                stderr, "Error for getaddrinfo, code"
                                ": %s\n", gai_strerror(_error_code)
                            );
                            return;
                        }
                    }

                    for (_rp = _result; _rp != nullptr; _rp = _rp->ai_next)
                    {
                        _listening = ::socket(
                            _rp->ai_family, _rp->ai_socktype, _rp->ai_protocol
                        );

                        if (_listening != -1)
                        {
                            if (bind(_listening, _rp->ai_addr, _rp->ai_addrlen) == 0)
                                break;

                            close();
                        }
                    }

                    freeaddrinfo(_result);

                    if (_rp == nullptr)
                    {
                        fprintf(stderr, "Could not bind\n");
                        return;
                    }
                }
            };
        } // namespace server_detail

        class server
        {
        private:
            server_detail::socket _sock;
            sockaddr_storage _hint{};
            socklen_t _len = sizeof(sockaddr_storage);
            net::protocol_type _protocol;
            hsd::sstream _net_buf{4095};

            inline void _clear_buf()
            {
                memset(_net_buf.data(), '\0', 4096);
            }

        public:
            inline server() = default;

            inline server(net::protocol_type protocol, const char* ip_addr)
                : _sock{protocol, ip_addr}, _protocol{protocol}
            {}

            inline ~server()
            {
                respond<"">();
            }

            inline hsd::pair< hsd::sstream&, net::received_state > receive()
            {
                _clear_buf();
                isize _response = 0;
                
                if(_protocol == net::protocol_type::ipv4)
                {
                    _response = recvfrom(_sock.get_listening(), _net_buf.data(), 
                        4096, 0, bit_cast<sockaddr*>(&_hintv4), &_len);
                }
                else
                {
                    _response = recvfrom(_sock.get_listening(), _net_buf.data(), 
                        4096, 0, bit_cast<sockaddr*>(&_hintv6), &_len);
                }
                if (_response == static_cast<isize>(net::received_state::err))
                {
                    hsd::io::err_print<"Error in receiving\n">();
                    _clear_buf();
                    return {_net_buf, net::received_state::err};
                }
                if (_response == static_cast<isize>(net::received_state::disconnected))
                {
                    hsd::io::err_print<"Client disconnected\n">();
                    _clear_buf();
                    return {_net_buf, net::received_state::disconnected};
                }

                return {_net_buf, net::received_state::ok};
            }

            template< basic_string_literal fmt, typename... Args >
            requires (IsSame<char, typename decltype(fmt)::char_type>)
            inline net::received_state respond(Args&&... args)
            {
                _clear_buf();
                _net_buf.write_data<fmt>(forward<Args>(args)...);

                isize _response = 0;

                if(_protocol == net::protocol_type::ipv4)
                {
                    _response = sendto(_sock.get_listening(), _net_buf.data(), 
                        _net_buf.size(), 0, bit_cast<sockaddr*>(&_hintv4), _len);
                }
                else
                {
                    _response = sendto(_sock.get_listening(), _net_buf.data(), 
                        _net_buf.size(), 0, bit_cast<sockaddr*>(&_hintv6), _len);
                }
                if(_response == static_cast<isize>(net::received_state::err))
                {
                    hsd::io::err_print<"Error in sending\n">();
                    return net::received_state::err;
                }

                return net::received_state::ok;
            }
        };
    } // namespace udp

    namespace tcp
    {
        namespace server_detail
        {
            class socket_support
            {
            private:
                #if defined(HSD_PLATFORM_POSIX)
                i32 _listening = 0;
                #else
                SOCKET _listening = 0;
                #endif
                
                sockaddr_in6 _hintv6;
                sockaddr_in _hintv4;
                net::protocol_type _protocol;

            public:
                inline socket_support(net::protocol_type protocol = net::protocol_type::ipv4, 
                    u16 port = 54000, const char* ip_addr = "0.0.0.0")
                {
                    #if defined(HSD_PLATFORM_WINDOWS)
                    network_detail::init_winsock();
                    #endif
                    
                    switch_to(protocol, port, ip_addr);
                }

                inline ~socket_support()
                {
                    close();
                }

                inline void close()
                {
                    #if defined(HSD_PLATFORM_POSIX)
                    ::close(_listening);
                    #else
                    ::closesocket(_listening);
                    #endif
                }

                inline i32 get_listener()
                {
                    return _listening;
                }

                inline void switch_to(net::protocol_type protocol, 
                    u16 port, const char* ip_addr)
                {
                    close();
                    _protocol = protocol;

                    if(_protocol == net::protocol_type::ipv4)
                    {
                        _listening = ::socket(static_cast<i32>(_protocol), net::socket_type::stream, 0);
                        _hintv4.sin_family = static_cast<u16>(_protocol);
                        _hintv4.sin_port = htons(port);
                        inet_pton(static_cast<i32>(_protocol), ip_addr, &_hintv4.sin_addr);
                        bind(_listening, bit_cast<sockaddr*>(&_hintv4), sizeof(_hintv4));
                        listen(_listening, SOMAXCONN);
                    }
                    else
                    {
                        _listening = ::socket(static_cast<i32>(_protocol), net::socket_type::stream, 0);
                        _hintv6.sin6_family = static_cast<u16>(_protocol);
                        _hintv6.sin6_port = htons(port);
                        inet_pton(static_cast<i32>(_protocol), ip_addr, &_hintv4.sin_addr);
                        bind(_listening, bit_cast<sockaddr*>(&_hintv6), sizeof(_hintv6));
                        listen(_listening, SOMAXCONN);
                    }
                }
            };

            class socket
            {
            private:
                #if defined(HSD_PLATFORM_POSIX)
                i32 _sock_value = 0;
                #else
                SOCKET _sock_value = 0;
                #endif

                socket_support _sock;
                sockaddr_in _sock_hintv4;
                sockaddr_in6 _sock_hintv6;
                socklen_t _size = 0;
                string _host{NI_MAXHOST - 1};
                string _service{NI_MAXSERV - 1};

            public:
                inline socket()
                {
                    switch_to(net::protocol_type::ipv4, 54000, "0.0.0.0");
                }

                inline socket(net::protocol_type protocol, u16 port, const char* ip_addr)
                {
                    switch_to(protocol, port, ip_addr);
                }

                inline ~socket()
                {
                    close();
                }

                inline void close()
                {
                    #if defined(HSD_PLATFORM_POSIX)
                    ::close(_sock_value);
                    #else
                    ::closesocket(_sock_value);
                    #endif
                }

                inline i32 get_sock()
                {
                    return _sock_value;
                }

                inline void switch_to(net::protocol_type protocol, 
                    u16 port, const char* ip_addr)
                {
                    _sock.switch_to(protocol, port, ip_addr);
                    memset(_host.data(), '\0', NI_MAXHOST);
                    memset(_service.data(), '\0', NI_MAXSERV);

                    if(protocol == net::protocol_type::ipv4)
                    {
                        _size = sizeof(_sock_hintv4);
                        _sock_value = accept(_sock.get_listener(), bit_cast<sockaddr*>(&_sock_hintv4), &_size);

                        if(getnameinfo(bit_cast<sockaddr*>(&_sock_hintv4), sizeof(_sock_hintv4), 
                            _host.data(), NI_MAXHOST, _service.data(), NI_MAXSERV, 0) == 0)
                        {
                            io::print<"{} connected on port {}\n">(ip_addr, ntohs(_sock_hintv4.sin_port));
                        }
                        else
                        {
                            inet_ntop(static_cast<i32>(protocol), &_sock_hintv4.sin_addr, _host.data(), NI_MAXHOST);
                            hsd::io::print<"{} connected on port {}\n">(ip_addr, ntohs(_sock_hintv4.sin_port));
                        }
                    }
                    else
                    {
                        _size = sizeof(_sock_hintv6);
                        _sock_value = accept(_sock.get_listener(), bit_cast<sockaddr*>(&_sock_hintv6), &_size);

                        if(getnameinfo(bit_cast<sockaddr*>(&_sock_hintv6), sizeof(_sock_hintv6), 
                                _host.data(), NI_MAXHOST, _service.data(), NI_MAXSERV, 0) == 0)
                        {
                            io::print<"{} connected on port {}\n">(ip_addr, _service.data());
                        }
                        else
                        {
                            inet_ntop(static_cast<i32>(protocol), &_sock_hintv6.sin6_addr, _host.data(), NI_MAXHOST);
                            hsd::io::print<"{} connected on port {}\n">(ip_addr, ntohs(_sock_hintv6.sin6_port));
                        }
                    }

                    _sock.close();
                }
            };
        } // namespace server_detail

        class server
        {
        private:
            server_detail::socket _sock;
            hsd::sstream _net_buf{4095};

            inline void _clear_buf()
            {
                memset(_net_buf.data(), '\0', 4096);
            }

        public:
            inline server() = default;
            inline ~server() = default;

            inline server(net::protocol_type protocol, u16 port, const char* ip_addr)
                : _sock{protocol, port, ip_addr}
            {}

            inline hsd::pair< hsd::sstream&, net::received_state > receive()
            {
                _clear_buf();
                isize _response = recv(_sock.get_sock(), 
                    _net_buf.data(), 4096, 0);

                if (_response == static_cast<isize>(net::received_state::err))
                {
                    hsd::io::err_print<"Error in receiving\n">();
                    _clear_buf();
                    return {_net_buf, net::received_state::err};
                }
                if (_response == static_cast<isize>(net::received_state::disconnected))
                {
                    hsd::io::err_print<"Client disconnected\n">();
                    _clear_buf();
                    return {_net_buf, net::received_state::disconnected};
                }

                return {_net_buf, net::received_state::ok};
            }

            template < basic_string_literal fmt, typename... Args >
            requires (IsSame<char, typename decltype(fmt)::char_type>)
            inline net::received_state respond(Args&&... args)
            {
                _clear_buf();
                _net_buf.write_data<fmt>(forward<Args>(args)...);

                isize _response = send(_sock.get_sock(), 
                    _net_buf.data(), _net_buf.size(), 0);

                if(_response == static_cast<isize>(net::received_state::err))
                {
                    hsd::io::err_print<"Error in sending\n">();
                    return net::received_state::err;
                }

                return net::received_state::ok;
            }
        };
    } // namespace tcp
} // namespace hsd