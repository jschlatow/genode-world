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

struct Genode::Adv7511 : Genode::I2c<Genode::I2c_driver, 1>
{
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

		struct Input_cfg2 : Register<0x17, 8> {
			struct Aspect_ratio : Bitfield<1, 1> {
				enum {
					ASPECT_4_3 = 0,
					ASPECT_16_9 = 1,
				};
			};
		};

		struct Vic : Register<0x3c, 8> { };

		struct Power : Register<0x41, 8> {
			struct Down : Bitfield<6, 1> { };
		};

		struct Status : Register<0x42, 8> {
			struct Hpd   : Bitfield<6, 1> { };
			struct Sense : Bitfield<5, 1> { };
		};

		struct Video_cfg2 : Register<0x48, 8> {
			struct Bus_order : Bitfield<6, 1> {
				enum {
					NORMAL = 0,
					REVERSE = 1,
				};
			};
		};

		struct Power2 : Register<0xd6, 8> {
			struct Hdp : Bitfield<6, 2> { };
		};

		struct Input_clk_div : Register<0x9d, 8> { };

		struct Cfg1 : Register<0xba, 8> {
			struct Clock_delay : Bitfield<5, 3> {
				enum {
					NO_DELAY = 0x3,
				};
			};
		};

		struct Timing : Register<0xd0, 8> {
			struct Sync_pulse : Bitfield<2, 2> {
				enum {
					NO_SYNC = 0,
				};
			};
		};

		struct Tmds : Register<0xd3, 8> {
			struct Inversion : Bitfield<3, 1> { };
		};

		struct Timing2 : Register<0xfb, 8> {
			struct Low_refresh : Bitfield<1, 2> { };
		};

		struct Cec_ctrl : Register<0xe2, 8> { };

		void dump_registers() {
			Genode::log("\n===== I2C register dump =====");
			for (int i=0; i < 0xFF; i++) {
				Genode::uint8_t value = read<Dump>(i);
				Genode::log("Register", Genode::Hex(i), " has value ", Genode::Hex(value));
			}
		}
};
