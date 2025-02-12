#pragma once

#include "Io.hpp"
#include "_Define.hpp"

#if defined(HSD_PLATFORM_POSIX)
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <memory.h>
#include <errno.h>
#else
#include <unistd.h>
#include <ws2tcpip.h>
#endif

namespace hsd
{
    namespace net
    {
        enum class received_state
        {
            err = -1,
            disconnected,
            ok
        };

        enum class protocol_type
        {
            ipv4 = AF_INET,
            ipv6 = AF_INET6,
            unspec = AF_UNSPEC
        };

        #if defined(HSD_PLATFORM_POSIX)
        struct socket_type
        {
            static constexpr usize stream = SOCK_STREAM;
            static constexpr usize clo_exec = SOCK_CLOEXEC;
            static constexpr usize dccp = SOCK_DCCP;
            static constexpr usize dgram = SOCK_DGRAM;
            static constexpr usize no_block = SOCK_NONBLOCK;
            static constexpr usize packet = SOCK_PACKET;
            static constexpr usize raw = SOCK_RAW;
            static constexpr usize rdm = SOCK_RDM;
            static constexpr usize seq_packet = SOCK_SEQPACKET;
        };
        #else
        struct socket_type
        {
            static constexpr usize stream = SOCK_STREAM;
            static constexpr usize dgram = SOCK_DGRAM;
            static constexpr usize raw = SOCK_RAW;
            static constexpr usize rdm = SOCK_RDM;
            static constexpr usize seq_packet = SOCK_SEQPACKET;
        };
        #endif
    } // namespace net

    namespace network_detail
    {
        #if defined(HSD_PLATFORM_WINDOWS)
        inline void init_winsock()
        {
            static bool _is_init = false;
            
            if (_is_init == false)
            {
                WSAData _data;
	            WORD _ver = MAKEWORD(2, 2);
	            i32 _ws_result = WSAStartup(_ver, &_data);
    
                if (_ws_result != 0)
	            {
	            	hsd::io::err_print<
                        "Can't start Winsock, here's"
                        " a irrelevant number #{}\n"
                    >(_ws_result);
                    return;
	            }

                _is_init = true;
            }
        }
        #endif
    } // namespace network_detail
} // namespace hsd