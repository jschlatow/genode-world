/*
 * \brief  Publish local services via mDNS and report detected remote services.
 * \author Nikolas Brendes
 * \date   2018-07-01
 */

#include <base/component.h>
#include <base/log.h>
#include <timer_session/connection.h>
#include <base/env.h>
#include <base/heap.h>
#include <base/exception.h>
#include <base/attached_rom_dataspace.h>
#include <util/endian.h>
#include <os/reporter.h>

#include <nic/packet_allocator.h>
#include <nic_session/connection.h>

#include <net/ethernet.h>
#include <net/ipv4.h>

#include <mdns/Resource_Record.h>
#include <mdns/mDNS.h>

#include <mdns/packet.h>
//#include <net.h>

namespace mDNS
{

	using Genode::Cstring;
	using Genode::Heap;
	using Genode::Packet_descriptor;
	using Genode::env;

	using Genode::Reporter;

	using Net::mDNS::Class;
	using Net::mDNS::QClass;
	using Net::mDNS::Type;
	using Net::mDNS::QType;

	using Net::mDNS::Name;
	using Net::mDNS::service_list;
	using Net::mDNS::service_item;
	using Net::mDNS::Question_Ptr;
	using Net::mDNS::Question;

	using Net::mDNS::Domain_Name_Ptr;
	using Net::mDNS::Resource_Record;
	
	using Net::mDNS::Packet;

	using Genode::size_t;
	using Genode::uint16_t;
	using Genode::uint32_t;
	using Genode::uint8_t;
	using Genode::int16_t;
	using Genode::int32_t;
	using Genode::int8_t;
	using Net::Ethernet_frame;
	using Net::Ipv4_address;
	using Net::Ipv4_packet;
	using Net::Mac_address;
	
	using Genode::Reporter;


	using Genode::Attached_rom_dataspace;

	
	class Packet;
	struct Type;
	struct QType;
	struct Class;
	struct QClass;
	class Resource_Record;
	class RR_A;
	class Question;
	class Label;
	class Domain_Name;

	class Root;
	class Main;

	enum{
		MAX_NAME_LEN = 32,
		MAX_PACK_SIZE = 1400
	};


	//Mac_address 	my_MAC;
	

	char 			node_name[MAX_NAME_LEN];
	char			rom_list_pd_name[64];
	bool 			verbose;

	//Not supported by NIC, using Broadcast MAC instead
	const Mac_address 	_dst_mac_brdc = Net::mac_from_string("FF:FF:FF:FF:FF:FF");
	const Mac_address 	_dst_mac_ipv4 = Net::mac_from_string("01:00:5E:00:00:FB");
	const Mac_address 	_dst_mac_ipv6 = Net::mac_from_string("33:33:00:00:00:FB");
	const Ipv4_address 	_dst_ip_ipv4  = Ipv4_packet::ip_from_string("224.0.0.251");
	

	unsigned int random(unsigned int max = 0)
	{
		static unsigned int seed = 93186752;
		const unsigned int a = 1588635695, q = 2, r = 1117695901;
		seed = a * (seed % q) - r * (seed / q);
		if(max == 0)
			return seed;
		else
			return seed % max;
	}

};








struct mDNS::Main : public Ethernet_frame, public Ipv4_packet
{
	enum
	{
		PACKET_SIZE = 1024,
		BUF_SIZE = Nic::Session::QUEUE_SIZE * PACKET_SIZE,
		MAX_NAME_LEN = 32,
		TIME_INTERVAL_SEC = 1
	};

	typedef enum{
		ZERO 				= 0,
		START 				= 1,
		PROBING 			= 2,
		ANNOUNCING 			= 3,
		ONLINE 				= 4,
		CHK_ONLINE_0		= 6,
		CHK_ONLINE_1		= 5	

	} State;


	Genode::Env 					&_env;
	Genode::Heap 					heap = {&_env.ram(), &_env.rm()};
	Timer::Connection 				_timer{_env};
	Timer::Connection 				_timer_tx{_env};
	Timer::Connection 				_timer_tll{_env};
	Nic::Packet_allocator 			_tx_block_alloc;
	Nic::Connection 				_nic;

	Genode::Signal_handler<Main> 	_sig_tx_handler{_env.ep(),*this, &Main::tx_handler};
	Genode::Signal_handler<Main> 	_sig_ttl_timer{_env.ep(),*this, &Main::ttl_handler};
	Genode::Signal_handler<Main> 	_list_handler{_env.ep(), *this, &Main::local_list_handler};
	Genode::Attached_rom_dataspace	_local_service_report;

