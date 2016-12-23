/*
 *
 * \description (from https://wiki.analog.com/resources/fpga/xilinx/kc705/adv7511) 
 *
 *  The video part consists of a Xilinx VDMA interface and the ADV7511 video interface. The ADV7511 interface consists
 *  of a 16bit YCbCr 422 with separate synchorinzation signals. The VDMA streams frame data to this core. The internal
 *  buffers of this pcore are small (1k) and do NOT buffer any frames as such. Additional resources may cause loss of
 *  synchronization due to DDR bandwidth requirements. The video core is capable of supporting any formats through a set
 *  of parameter registers (given below). The pixel clock is generated internal to the device and must be configured for
 *  the correct pixel frequency. It also allows a programmable color pattern for debug purposes. A zero to one
 *  transition on the enable bits trigger the corresponding action for HDMI enable and color pattern enable.
 *
 *  The reference design defaults to the 1080p video mode. Users may change the video settings by programming the
 *  following registers. The core requires a corresponding pixel clock to generate the video. This clock must be
 *  generated externally.
 *
 *  HSYNC count: is the total horizontal pixel clocks of the video, for 1080p this is 2200.
 *  HSYNC width: is the pulse width in pixel clocks, for 1080p this is 44.
 *  HSYNC DE Minimum: is the number of pixel clocks for the start of active video and is the sum of horizontal sync
 *  width and back porch, for 1080p this is 192 (44 + 148).
 *  HSYNC DE Maximum: is the number of pixel clocks for the end of active video and is the sum of horizontal sync width,
 *  back porch and the active video count, for 1080p this is 2112 (44 + 148 + 1920).
 *
 *  VSYNC count: is the total vertical pixel clocks of the video, for 1080p this is 1125.
 *  VSYNC width: is the pulse width in pixel clocks, for 1080p this is 5.
 *  VSYNC DE Minimum: is the number of pixel clocks for the start of active video and is the sum of vertical sync width
 *  and back porch, for 1080p this is 41 (5 + 36).
 *  VSYNC DE Maximum: is the number of pixel clocks for the end of active video and is the sum of vertical sync width,
 *  back porch and the active video count, for 1080p this is 1121 (5 + 36 + 1080).
 *
 *  Note that the pixel frequency for 1080p is 148.5MHz.
 *
 *  The reference design reads 24bits of RGB data from DDR and performs color space conversion (RGB to YCbCr) and down
 *  sampling (444 to 422). If bypassed, the lower 16bits of DDR data is passed to the HDMI interface as it is.
 *
 *  A color pattern register provides a quick check of any RGB values on the monitor. If enabled, the register data is
 *  used as the pixel data for the entire frame.
 *
 *  The audio part consists of a Xilinx DMA interface and the ADV7511 spdif audio interface. The audio clock is derived
 *  from the bus clock. A programmable register (see below) controls the division factor. The audio data is read from
 *  the DDR as two 16bit words for the left and right channels. It is then transmitted on the SPDIF frame. The sample
 *  frequency and format may be controlled using the registers below. The reference design defaults to 48KHz. 
 */
#ifndef AXI_HDMI_H
#define AXI_HDMI_H

#include <drivers/board_base.h>
#include <os/attached_io_mem_dataspace.h>
#include <util/mmio.h>


using namespace Genode;

struct Axi_hdmi : Attached_io_mem_dataspace, Mmio
{
    Axi_hdmi(Genode::addr_t const mmio_base, Genode::size_t const mmio_size) :
      Genode::Attached_io_mem_dataspace(mmio_base, mmio_size),
      Genode::Mmio((Genode::addr_t)local_addr<void>())
    { 
        // default values for 1080p 16:9 60Hz
        // copied from: https://github.com/analogdevicesinc/no-OS/blob/master/adv7511/zc706/cf_hdmi.c
        horizontal_active_time      = 1920;
        horizontal_blanking_time    = 280;
        horizontal_sync_offset      = 44;
        horizontal_sync_pulse_width = 88;
        horizontal_enable_min       = 192;
        // from no-os driver: min = sync_width + backporch = sync_width + blanking - sync_offset - sync_width
        horizontal_enable_min       = horizontal_blanking_time - horizontal_sync_offset;
        horizontal_enable_max       = horizontal_enable_min + horizontal_active_time;
        horizontal_count            = horizontal_active_time + horizontal_blanking_time;


        vertical_active_time      = 1080;
        vertical_blanking_time    = 45;
        vertical_sync_offset      = 4;
        vertical_sync_pulse_width = 5;
        // from no-os driver: min = sync_width + backporch = sync_width + blanking - sync_offset - sync_width
        vertical_enable_min       = vertical_blanking_time - vertical_sync_offset;
        vertical_enable_max       = vertical_enable_min + vertical_active_time;
        vertical_count            = vertical_active_time + vertical_blanking_time;
    }

    void start(bool color_pattern=false) {
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

		  if (color_pattern) {
			  write<AXI_COLORPATTERN>(0xffff0000);
			  write<AXI_CONTROL_SOURCE>(AXI_CONTROL_SOURCE::FULL_RANGE::bits(AXI_CONTROL_SOURCE::FULL_RANGE::COLOR_PATTERN));
		  } else {
			  write<AXI_CONTROL_SOURCE>(AXI_CONTROL_SOURCE::FULL_RANGE::bits(0x1));
		  }
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
        struct FULL_RANGE : Bitfield<0, 2> {
           enum {
              DATA = 0x1,
              TEST_PATTERN = 0x2,
              COLOR_PATTERN = 0x3,
           };
        };
    };
    struct AXI_COLORPATTERN : Register<0x04c, 32>
    {
       struct RAW : Bitfield<0, 32> {};
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
    uint16_t horizontal_blanking_time = 0;
    uint16_t horizontal_count = 0;
    uint16_t horizontal_sync_offset = 0;
    uint16_t horizontal_sync_pulse_width = 0;
    uint16_t horizontal_enable_max = 0;
    uint16_t horizontal_enable_min = 0;


    uint16_t vertical_active_time = 0;
    uint16_t vertical_blanking_time = 0;
    uint16_t vertical_count = 0;
    uint16_t vertical_sync_offset = 0;
    uint16_t vertical_sync_pulse_width = 0;
    uint16_t vertical_enable_max = 0;
    uint16_t vertical_enable_min = 0;

};




#endif
