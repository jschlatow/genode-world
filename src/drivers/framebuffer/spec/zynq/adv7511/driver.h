/*
 * \brief  Frame-buffer driver for the adv7511
 * \author ida
 * \date   2016
 */

/*
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <drivers/board_base.h>
#include <os/attached_io_mem_dataspace.h>
#include <util/mmio.h>
#include <timer_session/connection.h>
#include <util/register.h>
#include <vector>

#include <base/log.h>
#include <i2c_session/connection.h>
#include <vdma_session/connection.h>

#include "infoframe.h"
#include "axi-hdmi.h"
#include "axi-clkgen.h"
using namespace Vdma;


// aus dem linux kernel
#define BIT(x)	(1UL << (x))
#define CLR_BIT(p,n) ((p) &= ~((1) << (n)))
#define SET_BIT(p,n) ((p) |= (1 << (n)))

namespace Framebuffer {
    using namespace Genode;
    class Driver;
}

class Framebuffer::Driver
{
    public:

        enum Format { FORMAT_RGB565 };
        enum Output { OUTPUT_LCD, OUTPUT_HDMI };
        //TODO: remove unused register
        enum Register_names : uint8_t {
                REG_CHIP_REVISION            = 0x00,
                REG_N0                       = 0x01,
                REG_N1                       = 0x02,
                REG_N2                       = 0x03,
                REG_SPDIF_FREQ               = 0x04,
                REG_CTS_AUTOMATIC1           = 0x05,
                REG_CTS_AUTOMATIC2           = 0x06,
                REG_CTS_MANUAL0              = 0x07,
                REG_CTS_MANUAL1              = 0x08,
                REG_CTS_MANUAL2              = 0x09,
                REG_AUDIO_SOURCE             = 0x0a,
                REG_AUDIO_CONFIG             = 0x0b,
                REG_I2S_CONFIG               = 0x0c,
                REG_I2S_WIDTH                = 0x0d,
                REG_AUDIO_SUB_SRC0           = 0x0e,
                REG_AUDIO_SUB_SRC1           = 0x0f,
                REG_AUDIO_SUB_SRC2           = 0x10,
                REG_AUDIO_SUB_SRC3           = 0x11,
                REG_AUDIO_CFG1               = 0x12,
                REG_AUDIO_CFG2               = 0x13,
                REG_AUDIO_CFG3               = 0x14,
                REG_I2C_FREQ_ID_CFG          = 0x15,
                REG_VIDEO_INPUT_CFG1         = 0x16,
                REG_PIXEL_REPETITION         = 0x3b,
                REG_VIC_MANUAL               = 0x3c,
                REG_VIC_SEND                 = 0x3d,
                REG_VIC_DETECTED             = 0x3e,
                REG_AUX_VIC_DETECTED         = 0x3f,
                REG_PACKET_ENABLE0           = 0x40,
                REG_POWER                    = 0x41,
                REG_STATUS                   = 0x42,
                REG_EDID_I2C_ADDR            = 0x43,
                REG_PACKET_ENABLE1           = 0x44,
                REG_PACKET_I2C_ADDR          = 0x45,
                REG_DSD_ENABLE               = 0x46,
                REG_VIDEO_INPUT_CFG2         = 0x48,
                REG_INFOFRAME_UPDATE         = 0x4a,
                REG_AVI_INFOFRAME_VERSION    = 0x52,
                REG_AVI_INFOFRAME_LENGTH     = 0x53,
                REG_AVI_INFOFRAME_CHECKSUM   = 0x54,
                REG_AVI_INFOFRAME            = 0x55,
                REG_AUDIO_INFOFRAME_VERSION  = 0x70,
                REG_AUDIO_INFOFRAME_LENGTH   = 0x71,
                REG_AUDIO_INFOFRAME_CHECKSUM = 0x72,
                REG_INT_ENABLE0              = 0x92,
                REG_INT_ENABLE1              = 0x94,
                REG_INT_ENABLE2              = 0x95,
                REG_INTERRUPT                = 0x96,
                REG_INTERRUPT2               = 0x97,
                REG_INPUT_CLK_DIV            = 0x9d,
                REG_PLL_STATUS               = 0x9e,
                REG_HDMI_POWER               = 0xa1,
                REG_HDCP_HDMI_CFG            = 0xaf,
                REG_HDCP_STATUS              = 0xb8,
                REG_BCAPS                    = 0xbe,
                REG_EDID_SEGMENT             = 0xc4,
                REG_DDC_STATUS               = 0xc8,
                REG_EDID_READ_CTRL           = 0xc9,
                REG_TIMING_GEN_SEQ           = 0xd0,
                REG_POWER2                   = 0xd6,
                REG_HSYNC_PLACEMENT_MSB      = 0xfa,

                REG_TMDS_CLOCK_INV           = 0xde,
                REG_ARC_CTRL                 = 0xdf,
                REG_CEC_I2C_ADDR             = 0xe1,
                REG_CEC_CTRL                 = 0xe2,
                REG_CHIP_ID_HIGH             = 0xf5,
                REG_CHIP_ID_LOW              = 0xf6,

                HDMI_CFG_MODE_MASK           = 0x2,
                HDMI_CFG_MODE_DVI            = 0x0,
                HDMI_CFG_MODE_HDMI           = 0x2,
        };

    private:
        Genode::size_t _fb_width;
        Genode::size_t _fb_height;

        Format _fb_format;
        const uint8_t hdmi_slave_address = 0x39;

        struct hdmi_avi_infoframe avi_infoframe;

        struct register_seq {
            uint8_t reg; 
            uint8_t value;
        };
        // stolen from the linux kernel
        struct register_seq fixed_register[8] = {
            { 0x98, 0x03 },
            { 0x9a, 0xe0 },
            { 0x9c, 0x30 },
            { 0x9d, 0x01 },
            { 0xa2, 0xa4 },
            { 0xa3, 0xa4 },
            { 0xe0, 0xd0},
            { 0xf9, 0x00 }
        };

        I2C::Connection i2c;
        Vdma::Connection vdma;
        Axi_hdmi axi_hdmi;
        Axi_clkgen axi_clkgen;

        bool i2c_read_byte(const uint8_t reg, uint8_t *data) {
            return i2c.read_byte_8bit_reg(hdmi_slave_address, reg, data);
        };
        bool i2c_write_byte(const uint8_t reg, uint8_t data) {
            return i2c.write_byte_8bit_reg(hdmi_slave_address, reg, data);
        };
        bool i2c_update_bits(const uint8_t reg, uint8_t mask, uint8_t val) {
            uint8_t tmp, org;
            i2c_read_byte(reg, &org);
            tmp = org & ~mask;
            tmp |= val & mask;
            i2c_write_byte(reg, val);

            return true;
        };

        void packet_enable(uint16_t packet) {
            if (packet & 0xff)
                i2c_update_bits(REG_PACKET_ENABLE0, packet, 0xff);

            if (packet & 0xff00) {
                packet >>= 8;
                i2c_update_bits(REG_PACKET_ENABLE1, packet, 0xff);
            }
        };
        void packet_disable(uint16_t packet) {
            if (packet & 0xff)
                i2c_update_bits(REG_PACKET_ENABLE0, packet, 0x00);

            if (packet & 0xff00) {
                packet >>= 8;
                i2c_update_bits(REG_PACKET_ENABLE1, packet, 0x00);
            }
        };

    public:

        Driver();

        static Genode::size_t bytes_per_pixel(Format format)
        {
            switch (format) {
                case FORMAT_RGB565: return 24;
            }
            return 0;
        }

        Genode::size_t buffer_size(Genode::size_t width, Genode::size_t height, Format format) {
            return bytes_per_pixel(format)*width*height;
        }

        bool init(Genode::size_t width, Genode::size_t height, Format format,
                Output output, addr_t phys_base);

        void set_fixed_register() {
            struct register_seq *r = nullptr;
            for(Genode::size_t i = 0; i < 8; i++) {
                r = &(fixed_register[i]);
                i2c.write_byte_8bit_reg(hdmi_slave_address, r->reg, r->value);
            }
        }

        void set_infoframe() {
            uint8_t infoframe[17];
            hdmi_avi_infoframe config;
            hdmi_avi_infoframe_init(&config);

            config.scan_mode  = HDMI_SCAN_MODE_UNDERSCAN;
            config.colorspace = HDMI_COLORSPACE_YUV422;
            hdmi_avi_infoframe_pack(&config, infoframe, sizeof(infoframe));

            for (int i = 1; i < sizeof(infoframe); i++) {
                i2c.write_byte_8bit_reg(hdmi_slave_address, 0x52+(i*8), infoframe[i]);
            }
        }
        void dump();
};


Framebuffer::Driver::Driver()
    :
        _fb_width(0),
        _fb_height(0),
        _fb_format(FORMAT_RGB565),
        i2c(0),
        vdma(0),
        axi_hdmi(0x6c000000, 0x1000),    /* FIXME: add to board_base.h */
        axi_clkgen(0x66000000, 0x1000) /* FIXME: add to board_base.h */
{ 

}
// axi clock 66000000
// linux axi clock value = 148484847

