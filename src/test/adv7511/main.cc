#include <base/log.h>

#include <drivers/board_base.h>
#include <drivers/i2c.h>
#include <drivers/adv7511.h>

#include <drivers/axi-hdmi.h>
#include <drivers/axi-clkgen.h>

using namespace Genode;

struct Config {
	enum {
		ADV7511_ADDR_1 = 0x39,
		ADV7511_ADDR_2 = 0x3d,
	};
};

void dump(Adv7511 &adv7511)
{
	uint8_t value; 

	Genode::log("\n===== Hot Plug and Monitor Sense =====");

	Genode::log("HPD state:           ", Genode::Hex(adv7511.read<Adv7511::Status::Hpd>()));
	Genode::log("Monitor sense state: ", Genode::Hex(adv7511.read<Adv7511::Status::Sense>()));

	Genode::log("HPD Control:         ", Genode::Hex(adv7511.read<Adv7511::Power2::Hdp>()));

	value = adv7511.read<Adv7511::Dump>(0xAF);
	Genode::log("HDCP Control:        ", Genode::Hex(value));

	value = adv7511.read<Adv7511::Dump>(0xB8);
	Genode::log("HDCP Status:        ", Genode::Hex(value));

	Genode::log("\n===== System Monitoring =====");

	value = adv7511.read<Adv7511::Dump>(0x95);
	Genode::log("DDC Contr. Error Interrupt Enable: ", Genode::Hex(value>>7));

	value = adv7511.read<Adv7511::Dump>(0x97);
	Genode::log("DDC Contr. Error Interrupt Status: ", Genode::Hex(value>>7));

	value = adv7511.read<Adv7511::Dump>(0x9e);
	Genode::log("PLL Lock Status:                   ", Genode::Hex(value>>4));

	Genode::log("DDC Status:                        ", Genode::Hex(adv7511.read<Adv7511::Ddc::Status>()));
	Genode::log("DDC Error:                         ", Genode::Hex(adv7511.read<Adv7511::Ddc::Error>()));

	Genode::log("\n===== Video Mode Detection =====");

	value = adv7511.read<Adv7511::Dump>(0x3e);
	Genode::log("VIC detected:            ", Genode::Hex(value>>2));
	value = adv7511.read<Adv7511::Dump>(0x3f);
	Genode::log("VIC detected (non-861D): ", Genode::Hex(value>>5));

	value = adv7511.read<Adv7511::Dump>(0x3d);
	Genode::log("VIC to Rx:               ", Genode::Hex(value));

	value = adv7511.read<Adv7511::Dump>(0x3c);
	Genode::log("VIC manual:              ", Genode::Hex(value));

	value = adv7511.read<Adv7511::Dump>(0x3b);
	Genode::log("Pixel Repetition:        ", Genode::Hex(value));

	Genode::log("\n===== Input Formatting =====");

	value = adv7511.read<Adv7511::Dump>(0x15);
	Genode::log("Input ID:      ", Genode::Hex(value));

	Genode::log("Output Format: ", Genode::Hex(adv7511.read<Adv7511::Video_cfg::Output_format>()));
	Genode::log("Color Depth:   ", Genode::Hex(adv7511.read<Adv7511::Video_cfg::Input_colordepth>()));
	Genode::log("Input Style:   ", Genode::Hex(adv7511.read<Adv7511::Video_cfg::Input_style>()));
}