	Genode::Reporter    			_reporter {_env, "remote_services"};
	service_list 					local_list = {&heap};
	service_list					remote_list = {&heap};
	State							_state;

	Genode::uint16_t				_trans_id[5];
	Genode::List<Resource_Record>	r_list;
	Genode::List<Resource_Record>	list_tx_r;
	Genode::List<Resource_Record>	list_rx_r;
	Genode::List<Question>			list_tx_q;

	Genode::List<Question>			q_list;


	void ttl_handler()
	{
		// Decrement all RRs in Remote List and add Query to TX List when out of TTL
		for(service_item *i = remote_list.first(); i; i = i->next())
		{
			if(!i->ttl_chk())
			{
				Question* q = new(alloc(sizeof(Question))) Question(&heap, *i->name(), Net::mDNS::QType::A, Net::mDNS::QClass::CH );
				list_tx_q.insert(q);
			}
		}
		remote_list.remove_invalid();
	}

	void local_list_handler()
	{
		Genode::log("local_list_handler");
		// If firststart sigh TX Handler 
		if(_state == START)
		{
			_timer_tx.sigh(_sig_tx_handler);
		}

		// Add all Items to Temp List
		service_list list_temp(&heap);
		_local_service_report.xml().for_each_sub_node("item",[&] (Genode::Xml_node sub_node){
			
			char str[255] = {0};
			Ipv4_address    ip;

			sub_node.attribute("ip").value(str,15);
			ip = Ipv4_packet::ip_from_string(str);
			for(int i = 0; i < 16; i++)
				str[i] = 0;
			sub_node.attribute("name").value(str, 255);
			
			list_temp.add(my_IP(), Name((const char*) str));
		});
		// Send TTL Zero RRs for removed Service Items and remove them from Local List
		for(service_item* i = local_list.first(); i; i = i->next())
		{
			if( !list_temp.existing(i->ip(), i->name()))
			{
				Resource_Record *rr = new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, *i->name(), *i->ip());
				rr->ttl(0);
				list_tx_r.insert(rr);

				service_item* si = i->next();
				local_list.remove(i);

				i = si;
				if(!i)
					break;	
			}
		}
		// Add new Items to local List and switch to Probing State
		bool newItems = false;
		for(service_item* i = list_temp.first(); i; i = i->next())
		{
			if( !local_list.existing(i->ip(), i->name()))
			{	
				local_list.add(*i->ip(), *i->name());
				newItems = true;
			}
		}
		