//read and print selected registers values for debugging
void Framebuffer::Driver::dump()
{
    uint8_t value;
    Genode::log("\n===== Hot Plug and Monitor Sense =====");

    i2c_read_byte(REG_STATUS, &value);
    Genode::log("HPD state:           ", Genode::Hex(value>>6));
    Genode::log("Monitor sense state: ", Genode::Hex(value>>5));

    i2c_read_byte(REG_POWER2, &value);
    Genode::log("HPD Control:         ", Genode::Hex(value>>6));

    Genode::log("\n===== System Monitoring =====");

    i2c_read_byte(REG_INT_ENABLE2, &value);
    Genode::log("DDC Contr. Error Interrupt Enable: ", Genode::Hex(value>>7));

    i2c_read_byte(REG_INTERRUPT2, &value);
    Genode::log("DDC Contr. Error Interrupt Status: ", Genode::Hex(value>>7));

    i2c_read_byte(REG_PLL_STATUS, &value);
    Genode::log("PLL Lock Status:                   ", Genode::Hex(value>>4));
    
    i2c_read_byte(REG_DDC_STATUS, &value);
    Genode::log("DDC Status:                        ", Genode::Hex(value));

    Genode::log("\n===== Video Mode Detection =====");

    i2c_read_byte(REG_VIDEO_INPUT_CFG2, &value);
    Genode::log("Aspect Ratio: ", Genode::Hex(value));

    i2c_read_byte(REG_VIC_DETECTED, &value);
    Genode::log("VIC detected: ", Genode::Hex(value>>2));

    i2c_read_byte(REG_VIC_SEND, &value);
    Genode::log("VIC to Rx:    ", Genode::Hex(value));

    i2c_read_byte(REG_VIC_MANUAL, &value);
    Genode::log("VIC manual:   ", Genode::Hex(value));

    Genode::log("\n===== Input Formatting =====");

    i2c_read_byte(REG_I2C_FREQ_ID_CFG, &value);
    Genode::log("Input ID:      ", Genode::Hex(value));

    i2c_read_byte(REG_VIDEO_INPUT_CFG1, &value);
    Genode::log("Output Format: ", Genode::Hex(value));
    Genode::log("Color Depth:   ", Genode::Hex(value));
    Genode::log("Input Style:   ", Genode::Hex(value));

}

