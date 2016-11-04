/*
 * \brief  VDMA Driver for Zynq
 * \author Mark Albers
 * \date   2015-04-13
 */

/*
 * Copyright (C) 2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <io_mem_session/connection.h>
#include <timer_session/connection.h>
#include <util/mmio.h>
#include <vector>
#include <new>
#include <drivers/board_base.h>
#include "vdma.h"

namespace Vdma {
	using namespace Genode;
	class Driver;
}

class Vdma::Driver
{
    private:

        std::vector<Zynq_Vdma*> _vdma_bank;

        Driver(std::vector<Genode::addr_t> addr)
        {
            for (unsigned i = 0; i < addr.size(); i++)
            {
                _vdma_bank.push_back(new Zynq_Vdma(addr[i], Board_base::VDMA_MMIO_SIZE));
            }
        }

        ~Driver()
        {
            for (std::vector<Zynq_Vdma*>::iterator it = _vdma_bank.begin() ; it != _vdma_bank.end(); ++it)
            {
                delete (*it);
            }
            _vdma_bank.clear();
        }

	public:

        static Driver& factory(std::vector<Genode::addr_t> addr);

        bool setConfig(unsigned vdma, Genode::uint32_t data, bool isMM2S)
        {
            Genode::log("VDMA");
            Zynq_Vdma *vdma_reg = _vdma_bank[vdma];
            if (isMM2S) vdma_reg->write<Zynq_Vdma::MM2S_VDMACR>(data);
            else vdma_reg->write<Zynq_Vdma::S2MM_VDMACR>(data);

            uint32_t control = vdma_reg->read<Zynq_Vdma::MM2S_VDMACR>();
            Genode::log("Control ", Genode::Hex(control));

            uint32_t status = vdma_reg->read<Zynq_Vdma::MM2S_VDMASR>();
            Genode::log("Status ", Genode::Hex(status));

            if (isMM2S) {
                uint32_t fb = 0;
                fb = vdma_reg->read<Zynq_Vdma::Framebuffer>(0);

                Genode::log("Framebuffer 1 ", Genode::Hex(fb));
                fb = vdma_reg->read<Zynq_Vdma::Framebuffer>(1);
                Genode::log("Framebuffer 2 ", Genode::Hex(fb));
                fb = vdma_reg->read<Zynq_Vdma::Framebuffer>(2);
                Genode::log("Framebuffer 3 ", Genode::Hex(fb));
                fb = vdma_reg->read<Zynq_Vdma::Framebuffer>(3);
                Genode::log("Framebuffer 4 ", Genode::Hex(fb));
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 0);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 1);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 2);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 3);
                vdma_reg->write<Zynq_Vdma::MM2S_START_ADDRESS>(data);
            }

            return true;
        }

        bool setStride(unsigned vdma, Genode::uint16_t data, bool isMM2S)
        {
            Zynq_Vdma *vdma_reg = _vdma_bank[vdma];
            if (isMM2S) vdma_reg->write<Zynq_Vdma::MM2S_FRMDLY_STRIDE>(data);
            else vdma_reg->write<Zynq_Vdma::S2MM_FRMDLY_STRIDE>(data);
            return true;
        }

        bool setWidth(unsigned vdma, Genode::uint16_t data, bool isMM2S)
        {
            Zynq_Vdma *vdma_reg = _vdma_bank[vdma];
            if (isMM2S) vdma_reg->write<Zynq_Vdma::MM2S_HSIZE>(data);
            else vdma_reg->write<Zynq_Vdma::S2MM_HSIZE>(data);
            return true;
        }

        bool setHeight(unsigned vdma, Genode::uint16_t data, bool isMM2S)
        {
            Zynq_Vdma *vdma_reg = _vdma_bank[vdma];
            if (isMM2S) vdma_reg->write<Zynq_Vdma::MM2S_VSIZE>(data);
            else vdma_reg->write<Zynq_Vdma::S2MM_VSIZE>(data);
            return true;
        }

        bool setAddr(unsigned vdma, Genode::uint32_t data, bool isMM2S)
        {
            Zynq_Vdma *vdma_reg = _vdma_bank[vdma];
            if (isMM2S) {
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 0);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 1);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 2);
                vdma_reg->write<Zynq_Vdma::Framebuffer>(data, 3);
                //vdma_reg->write<Zynq_Vdma::MM2S_START_ADDRESS>(data);
            }
            else vdma_reg->write<Zynq_Vdma::S2MM_START_ADDRESS>(data);
            return true;
        }
};

Vdma::Driver& Vdma::Driver::factory(std::vector<Genode::addr_t> addr)
{
    static Vdma::Driver driver(addr);
    return driver;
}

#endif /* _DRIVER_H_ */