		if(newItems)
		{
			_timer_tx.trigger_once(1000 * random(250));
			_state = PROBING;
		}

	}


	class Rx_thread : public Genode::Thread
	{
	  protected:
		Nic::Connection &_nic;

		Main *_parent;
		Genode::Signal_receiver _sig_rec;
		Genode::Signal_dispatcher<Rx_thread> _link_state_dispatcher;
		Genode::Signal_dispatcher<Rx_thread> _rx_packet_avail_dispatcher;
		Genode::Signal_dispatcher<Rx_thread> _rx_ready_to_ack_dispatcher;

		void _handle_rx_packet_avail(unsigned)
		{
			while (_nic.rx()->packet_avail() && _nic.rx()->ready_to_ack())
			{
				Packet_descriptor _rx_packet = _nic.rx()->get_packet();
				char *content = _nic.rx()->packet_content(_rx_packet);
				_parent->rx_handler((Packet*)content);
				_nic.rx()->acknowledge_packet(_rx_packet);
			}
		}
		
		void _handle_rx_ready_to_ack(unsigned) { _handle_rx_packet_avail(0); }

		void _handle_link_state(unsigned)
		{
			Genode::log("link state changed");
		}

	  public:
		Rx_thread(Nic::Connection &nic, Main *parent)
			: Genode::Thread(Weight::DEFAULT_WEIGHT, "mDNS_nic_rx", 8192),
			  _nic(nic),
			  _parent(parent),
			  _link_state_dispatcher(_sig_rec, *this, &Rx_thread::_handle_link_state),
			  _rx_packet_avail_dispatcher(_sig_rec, *this, &Rx_thread::_handle_rx_packet_avail),
			  _rx_ready_to_ack_dispatcher(_sig_rec, *this, &Rx_thread::_handle_rx_ready_to_ack)

		{
			_nic.link_state_sigh(_link_state_dispatcher);
			_nic.rx_channel()->sigh_packet_avail(_rx_packet_avail_dispatcher);
			_nic.rx_channel()->sigh_ready_to_ack(_rx_ready_to_ack_dispatcher);
		}

		void entry()
		{
			while (true)
			{
				Genode::Signal sig = _sig_rec.wait_for_signal();
				int num = sig.num();

				Genode::Signal_dispatcher_base *dispatcher;
				dispatcher = dynamic_cast<Genode::Signal_dispatcher_base *>(sig.context());
				dispatcher->dispatch(num);
			}
		}
	};

	Rx_thread _rx_thread;
	
	bool chk_for_mDNS_packet(Packet* packet)
	{	
		Ipv4_packet * ipP = (Ipv4_packet*) packet;
		Ethernet_frame *etP = (Ethernet_frame*) packet;


		if( etP->src() != my_MAC() &&			  
			ipP->dst() == _dst_ip_ipv4 &&
			packet->port_dst() == 5353 && 
			packet->port_src() == 5353)
		{
			
			//Genode::log("got mDNS packet! ", Mac_address(((char*) packet)+6));
			return true;
		}
		return false;
	}
	
	void rx_handler(Packet* packet)
	{
		//Check for mDNS Packet
		if(!chk_for_mDNS_packet(packet))
			return;
		
		//Check for Truncated Packet
		if(packet->flags() & 0b0000001000000000)
		{
			_timer_tx.trigger_once(1000 * 500);
		}

		size_t offset = 0;
		Genode::List<Net::mDNS::Resource_Record> defend_List;
		
		for(int i = 0; i < packet->questions(); i++)
		{
			Question_Ptr QP(packet->data + offset);
			offset += QP.size();
			// Check Local List: Defend the Domain Name or add normal Answere to TX List
			for(service_item* i = local_list.first(); i; i = i->next())
			{	
				int8_t stat = Domain_Name_Ptr(QP.name()).match(Domain_Name_Ptr(i->name()->string())); //d2.match(d1);
				Genode::log("stat: ", (int)stat);
				if(QP.type() == QType::ANY && stat ==  1)
				{
					Genode::log("Defend: ", i->name()->string());
					Genode::log("Defend: ", QP.name());
					defend_List.insert(new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, *i->name(), *i->ip()));
				}
				else if( (QP.type() == QType::ANY && stat ==  0) || 
						 (QP.type() == QType::A   && stat ==  1) )
				{
					list_tx_r.insert(new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, *i->name(), *i->ip()));
				}
			}
			// Check Remote List: Add Strike to Service Item and try to remove Known Answeres
			for(service_item* i = remote_list.first(); i; i = i->next())
			{
				if(Domain_Name_Ptr(QP.name()).match(Domain_Name_Ptr(i->name()->string())) == 1)
				{

					i->add_strike();
					while(remove_known_question(&QP)) ;
				}
			}
		}
		// Send Defend List
		if(defend_List.first())
		{
			Genode::log("DEFEND LIST");
			char *buffer = (char*) alloc(1440);
			uint8_t rr_count = 0;
			size_t len = 0;
			for(Resource_Record *rr = defend_List.first(); rr; rr = rr->next())
			{
				if(len + rr->size() > 1440)
					{
						Nic::Packet_descriptor pd = alloc_tx_packet(sizeof(Packet) + len); 
						Packet *p = new (_nic.tx()->packet_content(pd)) Packet();
						p->trans_id(packet->trans_id());
						p->answere_rr(rr_count);
						p->length((20 + len));
						p->checksum(0x20FF);
						p->prepare_net(my_IP(),my_MAC());
						Genode::memcpy(p->data, buffer, len);
						submit_packet(pd);
						rr_count = 0;
						len = 0;
					}
				len += rr->copy(buffer + len);
				rr_count++;
			}
			Nic::Packet_descriptor pd = alloc_tx_packet(sizeof(Packet) + len); 
			Packet *p = new (_nic.tx()->packet_content(pd)) Packet();
			p->answere_rr(rr_count);
			p->length((20 + len));
			p->checksum(0x20FF);
			p->prepare_net(my_IP(),my_MAC());
			Genode::memcpy(p->data, buffer, len);
			submit_packet(pd);
			heap.free(buffer, 1440);	
			
			while(Resource_Record* r = defend_List.first())
			{
				defend_List.remove(r);
				r->~Resource_Record();
			}	
		}	
		// Copy RRs to RX List, so the TX Handler can use them
		for(int i = 0; i < packet->answere_rr(); i++)
		{
			list_rx_r.insert(new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, packet->data + offset));
			offset += list_rx_r.first()->size();
		}
	}
	
	

	void send_message()
	{
		char *buffer = (char*) alloc(1440);
		
		uint8_t q_count = 0;
		uint8_t rr_count = 0;
		size_t len = 0;
		for(Question *q = list_tx_q.first(); q; q = q->next())
		{
			if(len + q->size() > 1440)
				{
					Nic::Packet_descriptor pd = alloc_tx_packet(sizeof(Packet) + len); 
					Packet *p = new (_nic.tx()->packet_content(pd)) Packet();
					p->trans_id(_trans_id[0]);
					p->flags(0b0000010000000000);
					p->questions(q_count);
					p->length((20 + len));
					p->checksum(0x20FF);
					p->prepare_net(my_IP(),my_MAC());
					Genode::memcpy(p->data, buffer, len);
					submit_packet(pd);
					
					q_count = 0;
					len = 0;
				}
			len += q->copy(buffer + len);
			q_count++;
		}
		for(Resource_Record *rr = list_tx_r.first(); rr; rr = rr->next())
		{
			if(len + rr->size() > 1440)
				{
					Nic::Packet_descriptor pd = alloc_tx_packet(sizeof(Packet) + len); 
					Packet *p = new (_nic.tx()->packet_content(pd)) Packet();
					p->trans_id(_trans_id[0]);
					p->flags(0b1000010000000000);
					p->questions(q_count);
					p->answere_rr(rr_count);
					p->length((20 + len));
					p->checksum(0x20FF);
					p->prepare_net(my_IP(),my_MAC());
					Genode::memcpy(p->data, buffer, len);
					submit_packet(pd);
					rr_count = 0;
					len = 0;
				}
			len += rr->copy(buffer + len);
			rr_count++;
		}
		if(len == 0)
			return;
		Nic::Packet_descriptor pd = alloc_tx_packet(sizeof(Packet) + len); 
		Packet *p = new (_nic.tx()->packet_content(pd)) Packet();
		p->trans_id(_trans_id[0]);
		p->flags(0b0000000000000000);
		p->questions(q_count);
		p->answere_rr(rr_count);
		p->length((20 + len));
		p->checksum(0x20FF);
		p->prepare_net(my_IP(),my_MAC());
		Genode::memcpy(p->data, buffer, len);
		submit_packet(pd);
		heap.free(buffer, 1440);

		while(Question *q = list_tx_q.first())
		{
			list_tx_q.remove(q);
			q->~Question();
		}

		while(Resource_Record* r = list_tx_r.first())
		{
			list_tx_r.remove(r);
			r->~Resource_Record();
		}
	}

	void tx_handler()
	{	

		switch(_state)
		{
			case PROBING :
			{
				Genode::log("PROBING");
				//Check for Conflict
				for(Resource_Record* rr = list_rx_r.first(); rr; rr = rr->next())
				{
					Ipv4_address ip = my_IP(); 
					if(service_item * i = local_list.existing(&ip, rr->name()))
					{	
						Genode::warning("Item: ", i->name()->string(), " - ", *i->ip(), "  already existing! --> removing from Cache");
						local_list.remove(i);
					}
				}
				//Add Probe to TX List
				bool probe = false;
				for(service_item *i = local_list.first(); i; i = i->next())
				{	
					if(i->_probing < 3)
					{
						probe = true;
						i->_probing++;
						list_tx_q.insert( new(alloc(sizeof(Question))) Question(&heap, *i->name(), QType::ANY, QClass::CH));
					}	
				}
				
				//Switch State after all Probes 
				if(!probe)
				{
					_state = ANNOUNCING;
					tx_handler();
					return;
				}
				_timer_tx.trigger_once(1000 * 250);	
				break;
			}


			case ANNOUNCING :
			{
				Genode::log("ANNOUNCING");
				//Send all Service Item
				for(service_item *i = local_list.first(); i; i = i->next())
				{	
					list_tx_r.insert(new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, *i->name(), *i->ip()));
				}
				//Add ANY Question to get all RRs from other Hosts
				Name any("genode.local");
				list_tx_q.insert(new(alloc(sizeof(Question))) Question(&heap, any, QType::ANY, QClass::CH));

				_state = ONLINE;
				_timer_tx.trigger_once(1000 * 1000 * 1);
				break;
			}

			case ONLINE :
			{
				Genode::log("ONLINE ", remote_list._count);
				Ipv4_address myip = my_IP(); 
				for(Resource_Record* rr = list_rx_r.first(); rr; rr = rr->next())
				{	
					//Check for Conflict
					if(service_item * i = local_list.existing(&myip, rr->name()))
					{	
						Ipv4_address ip(i->ip());
						if( (myip.addr[0] + myip.addr[1] + myip.addr[2] + myip.addr[3] ) <
							(ip.addr[0] + ip.addr[1] + ip.addr[2] + ip.addr[3]) )
							_state = PROBING;
						else
						{
							list_tx_r.insert(new(alloc(sizeof(Resource_Record))) Resource_Record(&heap, *i->name(), *i->ip()));
						}
					}
					//Update Remote List or add new Item
					else if(!remote_list.lookup_rr(rr))
					{	
						char buffer[255] = {0};
						Domain_Name_Ptr(rr->name()->string()).to_str(buffer);
						remote_list.add(*(Ipv4_address*)rr->_data, buffer, rr->ttl());
					}
				}
				_timer_tx.trigger_once(1000 * 1000 * 1);
				break;
			}

			default :
			{
				Genode::warning("some sing went rong");
				break;
			}
		}

		while(remove_known_answere()) ;

		//Send Queue
		send_message();

		//Send Report when list changed
		remote_list.report(_reporter);

		//Clear RX List
		while(Resource_Record* r = list_rx_r.first())
		{
			list_rx_r.remove(r);
			r->~Resource_Record();
		}
	}
	
	bool remove_known_question(Question_Ptr* qp)
	{
		for(Question * q = list_tx_q.first(); q; q = q->next())
		{
			if(Domain_Name_Ptr(q->name()->string()).match(Domain_Name_Ptr(qp->name())) == 1)
			{
				list_tx_q.remove(q);
				q->~Question();
				return true;
			}
		}
		return false;
	}

	bool remove_known_answere()
	{
		for(Resource_Record* rr = list_rx_r.first(); rr; rr = rr->next())
		{	
			for(Resource_Record* tx_rr = list_tx_r.first(); tx_rr; tx_rr = tx_rr->next())
			{
				if(Domain_Name_Ptr(rr->name()->string()).match(Domain_Name_Ptr(tx_rr->name()->string())) == 1)
				{
					list_rx_r.remove(rr);
					rr->~Resource_Record();
					return true;
				}
			}
		}
		return false;
	}

	void * alloc(Genode::size_t s)
	{
		void *p = 0;
		heap.alloc(s,&p);
		return p;
	}

	Nic::Packet_descriptor alloc_tx_packet(Genode::size_t size)
	{
		while (true)
		{
			try
			{
				Nic::Packet_descriptor packet = _nic.tx()->alloc_packet(size);
				return packet;
			}
			catch (Nic::Session::Tx::Source::Packet_alloc_failed)
			{
				/* packet allocator exhausted, wait for acknowledgements */
				_tx_ack(true);
			}
		}
	}
	void _tx_ack(bool block = false)
	{
		/* check for acknowledgements */
		while (_nic.tx()->ack_avail() || block)
		{
			Nic::Packet_descriptor acked_packet = _nic.tx()->get_acked_packet();
			_nic.tx()->release_packet(acked_packet);
			block = false;
		}
	}
	void submit_packet(Nic::Packet_descriptor packet)
	{
		_nic.tx()->submit_packet(packet);
		_tx_ack();
		/* check for acknowledgements */
		while (_nic.tx()->ack_avail())
		{
			Nic::Packet_descriptor acked_packet = _nic.tx()->get_acked_packet();
			_nic.tx()->release_packet(acked_packet);
		}
	}

	Ipv4_address my_IP() { return Ipv4_address(Attached_rom_dataspace(_env, "akt_ip_rep").local_addr<void>());	}
	Mac_address my_MAC() { return _nic.mac_address(); }

	Main(Genode::Env &env) : Ethernet_frame(),
							 Ipv4_packet(),
							 _env(env),
							 _tx_block_alloc(&heap),
							 _nic(env, &_tx_block_alloc, BUF_SIZE, BUF_SIZE),
							 _local_service_report(env,"local_services"),
							 _rx_thread(_nic, this)
	{
		_timer_tll.sigh(_sig_ttl_timer);
		_timer_tll.trigger_periodic(1000 * 1000); // fire TTL Handler every secound


		
		_local_service_report.sigh(_list_handler);
		_rx_thread.start();
		_state = START;
	
		Genode::log("component constructed.");
	}
};
Genode::size_t 	Component::stack_size() { return 8*1024*sizeof(long); }
void 	Component::construct(Genode::Env &env)
{
	env.exec_static_constructors();

	Genode::Attached_rom_dataspace config = {env, "config"};
	try
	{			
		
		Genode::Xml_node xml = config.xml().sub_node("conf");
		xml.attribute("rom_list_pd").value(mDNS::rom_list_pd_name,sizeof(mDNS::rom_list_pd_name));

	}
	catch (...)
	{
		Genode::warning("Loading Config Failed!");
		
	}

	static mDNS::Main main(env);
}
