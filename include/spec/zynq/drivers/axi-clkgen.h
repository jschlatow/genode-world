/*
 * \author Johannes Schlatow
 * \date 2016-12-23
 */
#ifndef AXI_CLKGEN_H
#define AXI_CLKGEN_H

#include <drivers/board_base.h>
#include <base/attached_io_mem_dataspace.h>
#include <util/mmio.h>

using namespace Genode;

namespace Genode {
	template<typename DRIVER> class Mmcm_plain_access;
	struct Mmcm;
	class Axi_clkgen;
}

template<typename DRIVER>
class Genode::Mmcm_plain_access
{
	friend Register_set_plain_access;

	private:
		DRIVER &_driver;

		/**
		 * Write 16bit MMCM register 'reg'
		 */
		template<typename ACCESS_T>
		inline void _write(off_t const &reg, ACCESS_T const &value) const
		{
			if (!_driver.mmcm_write(reg, value))
				Genode::error("mmcm_write() timed out");
		}

		/**
		 * Read 16bit MMCM register 'reg'
		 */
		template<typename ACCESS_T>
		inline ACCESS_T _read(off_t const &reg) const
		{
			ACCESS_T value;
			/* FIXME deal with errors */
			if (!_driver.mmcm_read(reg, &value))
				Genode::error("mmcm_read() timed out");
			return value;
		}

	public:
		Mmcm_plain_access(DRIVER &driver) : _driver(driver)
		{ }

};

/* Encapsulate Mmcm registers, uses Axi_clkgen as driver */
struct Genode::Mmcm : Mmcm_plain_access<Axi_clkgen>, Register_set<Mmcm_plain_access<Axi_clkgen>> {
	public: 
		Mmcm(Axi_clkgen &driver)
			: Mmcm_plain_access<Axi_clkgen>(driver), 
			  Register_set(*static_cast<Mmcm_plain_access<Axi_clkgen>*>(this))
		{ }

		struct Out1 : Register<0x08, 16>
		{
			struct Mask1 : Bitfield<0, 12> { };
			struct Mask2 : Bitfield<13, 3> { };
		};
		struct Out1_bitset : Bitset_2<Out1::Mask1, Out1::Mask2> { };

		struct Out2 : Register<0x09, 16>
		{
			struct Mask : Bitfield<0, 10> { };
		};

		struct Fb1 : Register<0x14, 16>
		{
			struct Mask1 : Bitfield<0, 12> { };
			struct Mask2 : Bitfield<13, 3> { };
		};
		struct Fb1_bitset : Bitset_2<Fb1::Mask1, Fb1::Mask2> { };

		struct Fb2 : Register<0x15, 16>
		{
			struct Mask : Bitfield<0, 10> { };
		};

		struct Div : Register<0x16, 16>
		{
			struct Mask : Bitfield<0, 14> { };
		};

		struct Lock1 : Register<0x18, 16>
		{
			struct Mask : Bitfield<0, 10> { };
		};

		struct Lock2 : Register<0x19, 16>
		{
			struct Mask : Bitfield<0, 15> { };
		};
		
		struct Lock3 : Register<0x1a, 16>
		{
			struct Mask : Bitfield<0, 15> { };
		};

		struct Filter1 : Register<0x4e, 16>
		{
			struct Mask1 : Bitfield<8, 1>  { };
			struct Mask2 : Bitfield<11, 2> { };
			struct Mask3 : Bitfield<15, 1> { };
		};
		struct Filter1_bitset : Bitset_3<Filter1::Mask1, Filter1::Mask2, Filter1::Mask3> { };

		struct Filter2 : Register<0x4f, 16>
		{
			struct Mask1 : Bitfield<8, 1>  { };
			struct Mask2 : Bitfield<11, 2> { };
			struct Mask3 : Bitfield<15, 1> { };
		};
		struct Filter2_bitset : Bitset_3<Filter2::Mask1, Filter2::Mask2, Filter2::Mask3> { };
};

class Genode::Axi_clkgen : Attached_io_mem_dataspace, Mmio
{
	private:
		Mmcm _mmcm;

		/* Register interface */
		struct Reset : Register<0x40, 32>
		{
			struct Enable      : Bitfield<0, 1> { };
			struct Mmcm_enable : Bitfield<1, 1> { };
		};

		struct Drp_control : Register<0x70, 32>
		{
			struct Value  : Bitfield<0, 16> { };
			struct Register : Bitfield<16, 8> { };
			struct Read     : Bitfield<28, 1> { };
			struct Select   : Bitfield<29, 1> { };
		};

		struct Drp_status : Register<0x74, 32>
		{
			struct Value  : Bitfield<0, 16> { };
			struct Busy   : Bitfield<16, 1> { };
		};

