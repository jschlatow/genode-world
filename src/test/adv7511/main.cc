#include <base/log.h>

#include <drivers/board_base.h>
#include <drivers/i2c.h>
#include <drivers/adv7511.h>

using namespace Genode;

struct Config {
	enum {
		ADV7511_ADDR_1 = 0x39,
		ADV7511_ADDR_2 = 0x3d,
	};
};

int main()
{
	I2c_driver driver(Board_base::I2C0_MMIO_BASE, Board_base::I2C_MMIO_SIZE);
	Adv7511 adv7511(Config::ADV7511_ADDR_1, driver);


	Genode::log("\nTesting ADV7511 interface on address ", Hex(Config::ADV7511_ADDR_1), 
					" via I2C controller at: ", Hex(Board_base::I2C0_MMIO_BASE));

	adv7511.dump_registers();

	return 0;
}