bool Framebuffer::Driver::init(Genode::size_t width, Genode::size_t height,
        Framebuffer::Driver::Format format,
        Output output,
        Framebuffer::addr_t phys_base)
{
    _fb_width = width;
    _fb_height = height;
    _fb_format = format;

    hdmi_avi_infoframe_init(&avi_infoframe);

    /* set pixel clock */
    axi_clkgen.set_rate(148500000, 200000000);
    Genode::log("nach axi_clkgen.start()");

    axi_hdmi.start(true);
    Genode::log("nach axi_hdmi.start()");

    uint32_t addr = (uint32_t) &phys_base;
    vdma.setAddr(addr, true);

    vdma.setStride((height * 3), true);
    vdma.setWidth((height * 3), true );
    vdma.setHeight((width), true);
    
    uint32_t vdma_config = 0x00000003;
    vdma.setConfig(vdma_config, true);
    //end vdma init

    uint8_t hpd = 0;
    i2c_read_byte(REG_STATUS, &hpd);
    Genode::log("HPD ", hpd & BIT(6));

    i2c_write_byte(0x96, 0xff); // clear all interrupts
    i2c_write_byte(REG_POWER, 0x10);
    i2c_read_byte(0x96, &hpd);

    i2c_write_byte(0xd6, (1 << 7) | (1 << 6)); // force the transmitter to turn on

    while(1) {
        uint8_t status = 0;
        i2c_read_byte(0x41, &status);

        if ((status & 0x40) == 0) {
            //Genode::log("still sleeping");
        }
        else {
            Genode::log("I AM AWAKE");
            break;
        }
    }
    set_fixed_register();

    i2c_write_byte(0xa1, 0x0);

    i2c_write_byte(REG_AUDIO_SOURCE, 0x11);
    i2c_write_byte(REG_I2C_FREQ_ID_CFG, 0x1); // no audio and 24 bit color

    i2c_update_bits(REG_POWER2, 0xc0, 0xc0);

    i2c_write_byte(REG_PACKET_ENABLE1, 0x10);  // enable avi infoframes
    i2c_write_byte(REG_INT_ENABLE1, BIT(2));
    i2c_write_byte(REG_INT_ENABLE2, BIT(7));


    //i2c_write_byte(REG_INTERRUPT, 0x60); 
    i2c_write_byte(REG_INPUT_CLK_DIV, 0x61); 

    //i2c_write_byte(REG_TIMING_GEN_SEQ, 0x0c); 
    i2c_write_byte(REG_CEC_CTRL, 0x00); 
    i2c_write_byte(0xfb, 0x02); // sync delay stuff

    // linux kernel copy
    i2c_write_byte(REG_INPUT_CLK_DIV, 0x61); 

    i2c_update_bits(REG_I2C_FREQ_ID_CFG,0xf, 0x1);

    i2c_update_bits(REG_VIDEO_INPUT_CFG1, 0x7e, (0x0 << 4) | (0x2 << 2));

    i2c_write_byte(REG_VIDEO_INPUT_CFG2, (0x0 << 6));
    i2c_write_byte(REG_TIMING_GEN_SEQ, (0x3 << 2) | (0x0 << 1));

    i2c_update_bits(0xfb, 0x6, (0 << 1)); // no low refresh rate

    i2c_write_byte(0xba, (0x3 << 5));// no clock delay
    i2c_update_bits(REG_TMDS_CLOCK_INV, 0x08, (0x0 << 3)); 
    drop_interrupts();

    i2c_write_byte(0x3c, 0x10); // VIC manual 1080p 60hz 16:9
    i2c_write_byte(0xd5, 1);


    // ###################### AVI INFOFRAME ##############################
    // Packet enable Infoframe
    i2c_write_byte(0xd5, 1); /* double refresh rate for VIC detection */
    // Packet enable Infoframe
    uint8_t avi_ctrl = 0;
    i2c_read_byte(0x4a, &avi_ctrl);
    SET_BIT(avi_ctrl, 7);
    i2c_write_byte(0x4a, avi_ctrl); //set autogenerated checksum

    uint8_t avi_infoframe_status = 0;
    i2c_read_byte(REG_INFOFRAME_UPDATE, &avi_infoframe_status);
    SET_BIT(avi_infoframe_status, 6);
    i2c_write_byte(REG_INFOFRAME_UPDATE, avi_infoframe_status);


    i2c_write_byte(0x54, 0x4d); // avi infoframe length

    i2c_write_byte(REG_AVI_INFOFRAME, 0x22); // set output format, should be written after 0x16 Register

    i2c_write_byte(REG_HDCP_HDMI_CFG, 0x16); 
    i2c_write_byte(0x54, 0x4d); // avi infoframe length

    //i2c_write_byte(REG_VIDEO_INPUT_CFG1, 0x89); // colorspace ycbcr
    i2c_write_byte(REG_AVI_INFOFRAME, 0x22); // set output format, should be written after 0x16 Register

    i2c_write_byte(REG_HDCP_HDMI_CFG, 0x16); /* HDMI Mode, HDCP Disabled, Current Frame HDCP Encrypted */

    // Packet disable Infoframe
    avi_infoframe_status = 0;
    i2c_read_byte(REG_INFOFRAME_UPDATE, &avi_infoframe_status);
    CLR_BIT(avi_infoframe_status, 6);
    i2c_write_byte(REG_INFOFRAME_UPDATE, avi_infoframe_status);

    // ###################### END OF INFOFRAME #############################

    dump();
    
    /*
     *for (int i = 0; i <= 0xff; i++) {
     *    uint8_t value = 0;
     *    i2c.read_byte_8bit_reg(hdmi_slave_address, i, &value);
     *    Genode::log("", Genode::Hex(i), " ", Genode::Hex(value));
     *}
     */

    return true;

}
