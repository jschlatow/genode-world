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
        horizontal_active_time = 1920;
        horizontal_count = 2200;
        horizontal_sync_pulse_width = 44;
        horizontal_enable_max = 2112;
        horizontal_enable_min = 192;


        vertical_active_time = 1080;
        vertical_count = 1125;
        vertical_sync_pulse_width = 5;
        vertical_enable_max = 1121;
        vertical_enable_min = 41;
    }



    private:
    enum Register : uint32_t {
        VERSION = 0x0,
        ID = 0x4,
        SCRATCH = 0x8,
        RESET = 0x040,
        CNTRL = 0x044, // controll & status
        SOURCE_SEL = 0x048,

        VDMA_STATUS = 0x60,
        TPM_STATUS = 0x68,
        HSYNC_1 = 0x4000,
        HSYNC_2 = 0x0404,
        HSYNC_3 = 0x0408,

        VSYNC_1 = 0x0440,
        VSYNC_2 = 0x0444,
        VSYNC_3 = 0x0448,

    };
    uint32_t horizontal_active_time = 0;
    uint32_t horizontal_count = 0;
    uint32_t horizontal_sync_pulse_width = 0;
    uint32_t horizontal_enable_max = 0;
    uint32_t horizontal_enable_min = 0;


    uint32_t vertical_active_time = 0;
    uint32_t vertical_count = 0;
    uint32_t vertical_sync_pulse_width = 0;
    uint32_t vertical_enable_max = 0;
    uint32_t vertical_enable_min = 0;



};




#endif