		/* lookup tables */
		const unsigned long _filter_table[47] =
		{
			0x01001990, 0x01001190, 0x01009890, 0x01001890,
			0x01008890, 0x01009090, 0x01009090, 0x01009090,
			0x01009090, 0x01000890, 0x01000890, 0x01000890,
			0x08009090, 0x01001090, 0x01001090, 0x01001090,
			0x01001090, 0x01001090, 0x01001090, 0x01001090,
			0x01001090, 0x01001090, 0x01001090, 0x01008090,
			0x01008090, 0x01008090, 0x01008090, 0x01008090,
			0x01008090, 0x01008090, 0x01008090, 0x01008090,
			0x01008090, 0x01008090, 0x01008090, 0x01008090,
			0x01008090, 0x08001090, 0x08001090, 0x08001090,
			0x08001090, 0x08001090, 0x08001090, 0x08001090,
			0x08001090, 0x08001090, 0x08001090
		};

		const unsigned long _lock_table[36] =
		{
			0x060603e8, 0x060603e8, 0x080803e8, 0x0b0b03e8,
			0x0e0e03e8, 0x111103e8, 0x131303e8, 0x161603e8,
			0x191903e8, 0x1c1c03e8, 0x1f1f0384, 0x1f1f0339,
			0x1f1f02ee, 0x1f1f02bc, 0x1f1f028a, 0x1f1f0271,
			0x1f1f023f, 0x1f1f0226, 0x1f1f020d, 0x1f1f01f4,
			0x1f1f01db, 0x1f1f01c2, 0x1f1f01a9, 0x1f1f0190,
			0x1f1f0190, 0x1f1f0177, 0x1f1f015e, 0x1f1f015e,
			0x1f1f0145, 0x1f1f0145, 0x1f1f012c, 0x1f1f012c,
			0x1f1f012c, 0x1f1f0113, 0x1f1f0113, 0x1f1f0113
		};

		/* helper function used by mmcm_read() and mmcm_write() */
		inline bool _mmcm_wait(uint16_t *value);

		/* helper functions for parameter calculation */
		inline unsigned long _div_round_up(unsigned long x, unsigned long y) {
			return (x + y - 1) / y;
		}

		inline unsigned long _div_round_closest(unsigned long x, unsigned long y) {
			return (unsigned long)(((double)x / y) + 0.5);
		}

		inline void _calc_params(unsigned long fin,
										 unsigned long fout,
										 unsigned long *best_d,
										 unsigned long *best_m,
										 unsigned long *best_dout)
		{
			unsigned long fpfd_min       = 10000;
			const unsigned long fpfd_max = 300000;
			const unsigned long fvco_min = 600000;
			const unsigned long fvco_max = 1200000;
			unsigned long d              = 0;
			unsigned long d_min          = 0;
			unsigned long d_max          = 0;
			unsigned long _d_min         = 0;
			unsigned long _d_max         = 0;
			unsigned long m              = 0;
			unsigned long m_min          = 0;
			unsigned long m_max          = 0;
			unsigned long dout           = 0;
			unsigned long fvco           = 0;
			long          f              = 0;
			long          best_f         = 0;

			fin /= 1000;
			fout /= 1000;

			best_f = 0x7fffffff;
			*best_d = 0;
			*best_m = 0;
			*best_dout = 0;

			d_min = Genode::max(_div_round_up(fin, fpfd_max), 1);
			d_max = Genode::min(fin / fpfd_min, 80);

			m_min = Genode::max(_div_round_up(fvco_min, fin) * d_min, 1);
			m_max = Genode::min(fvco_max * d_max / fin, 64);

			for(m = m_min; m <= m_max; m++)
			{
				_d_min = Genode::max(d_min, _div_round_up(fin * m, fvco_max));
				_d_max = Genode::min(d_max, fin * m / fvco_min);

				for (d = _d_min; d <= _d_max; d++)
				{
					fvco = fin * m / d;

					dout = _div_round_closest(fvco, fout);
					dout = (dout < 1 ? 1 : (dout > 128 ? 128 : dout));
					f = fvco / dout;
					if (Genode::abs(f - fout) < Genode::abs(best_f - fout))
					{
						best_f = f;
						*best_d = d;
						*best_m = m;
						*best_dout = dout;
						if ((unsigned long)best_f == fout)
						{
							return;
						}
					}
				}
			}
		}

		inline void _calc_clock_params(unsigned long divider,
										  unsigned long *low,
										  unsigned long *high,
										  unsigned long *edge,
										  unsigned long *nocount)
		{
			if (divider == 1)
				*nocount = 1;
			else
				*nocount = 0;

			*high = divider / 2;
			*edge = divider % 2;
			*low  = divider - *high;
		}

		/* access lookup tables with default values */
		inline unsigned long _filter(unsigned m)
		{
			if (m < 47) {
				return _filter_table[m] & 0x99009900;
			}
			return 0x08008000;
		}

		inline unsigned long _lock(unsigned m)
		{
			if (m < 36) {
				return _lock_table[m];
			}
			return 0x1f1f00fa;
		}

