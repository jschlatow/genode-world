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

		struct Revision : Register<0x0, 8>
		{
			struct Value : Bitfield<0, 8> {};
		};

		struct Input_cfg : Register<0x15, 8>
		{
			struct Video_format : Bitfield<0, 4> {};
			struct I2S_freq     : Bitfield<4, 4> {};
		};

		struct Video_cfg : Register<0x16, 8>
		{
			struct Input_colorspace : Bitfield<0, 1> {
				enum {
					RGB = 0x0,
					YCBCR = 0x1,
				};
			};
			struct Input_edge       : Bitfield<1, 1> {
				enum {
					FALLING = 0x0,
					RISING  = 0x1,
				};
			};
			struct Input_style      : Bitfield<2, 2> {
				enum {
					STYLE_2 = 0x1,
					STYLE_1 = 0x2,
					STYLE_3 = 0x3,
				};
			};
			struct Input_colordepth : Bitfield<4, 2> {
				enum {
					_10BIT = 0x1,
					_12BIT = 0x2,
					_8BIT  = 0x3,
				};
			};
			struct Output_format    : Bitfield<7, 1> {
				enum {
					_444 = 0,
					_422 = 1
				};
			};
		};

		void dump_registers() {
			Genode::log("\n===== I2C register dump =====");
			for (int i=0; i < 0xFF; i++) {
				Genode::uint8_t value = read<Dump>(i);
				Genode::log("Register", Genode::Hex(i), " has value ", Genode::Hex(value));
			}
		}
};
