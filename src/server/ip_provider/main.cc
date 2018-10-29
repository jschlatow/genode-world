/*
 * \brief  IP address reporter for mDNS
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
#include <base/attached_rom_dataspace.h>

namespace ip_provider
{
    using Genode::env;
    
    using Genode::Reporter;
    using Net::Ipv4_packet;
    using Net::Ipv4_address;
    using Genode::uint8_t;

    class Main;
};

class ip_provider::Main
{
    
    Genode::Env         &_env;
    Genode::Reporter    _reporter {_env, "ip_report"};
    Net::Ipv4_address   _ip;

    char                _ip_str[15];
    uint8_t             _len;

 
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

    void load_config()
    {
        try
        {
            Genode::Attached_rom_dataspace config = {_env, "config"};
            char buffer[16] = {0};
            Genode::Xml_node xml = config.xml().sub_node("conf");
            xml.attribute("ip").value(buffer,16);
            _ip = Ipv4_packet::ip_from_string(buffer);
            Genode::log("using static IP: ", (const char*)buffer);
        }
        catch(...)
        {
            
            _ip = Net::Ipv4_packet::ip_from_string("10.0.0.20");
            _ip.addr[3] = random(253) + 1;
            Genode::log("using random IP: ", _ip);
        }
        
    }

  public:

    Main(Genode::Env &env) : _env(env) 
    {
        load_config();
        _reporter.enabled(true);
        _reporter.clear();
        _reporter.report(&_ip,4);
    }
};

void Component::construct(Genode::Env &env)
{
	static ip_provider::Main main(env);
}