	public:

		Axi_clkgen(Genode::Env &env, Genode::addr_t const mmio_base, Genode::size_t const mmio_size)
			: Genode::Attached_io_mem_dataspace(env, mmio_base, mmio_size),
		     Genode::Mmio((Genode::addr_t)local_addr<void>()),
		     _mmcm(*this)
		{ }

		/* provide access to the MMCM registers (driver interface for Mmcm_base)  */
		bool mmcm_read(unsigned int reg, uint16_t *value);
		bool mmcm_write(unsigned int reg, uint16_t const &value);

		/* set rate */
		void set_rate(unsigned long rate, unsigned long parent_rate);

		void dump();
};

bool Genode::Axi_clkgen::_mmcm_wait(uint16_t *value)
{
	unsigned timeout = 10000;
	uint32_t raw;

	do {
		raw = read<Drp_status>();
	} while ((raw & Drp_status::Busy::bits(0x1)) && --timeout);

	*value = Drp_status::Value::masked(raw);

	return !(raw & Drp_status::Busy::bits(0x1));
}

bool Genode::Axi_clkgen::mmcm_read(unsigned int reg, uint16_t *value)
{
	/* wait for mmcm */
	bool busy = !_mmcm_wait(value);

	if (busy)
		return false;

	/* no-os driver first writes 0x00 */
//	write<Drp_control>(0);
	write<Drp_control>(Drp_control::Read::bits(1) | Drp_control::Select::bits(1) | Drp_control::Register::bits(reg));

	busy = !_mmcm_wait(value);

	return !busy;
}

bool Genode::Axi_clkgen::mmcm_write(unsigned int reg, uint16_t const &value)
{
	uint16_t tmp;
	/* wait for mmcm */
	bool busy = !_mmcm_wait(&tmp);

	if (busy)
		return false;

	/* no-os driver first writes 0x00 */
//	write<Drp_control>(0);
	write<Drp_control>(Drp_control::Select::bits(1) | Drp_control::Register::bits(reg) | Drp_control::Value::bits(value));

	return true;
}

void Genode::Axi_clkgen::set_rate(unsigned long rate, unsigned long parent_rate)
{
	if (!rate || !parent_rate)
		return;

	unsigned long d = 0;
	unsigned long m = 0;
	unsigned long dout = 0;

	/* calculate paarameters */
	_calc_params(parent_rate, rate, &d, &m, &dout);
	if (!d || !m || !dout)
		return;

	unsigned long filter = _filter(m-1);
	unsigned long lock = _lock(m-1);

	/* disable MMCM */
	write<Reset::Mmcm_enable>(0);

	unsigned long low = 0;
	unsigned long high = 0;
	unsigned long edge = 0;
	unsigned long nocount = 0;

	_calc_clock_params(dout, &low, &high, &edge, &nocount);
	_mmcm.write<Mmcm::Out1_bitset>((high << 6) | low);
	_mmcm.write<Mmcm::Out2::Mask>((edge << 7) | (nocount << 6));

	_calc_clock_params(m, &low, &high, &edge, &nocount);
	_mmcm.write<Mmcm::Div::Mask>((edge << 13) | (nocount << 12) | (high << 6) | low);

	_calc_clock_params(m, &low, &high, &edge, &nocount);
	_mmcm.write<Mmcm::Fb1_bitset>((high << 6) | low);
	_mmcm.write<Mmcm::Fb2::Mask>((edge << 7) | (nocount << 6));

	/* set lock and filter */
	_mmcm.write<Mmcm::Lock1::Mask>(lock);
	_mmcm.write<Mmcm::Lock2::Mask>((((lock >> 16) & 0x1f) << 10) | 0x1);
	_mmcm.write<Mmcm::Lock3::Mask>((((lock >> 24) & 0x1f) << 10) | 0x3e9);
	_mmcm.write<Mmcm::Filter1_bitset>(filter >> 16);
	_mmcm.write<Mmcm::Filter2_bitset>(filter);

	/* enable MMCM */
	write<Reset::Mmcm_enable>(1);
}

void Genode::Axi_clkgen::dump()
{
	Genode::log("Drp reset status: ", Genode::Hex(read<Reset>()));

	uint16_t value = 0;
	value = _mmcm.read<Mmcm::Out1_bitset>();
	Genode::log("Mmcm::Out1_bitset: ", Genode::Hex(value));
	value = _mmcm.read<Mmcm::Out2::Mask>();
	Genode::log("Mmcm::Out1::Mask: ", Genode::Hex(value));

	uint32_t filter = 0;
	filter = _mmcm.read<Mmcm::Filter1_bitset>();
	filter = filter << 16;
	filter |= _mmcm.read<Mmcm::Filter2_bitset>();

	Genode::log("Filter: ", Genode::Hex(filter));
}

#endif /* AXI_CLKGEN_H_ */
