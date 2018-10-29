/*
 * \brief  Service list for mDNS.
 * \author Nikolas Brendes
 * \date   2018-07-01
 */

#include <base/env.h>
#include <base/heap.h>
#include <base/attached_rom_dataspace.h>

#include <os/reporter.h>

#include <util/xml_node.h>
#include <util/list.h>

#include <net/ipv4.h>
#include <mDNS/Resource_Record.h>

namespace Net
{
    namespace mDNS
    {
        enum {
            MAX_LABEL_NUM = 32
        };

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



        class service_item : public Genode::List<service_item>::Element
        {
            


            Name              _name;
            Genode::uint8_t   _strike = 0;
            Genode::uint16_t  _ttl = 255;
            Genode::uint16_t  _ttl0 = 255;
           
            Net::Ipv4_address _ip;
          
          public:

            typedef enum {
                PROBING_0   = 1,
                PROBING_1   = 2,
                PROBING_2   = 3,
                PROBING_3   = 11,
                ANNOUNCE    = 4,
                TTL_0       = 5,
                TTL_1       = 6,
                TTL_2       = 7,
                TTL_3       = 8,
                ONLINE      = 9,
                FLUSH       = 10
            } State;

            Genode::uint8_t   _probing = 0;  
            Name * name() { return &_name; }
            Ipv4_address* ip() { return &_ip; }

            void ttl(Genode::uint16_t t) { _ttl = t; _strike = 0; }
            bool ttl_chk(Genode::uint8_t decrement_sec = 1)
            {
                if(_ttl > decrement_sec)
                    _ttl -= decrement_sec;

                Genode::uint8_t prozent = (Genode::uint8_t) ((_ttl0 * 100 - _ttl * 100) / _ttl0) + random(4);
                
                
                
                if(
                    (_strike == 0 && prozent > 80) ||
		            (_strike == 1 && prozent > 85) ||
		            (_strike == 2 && prozent > 90) ||
                    (_strike == 3 && prozent > 95) 
                )       
                { 
                    _strike++;
                    Genode::log(" >>", _strike, " ", prozent, "%  ",_ip, "  ", _name.string());
                    return false; 
                }
                else
                { 
                    return true; 
                }
            }
            void add_strike()
            {
                _strike++;
            }

            bool valid() 
            { 
                if(_strike > 3 || _ttl == 0)
                {   return false; }
                else
                {   return true; }
                 
            }
            
            service_item() {}
            
            service_item(Net::Ipv4_address ip, Name name) 
            : _name(name), _ip(ip)
            { }

            service_item(Net::Ipv4_address ip, Name name, Genode::uint32_t ttl) 
            : _name(name), _ttl(ttl), _ttl0(ttl), _ip(ip)
            { }
            
            void *operator new(__SIZE_TYPE__ size, void *addr) { 
                for(unsigned int i = 0; i < size; i++)
                    *((char*)addr) = 0;
                return addr; }
        };
        
        class service_list
        {
          private:  
            char _ip_str[15];
            void convert(Net::Ipv4_address ip)
            {
                
                Genode::uint8_t index = 0;
                for (int i = 0; i < 4; i++)
                {
                    if (i > 0)
                    {
                        _ip_str[index] = 46;
                        index++;
                    }
                    if (ip.addr[i] > 99)
                    {
                        _ip_str[index] = 48 + (ip.addr[i] / 100);
                        index++;
                        _ip_str[index] = 48 + (ip.addr[i] % 100) / 10;
                        index++;
                        _ip_str[index] = 48 + (ip.addr[i] % 10);
                        index++;
                    }
                    else if (ip.addr[i] > 9)
                    {
                        _ip_str[index] = 48 + (ip.addr[i] / 10);
                        index++;
                        _ip_str[index] = 48 + (ip.addr[i] % 10);
                        index++;
                    }
                    else
                    {
                        _ip_str[index] = 48 + (ip.addr[i]);
                        index++;
                    }
                }
            }

	        Genode::Heap 					*_heap;

            void * alloc(Genode::size_t s)
	        {
	        	void *p = 0;
	        	_heap->alloc(s,&p);
	        	return p;

                
	        }

          public:
            

            Genode::List<service_item>      _list;
            Genode::uint8_t _count;
            bool            update = false;
            service_list(Genode::Heap *heap) :_heap(heap) {}

            service_list(Genode::Heap *heap, Genode::Xml_node xml) :_heap(heap)
            {
                parse(xml);
                
            }
            ~service_list()
            {
                while(service_item *i = _list.first())
		        {
			        _list.remove(i);
                    _heap->free(i, sizeof(service_item));
		        }
                
            }
            service_item* first()
            { 
                return _list.first(); 
            }
            
            void remove_invalid()
            {
                for(service_item *i = first(); i; i = i->next())
				{
					if( !i->valid())
					{
						service_item *si = i->next();
						remove(i);
						i = si;
						if(!i)
							break;
                    }
				}
            }

            void remove(service_item* i)
            {
                update = true;
                _list.remove(i);
                _heap->free(i, sizeof(service_item));
                _count--;
            }

            void add(Net::Ipv4_address ip, Name name, Genode::uint16_t ttl = 255)
            {   
                update = true;
                 _list.insert(new(alloc(sizeof(service_item))) service_item(ip, name, ttl));
                _count++;
            }

            void parse(Genode::Xml_node xml)
            {
                Genode::log("Start parsing XML ",xml.num_sub_nodes());
                char            buffer[MAX_NAME_LEN];
                Ipv4_address    ip;

                xml.for_each_sub_node("item",[&] (Genode::Xml_node sub_node){
			
		    	    sub_node.attribute("ip").value(buffer,15);
                    ip = Ipv4_packet::ip_from_string(buffer);
                    sub_node.attribute("name").value(buffer, 255);
                    add(ip, Name((const char*)buffer));
                    
		        });
            }

            service_item * existing(Ipv4_address *ip, Name *name)
            {
                for(service_item *i = _list.first(); i; i = i->next())
                {
                    if( Domain_Name_Ptr(i->name()->string() ).match(Domain_Name_Ptr(name->string())) == 1 
                        && *(i->ip()) == *ip)
                    {
                        return i;
                    }
                }
                return 0;
            }

            bool lookup_rr(Resource_Record *rr)
            {
                for(service_item *i = _list.first(); i; i = i->next())
                {
                    if( Domain_Name_Ptr(i->name()->string()).match(Domain_Name_Ptr(rr->name()->string())) == 1 
                        && *i->ip() == Ipv4_address(rr->_data))
                    {
                        i->ttl(rr->ttl());
                        return true;
                    }
                }
                return false;
            }

            void report(Genode::Reporter &reporter)
            {
                if(update)
                {
                    reporter.enabled(true);
                    reporter.clear();
                    Genode::Reporter::Xml_generator xml(reporter, [&]() {

                        for(service_item* i = _list.first(); i; i = i->next())
                        {
                            convert(*i->ip());
                            xml.node("item", [&]() {
                                xml.attribute("ip", _ip_str);
                                xml.attribute("name", i->name()->string());
                            });
                        }

                    });
                }
                update = false;   
            }
        };


        
    }; // namespace mDNS
} // namespace Net
