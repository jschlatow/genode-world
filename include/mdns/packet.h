/*
 * \brief  mDNS packet
 * \author Nikolas Brendes
 * \date   2018-07-01
 */

#include <net/ipv4.h>
#include <net/ethernet.h>
#include <base/env.h>

namespace Net
{
    namespace mDNS
    {
        class Packet : private Ethernet_frame, private Ipv4_packet
        {
            private:
            
            Genode::uint16_t random_id()
	        {
		        static Genode::uint16_t seed = 93186752;
		        const Genode::uint16_t a = 1588635695, q = 2, r = 1117695901;
		        seed = a * (seed % q) - r * (seed / q);
		        return seed;
	        }

        	//UDP Part of Packet:
        	Genode::uint16_t		_src_port = host_to_big_endian((Genode::uint16_t)5353);
        	Genode::uint16_t		_dst_port = host_to_big_endian((Genode::uint16_t)5353);
        	Genode::uint16_t		_length;			// = length of UDP ( 8 ) + Length of mDNS
        	Genode::uint16_t		_checksum;

        	//mDNS Part of Packet:
        	Genode::uint16_t		_trans_id = 0;
        	Genode::uint16_t		_flags = 0;
        	Genode::uint16_t		_questions = 0;
        	Genode::uint16_t		_answere_rr = 0;
        	Genode::uint16_t		_authority_rr = 0;
        	Genode::uint16_t		_additional_rr = 0;


        	/*****************************************************
        	** 'data' must be the last member of this class **
        	*****************************************************/
			
          public:

        	char			data[0];

        	Packet() : Ethernet_frame(),  Ipv4_packet() { _trans_id = random_id(); }

			Genode::uint16_t	port_src()			const { return host_to_big_endian(_src_port); }
			Genode::uint16_t	port_dst()			const { return host_to_big_endian(_dst_port); }

        	Genode::int16_t		length()			const { return host_to_big_endian(_length); }		
        	Genode::uint16_t	checksum()			const { return host_to_big_endian(_checksum); }

        	Genode::uint16_t	trans_id()			const { return host_to_big_endian(_trans_id); }
        	Genode::uint16_t	flags()				const { return host_to_big_endian(_flags); }
        	Genode::uint16_t	questions()			const { return host_to_big_endian(_questions); }
        	Genode::uint16_t	answere_rr() 		const { return host_to_big_endian(_answere_rr); }
        	Genode::uint16_t	authority_rr() 		const { return host_to_big_endian(_authority_rr); }
        	Genode::uint16_t	additional_rr() 	const { return host_to_big_endian(_additional_rr); }


        	void	length(Genode::uint16_t v)			 { _length = host_to_big_endian((Genode::uint16_t)v); }		
        	void	checksum(Genode::uint16_t v)		 { _checksum = host_to_big_endian((Genode::uint16_t)v); }

        	void	trans_id(Genode::uint16_t v)		 { _trans_id = host_to_big_endian((Genode::uint16_t)v); }	
        	void	flags(Genode::uint16_t v)			 { _flags = host_to_big_endian((Genode::uint16_t)v); }
        	void	questions(Genode::uint16_t v)		 { _questions = host_to_big_endian((Genode::uint16_t)v); }
        	void	answere_rr(Genode::uint16_t v) 		 { _answere_rr = host_to_big_endian((Genode::uint16_t)v); }
        	void	authority_rr(Genode::uint16_t v) 	 { _authority_rr = host_to_big_endian((Genode::uint16_t)v); }
        	void	additional_rr(Genode::uint16_t v) 	 { _additional_rr = host_to_big_endian((Genode::uint16_t)v); }

        	/**
        		 * Return size of the packet
        		 */
        	Genode::size_t size() const { return sizeof(Packet); }



        	void prepare_net(Ipv4_address src_ip, Mac_address src_mac)
        	{
                Ethernet_frame::src(src_mac);
        		Ethernet_frame::dst(Net::mac_from_string("FF:FF:FF:FF:FF:FF"));
				//Ethernet_frame::dst(Net::mac_from_string("01:00:5E:00:00:FB"));
        		Ethernet_frame::type(Ethernet_frame::Type::IPV4);

        		Ipv4_packet::version(4);
        		Ipv4_packet::header_length(5);
        		Ipv4_packet::time_to_live(255);
        		Ipv4_packet::protocol(Ipv4_packet::Protocol::UDP);
        		Ipv4_packet::src(src_ip);
        		Ipv4_packet::dst(Ipv4_packet::ip_from_string("224.0.0.251"));
        		Ipv4_packet::total_length(sizeof(Packet) - sizeof(Ethernet_frame) + length() -20);
        		Ipv4_packet::update_checksum();
        	}


        	/**
        	* Placement new.
        	*/
        	
            void * operator new(__SIZE_TYPE__ size, void* addr) { return addr; }
        };


    };
}
