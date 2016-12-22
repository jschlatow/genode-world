/*
 * \brief  Driver for the adv7511
 * \author Johannes Schlatow
 * \date   2016-12-12
 *
 * TODO implementation is not complete yet
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <drivers/i2c.h>
#include <util/i2c.h>

namespace Genode {
    class Adv7511;
}

class Genode::Adv7511 : Genode::I2c<Genode::I2c_driver, 1>
{
	public:
		Adv7511(Genode::addr_t const slave_addr, I2c_driver &driver)
			: I2c(slave_addr, driver)
		{ }

		struct Dump : Register_array<0x0, 8, 0xFF, 8>
		{
			struct Value : Bitfield<0,8> {};
		};

		void dump_registers() {
			Genode::log("\n===== I2C register dump =====");
			for (int i=0; i < 0xFF; i++) {
				Genode::uint8_t value = read<Dump>(i);
				Genode::log("Register", Genode::Hex(i), " has value ", Genode::Hex(value));
			}
		}
};
