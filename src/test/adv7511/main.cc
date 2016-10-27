#include <base/log.h>

#include <i2c_session/connection.h>

#include <drivers/board_base.h>

using namespace Genode;

struct Config {
	enum {
		ADV7511_ADDR_1 = 0x39,
		ADV7511_ADDR_2 = 0x3d,
	};
};

void dump_registers(I2C::Connection &i2c, uint8_t slave_address)
{
	for (int i=0x00; i < 0xFE; i++) {
		uint8_t data;
		i2c.read_byte_8bit_reg(slave_address, i, &data);
		Genode::log("Register ", Hex(i), " value: ", Hex(data));
	}
}

int main()
{
	I2C::Connection i2c(0);

	Genode::log("\nTesting ADV7511 interface on address ", Hex(Config::ADV7511_ADDR_1), 
					" via I2C controller at: ", Hex(Board_base::I2C0_MMIO_BASE));
	dump_registers(i2c, Config::ADV7511_ADDR_1);

	return 0;
}
