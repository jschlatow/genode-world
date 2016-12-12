#ifndef AXI_HDMI_H
#define AXI_HDMI_H

#include <drivers/board_base.h>
#include <os/attached_io_mem_dataspace.h>
#include <util/mmio.h>


using namespace Genode;

struct Axi_hdmi : Attached_io_mem_dataspace, Mmio {
    Axi_hdmi(Genode::addr_t const mmio_base, Genode::size_t const mmio_size) :
		Genode::Attached_io_mem_dataspace(mmio_base, mmio_size),
	  	Genode::Mmio((Genode::addr_t)local_addr<void>())
    { 
        // default values for 1080p 16:9 60Hz
        horizontal_active_time      = 1920;
        horizontal_count            = 2200;
        horizontal_sync_pulse_width = 44;
        horizontal_enable_max       = 2112;
        horizontal_enable_min       = 192;


        vertical_active_time      = 1080;
        vertical_count            = 1125;
        vertical_sync_pulse_width = 5;
        vertical_enable_max       = 1121;
        vertical_enable_min       = 41;
    }

    void start() {
        write<AXI_HSYNC_1>(AXI_HSYNC_1::H_LINE_ACTIVE::bits(horizontal_active_time) 
                | AXI_HSYNC_1::H_LINE_WIDTH::bits(horizontal_count));
        write<AXI_HSYNC_2>(AXI_HSYNC_2::H_SYNC_WIDTH::bits(horizontal_sync_pulse_width));
        write<AXI_HSYNC_3>(AXI_HSYNC_3::H_ENABLE_MAX::bits(horizontal_enable_max)
                | AXI_HSYNC_3::H_ENABLE_MIN::bits(horizontal_enable_min));

        write<AXI_VSYNC_1>(AXI_VSYNC_1::H_LINE_ACTIVE::bits(horizontal_active_time) 
                | AXI_VSYNC_1::H_LINE_WIDTH::bits(horizontal_count));
        write<AXI_VSYNC_2>(AXI_VSYNC_2::H_SYNC_WIDTH::bits(horizontal_sync_pulse_width));
        write<AXI_VSYNC_3>(AXI_VSYNC_3::H_ENABLE_MAX::bits(horizontal_enable_max)
                | AXI_VSYNC_3::H_ENABLE_MIN::bits(horizontal_enable_min));


        write<AXI_RESET>(AXI_RESET::RESET::bits(0x1));
        write<AXI_CONTROL_SOURCE>(AXI_CONTROL_SOURCE::FULL_RANGE::bits(0x0));
        write<AXI_CONTROL_SOURCE>(AXI_CONTROL_SOURCE::FULL_RANGE::bits(0x1));
    }


    private:
    struct AXI_VERSION : Register<0x00, 32>
    {
        struct VERSION : Bitfield<0,31> {};
    };
    struct AXI_RESET : Register<0x040, 32>
    {
        struct RESET : Bitfield<0,31> {};
    };
    struct AXI_CONTROL : Register<0x044, 32>
    {
        struct FULL_RANGE : Bitfield<1, 1> {};
        struct CSC_BYPASS : Bitfield<0, 1> {};
    };
    struct AXI_CONTROL_SOURCE : Register<0x048, 32>
    {
        struct FULL_RANGE : Bitfield<0, 2> {};
    };
    struct VDMA_STATUS : Register<0x0060, 32> {
        struct OVERFLOW : Bitfield<1, 1> {};
        struct UNDERFLOW : Bitfield<0, 1> {};
    };

    struct AXI_HSYNC_1 : Register<0x0400, 32> {
        struct H_LINE_ACTIVE : Bitfield<16, 16> {};
        struct H_LINE_WIDTH : Bitfield<0, 16> {};
    };
    struct AXI_HSYNC_2 : Register<0x0404, 32> {
        struct H_SYNC_WIDTH : Bitfield<0, 16> {};
    };
    struct AXI_HSYNC_3 : Register<0x0408, 32> {
        struct H_ENABLE_MAX : Bitfield<16, 16>{};
        struct H_ENABLE_MIN : Bitfield<0, 16> {};
    };

    struct AXI_VSYNC_1 : Register<0x0440, 32> {
        struct H_LINE_ACTIVE : Bitfield<16, 16> {};
        struct H_LINE_WIDTH : Bitfield<0, 16> {};
    };
    struct AXI_VSYNC_2 : Register<0x0444, 32> {
        struct H_SYNC_WIDTH : Bitfield<0, 16> {};
    };
    struct AXI_VSYNC_3 : Register<0x0448, 32> {
        struct H_ENABLE_MAX : Bitfield<16, 16>{};
        struct H_ENABLE_MIN : Bitfield<0, 16> {};
    };

    uint16_t horizontal_active_time = 0;
    uint16_t horizontal_count = 0;
    uint16_t horizontal_sync_pulse_width = 0;
    uint16_t horizontal_enable_max = 0;
    uint16_t horizontal_enable_min = 0;


    uint16_t vertical_active_time = 0;
    uint16_t vertical_count = 0;
    uint16_t vertical_sync_pulse_width = 0;
    uint16_t vertical_enable_max = 0;
    uint16_t vertical_enable_min = 0;

};




#endif
