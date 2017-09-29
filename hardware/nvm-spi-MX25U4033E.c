/*
 * nvm-spi-MX25U4033E.c
 *
 *  Created on: 27 Jul 2015
 *      Author: matti.haugwitz
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hardware/nvm-spi-MX25U4033E.h>
#include <pl/assert.h>

#define LOG_TAG "nvm_MX25U4033E"
#include <pl/utils.h>

static int send_cmd(struct pl_spi *spi, uint8_t cmd);
//remove static int nvm_MX25U4033E_spi_pgm(struct pl_nvm * nvm, unsigned int addr, uint8_t * blob, int len);
static int nvm_MX25U4033E_spi_read(struct pl_nvm * nvm, unsigned int addr, uint8_t * blob, int len);

int nvm_MX25U4033E_spi_init(struct pl_nvm * nvm, struct pl_spi * spi){

	nvm->hw_ref = spi;
	nvm->read = nvm_MX25U4033E_spi_read;
	//remove nvm->pgm = nvm_MX25U4033E_spi_pgm;
	nvm->size = MX25U4033E_SIZE;

	return 0;
}

static int send_cmd(struct pl_spi *spi, uint8_t cmd){

	int stat = spi->write_bytes(spi, &cmd, 1);
	return stat;
}

static int nvm_MX25U4033E_spi_read(struct pl_nvm * nvm, unsigned int addr, uint8_t * blob, int len){

	assert(blob);

	int stat = -1;
	uint32_t register_address = addr;
	uint8_t reg[3];
	int chunkSize = 256;
	uint32_t byte_offset = 0;
	int bytes_to_transfer = len;
	uint8_t *data;

	struct pl_spi spi = *(struct pl_spi *) nvm->hw_ref;
	spi.mSpi->msh = 12000000; // set spi read speed to 12MHz to

	// open spi
	stat = spi.open(&spi);

	while(bytes_to_transfer > 0){

		// transfer chunkSize or bytes to transfer
		size_t transferChunkSize = (bytes_to_transfer >= chunkSize) ? chunkSize : bytes_to_transfer;

		reg[0] = (register_address >> 16) & 0xff;
		reg[1] = (register_address >> 8) & 0xff;
		reg[2] = (uint8_t) register_address;

		data = &(blob[byte_offset]);


		stat = spi.set_cs(&spi, 0);
		stat = send_cmd(&spi, MX25U4033E_READ);			// read command
		stat = spi.write_bytes(&spi, reg, 3);			// write 3-byte address
		stat = spi.read_bytes(&spi, data, transferChunkSize);		// read data
		stat = spi.set_cs(&spi, 1);

		byte_offset += transferChunkSize;
		register_address += transferChunkSize;
		bytes_to_transfer -= transferChunkSize;
	}

	return stat;
}
/*//remove
static int nvm_MX25U4033E_spi_pgm(struct pl_nvm * nvm, unsigned int addr, uint8_t * blob, int len){

	assert(blob);

	int stat = -1;
	uint32_t register_address = addr;
	uint8_t reg[3];
	int chunkSize = 256;
	uint32_t byte_offset = 0;
	int bytes_to_transfer = len;
	uint8_t *data;
	uint8_t cmp_blob[len];
	uint8_t buf = 0;

	struct pl_spi *spi = (struct pl_spi *) nvm->hw_ref;

	// open spi
	stat = spi->open(spi);

	// send write enable
	stat = spi->set_cs(spi, 1);
	stat = spi->set_cs(spi, 0);
	stat = send_cmd(spi, MX25U4033E_WREN);
	stat = spi->set_cs(spi, 1);

	// send chip erase
	stat = spi->set_cs(spi, 0);
	stat = send_cmd(spi, MX25U4033E_CE);
	//uint8_t reg_[3] = {0x00, 0x00, 0x00};
	//stat = spi->write_bytes(spi, reg_, 3);
	stat = spi->set_cs(spi, 1);

	// poll for chip erase has finished (WEL=0, WIP=0)
	do
	{
		stat = spi->set_cs(spi, 0);
		stat = send_cmd(spi, MX25U4033E_RDSR);
		spi->read_bytes(spi, &buf, 1);
		stat = spi->set_cs(spi, 1);
	}
	while(buf >= MX25U4033E_STATUS_WIP);

	// write new data to the flash
	while(bytes_to_transfer > 0){

		// transfer chunkSize or bytes to transfer
		size_t transferChunkSize = (bytes_to_transfer >= chunkSize) ? chunkSize : bytes_to_transfer;

		reg[0] = (register_address >> 16) & 0xff;
		reg[1] = (register_address >> 8) & 0xff;
		reg[2] = (uint8_t) register_address;

		data = &(blob[byte_offset]);

		// send write enable
		stat = spi->set_cs(spi, 0);
		stat = send_cmd(spi, MX25U4033E_WREN);
		stat = spi->set_cs(spi, 1);

		// write data
		stat = spi->set_cs(spi, 0);
		stat = send_cmd(spi, MX25U4033E_PP);						// write page program command
		stat = spi->write_bytes(spi, reg, 3);						// write 3-byte address
		stat = spi->write_bytes(spi, data, transferChunkSize);		// write data
		stat = spi->set_cs(spi, 1);

		// poll for page programming has finished (WEL=0, WIP=0)
		do
		{
			stat = spi->set_cs(spi, 0);
			stat = send_cmd(spi, MX25U4033E_RDSR);
			spi->read_bytes(spi, &buf, 1);
			stat = spi->set_cs(spi, 1);
		}
		while(buf >= MX25U4033E_STATUS_WIP);

		byte_offset += transferChunkSize;
		register_address += transferChunkSize;
		bytes_to_transfer -= transferChunkSize;
	}

	// data verification
	nvm_MX25U4033E_spi_read(nvm, addr, cmp_blob, len);

	if (memcmp(blob, cmp_blob, len) != 0)
	{
		LOG("NVM programming finished: verify after pgm failed!\n");
		return -1;
	}
	else
	{
		printf("NVM programming finished: verification successful!\n");
	}

	return stat;
}
//*/