int main()
{
	I2c_driver driver(Board_base::I2C0_MMIO_BASE, Board_base::I2C_MMIO_SIZE);
	Adv7511 adv7511(Config::ADV7511_ADDR_1, driver);

	Axi_hdmi   axi_hdmi(0x6c000000, 0x10000);    /* FIXME: add to board_base.h */
	Axi_clkgen axi_clkgen(0x66000000, 0x10000);  /* FIXME: add to board_base.h */

//	Genode::log("\nTesting ADV7511 interface on address ", Hex(Config::ADV7511_ADDR_1), 
//					" via I2C controller at: ", Hex(Board_base::I2C0_MMIO_BASE));
//
//	adv7511.dump_registers();
	
	/* set pixel clock */
	Genode::log("Configuring axi-clkgen");
	axi_clkgen.set_rate(148500000, 200000000);

	Genode::log("Starting axi-hdmi in COLOR_PATTERN mode");
	axi_hdmi.start(true);

	/* reset transmitter */
	adv7511.write<Adv7511::Power::Down>(1);

	dump(adv7511);

	// clear all interrupts
	adv7511.write<Adv7511::Dump>(0xff, 0x96); // FIXME
	adv7511.write<Adv7511::Power>(0x10); // set default value

	// set hot plug control (HPD pin only)
	adv7511.write<Adv7511::Power2::Hdp>(2);

	/* wait for HPD */
	Genode::log("Waiting for hot plug detection");
	while (!adv7511.read<Adv7511::Status::Hpd>()) { }

	/* reset transmitter */
	adv7511.write<Adv7511::Power::Down>(0);

	/* wait for monitor sense */
	Genode::log("Waiting for monitor sense.");
	while (!adv7511.read<Adv7511::Status::Sense>()) { }

	/* check whether all systems are powered up */
	if (adv7511.read<Adv7511::Power::Down>())
		Genode::error("ADV7511 still powered down");

	/* set fixed register values */
	adv7511.write<Adv7511::Dump>(0x03, 0x98);
	adv7511.write<Adv7511::Dump>(0xe0, 0x9a);
	adv7511.write<Adv7511::Dump>(0x30, 0x9c);
	adv7511.write<Adv7511::Dump>(0x01, 0x9d);
	adv7511.write<Adv7511::Dump>(0xa4, 0xa2);
	adv7511.write<Adv7511::Dump>(0xa4, 0xa3);
	adv7511.write<Adv7511::Dump>(0xd0, 0xe0);
	adv7511.write<Adv7511::Dump>(0x00, 0xf9);
//	adv7511.write<Adv7511::Dump>(0x02, 0x55); /* from linux driver */

	adv7511.write<Adv7511::Dump>(0x0, 0xa1);

	// no audio (invalid)
	adv7511.write<Adv7511::Dump>(0x71, 0x0a);

//	/* set HDP to always high */
//	adv7511.write<Adv7511::Power2::Hdp>(0x3);

	adv7511.write<Adv7511::Cec_ctrl>(0x00); /* disable CEC (default) */
//	adv7511.write<Adv7511::Dump>(0x02, 0xfb); // set low refresh rate

	adv7511.write<Adv7511::Input_clk_div>(0x61);
	adv7511.write<Adv7511::Input_cfg::Video_format>(0x1); /* 16,20,24bit YcbCr 4:2:2 (separate syncs) */
	adv7511.write<Adv7511::Video_cfg::Input_style>(0x2);
	adv7511.write<Adv7511::Video_cfg::Input_colordepth>(0x3); /* 8 (16) bit */
	adv7511.write<Adv7511::Video_cfg::Output_format>(Adv7511::Video_cfg::Output_format::_422);
	adv7511.write<Adv7511::Input_cfg2::Aspect_ratio>(Adv7511::Input_cfg2::Aspect_ratio::ASPECT_16_9);

	// linux kernel copy
//	adv7511.write<Adv7511::Video_cfg2>(Adv7511::Video_cfg2::Bus_order::bits(Adv7511::Video_cfg2::Bus_order::NORMAL));
//	adv7511.write<Adv7511::Timing>(Adv7511::Timing::Sync_pulse::bits(Adv7511::Timing::Sync_pulse::NO_SYNC));

	adv7511.write<Adv7511::Timing2::Low_refresh>(0); // no low refresh rate
//	adv7511.write<Adv7511::Cfg1::Clock_delay>(Adv7511::Cfg1::Clock_delay::bits(Adv7511::Cfg1::Clock_delay::NO_DELAY));
//	adv7511.write<Adv7511::Tmds::Inversion>(0); /* no clock inversion (default) */

	/* VIC manual 1080p 60hz 16:9 */
	adv7511.write<Adv7511::Vic>(0x10);

	/* disable black image */
	adv7511.write<Adv7511::Dump>(0x0, 0xd5);

	dump(adv7511);

	/* AVI infoframe */

	/* set update active */
	adv7511.write<Adv7511::Avi_packet::Update>(1);

	/* use auto-generated checksum */
	adv7511.write<Adv7511::Avi_packet::Checksum>(Adv7511::Avi_packet::Checksum::AUTO);

	/* set mandatory values */
	adv7511.write<Adv7511::Avi_infoframe1::Y1Y0>(Adv7511::Avi_infoframe1::Y1Y0::YCBCR_422);
	adv7511.write<Adv7511::Avi_infoframe2::Aspect_ratio>(Adv7511::Avi_infoframe2::Aspect_ratio::ASPECT_16_9);

	/* set update inactive */
	adv7511.write<Adv7511::Avi_packet::Update>(0);
	adv7511.write<Adv7511::Avi_status::Infoframe_enable>(1);

	dump(adv7511);

	Genode::log("Starting axi-hdmi in COLOR_PATTERN mode");
	axi_hdmi.start(true);

	axi_clkgen.dump();

	while(1) { }

	return 0;
}
