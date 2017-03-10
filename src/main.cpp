#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <stfy/sys.hpp>
#include <stfy/hal.hpp>


#define SLAVE_REG_SIZE 512
#define SLAVE_REG_SMALL_SIZE 64
char slave_regs[SLAVE_REG_SIZE];


static I2C master(1);
static I2C slave(2);

int test_master_write(int slave_reg_size);
int test_master_read(int slave_reg_size);

int main(int argc, char * argv[]){

	//init slave regs
	memset(slave_regs, 0, SLAVE_REG_SIZE);

	if( master.init(400000) < 0 ){
		perror("failed to init master (i2c1)");
		exit(1);
	}

	if( slave.init() < 0 ){
		perror("failed to init slave (i2c2)");
		exit(1);
	}

	//init can cause glitches on the I2C bus that require the bus to be reset
	slave.reset();
	master.reset();

	if( slave.setup_slave(0x4C, slave_regs, SLAVE_REG_SMALL_SIZE) <  0 ){
		perror("failed to setup slave");
		exit(1);
	}

	master.reset();

	master.setup(0x4C);

	printf("Small write test\n");
	test_master_write(SLAVE_REG_SMALL_SIZE);
	printf("Small read test\n");
	test_master_read(SLAVE_REG_SMALL_SIZE);
	printf("Small read test complete\n");

	if( slave.setup_slave(0x4C, slave_regs, SLAVE_REG_SIZE) <  0 ){
		perror("failed to setup slave");
		exit(1);
	}

	master.setup(0x4C, I2C::SETUP_NORMAL_16_TRANSFER);

	printf("Write test\n");
	test_master_write(SLAVE_REG_SIZE);
	printf("Read test\n");
	test_master_read(SLAVE_REG_SIZE);


	printf("Tests complete\n");

	return 0;
}

int test_master_write(int slave_reg_size){
	char wbuffer[16];
	int ret;
	u8 value;
	int i;

	for(i=0; i < slave_reg_size; i++){
		value = 0xAA + i;
		memset(wbuffer, value, 16);
		Timer::wait_msec(5);

		ret = master.write(i, wbuffer, 16);
		if( ret <= 0 ){
			perror("Failed to write master (i2c1)");
			printf("I2C Error %d\n", master.err());
		} else {
			if( memcmp(wbuffer, slave_regs + i, ret) != 0 ){
				printf("Failed to write correctly at %d\n", i);
				printf("0x%X != 0x%X (%d)\n", value, slave_regs[i], ret);
			}
		}
	}

	for(i=0; i < slave_reg_size; i++){
		if( slave_regs[i] != ((0xAA + i) & 0xFF) ){
			printf("Data failed at %d (0x%X != 0x%X)\n", i, slave_regs[i], 0xAA+i);
		}
	}

	return 0;
}

int test_master_read(int slave_reg_size){
	char buffer[16];
	int ret;
	char value;
	int i;
	int size;

	for(i=0; i < slave_reg_size; i++){
		if( i < 256 ){
			value = 0xAA + i;
		} else {
			value = 0x55 + i;
		}
		size = 16;
		if( i + size >= slave_reg_size){
			size = slave_reg_size - i;
		}
		memset(slave_regs + i, value, size);
		Timer::wait_msec(5);

		ret = master.read(i, buffer, size);

		if( ret != size ){
			printf("RET != size (%d != %d)\n", ret, size);
		}

		if( ret <= 0 ){
			perror("Failed to read master");
			printf("I2C Error %d\n", master.err());
		} else {
			if( memcmp(buffer, slave_regs + i, ret) != 0 ){
				printf("Failed to read correctly at %d (%d)\n", i, ret);
				for(int j=0; j < size; j++){
					printf("0x%X != 0x%X\n", buffer[j], slave_regs[i+j]);
				}
				exit(1);
			} else {
				//printf("Read %d bytes at %d\n", ret, i);
			}
		}
	}
	return 0;
}
