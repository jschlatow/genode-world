/*
 * \brief  Local service list provider for mDNS.
 * \author Nikolas Brendes
 * \date   2018-07-01
 */

#include <base/component.h>
#include <base/log.h>
#include <base/env.h>
#include <base/heap.h>
#include <base/exception.h>
#include <os/reporter.h>
#include <net/ipv4.h>

#include <mdns/mDNS.h>
#include <mdns/Resource_Record.h>

namespace local_srvc_provider
{
    using Genode::env;
    using Net::mDNS::service_list;
    using Genode::Reporter;
    using Net::Ipv4_packet;
    using Net::Ipv4_address;
    using Genode::uint8_t;


    class Main;
};

class local_srvc_provider::Main
{
    
    
    Genode::Env         &_env;
    Genode::Heap 		heap = {&_env.ram(), &_env.rm()};
    Genode::Reporter    _reporter {_env, "local_services"};

    service_list   local_list;
  
    void load_config()
    {
        try
        {
            Genode::Attached_rom_dataspace config = {_env, "config"};
            
            config.xml().for_each_sub_node("service",[&] (Genode::Xml_node sub_node){
			
			    char buffer[255] = {0};

    			sub_node.attribute("name").value(buffer, 255);

                Net::mDNS::Domain_Name_Ptr dName((const char*)buffer);
                const char* str = ".genode.local";
                Net::mDNS::Domain_Name_Ptr d0(str);

                if(dName.match(d0) == 0)
                {
                    Net::mDNS::Name n(buffer);
                    local_list.add(Ipv4_packet::current(),n);
                }
                else
                {
                    Genode::strncpy(buffer + sub_node.attribute("name").value_size(), str, 14);
                    Net::mDNS::Name n(buffer);
                    local_list.add(Ipv4_packet::current(),n);
                }
		    });
        }
        catch(...)
        {
        }
        
    }

  public:

    Main(Genode::Env &env) : _env(env) , local_list(&heap)
    {
        load_config();
        local_list.report(_reporter);
    }
};

void Component::construct(Genode::Env &env)
{
	//env.exec_static_constructors();

	static local_srvc_provider::Main main(env);
}
