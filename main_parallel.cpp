#include "ssd.hpp"
#include <cstdint>

int main()
{
	int num_blocks = 10;
	// block size is the number of pages inside the block.
	int block_size = 20;
	int page_size = 10;
	int ssd_cell_type = 3;	

	int ssd_capacity_in_bytes = num_blocks * block_size * page_size * ssd_cell_type;

	uint8_t *wdata = new uint8_t [ssd_capacity_in_bytes]();
	uint8_t *rdata = new uint8_t [ssd_capacity_in_bytes]();

	for (int i = 0 ; i < ssd_capacity_in_bytes ; i++) {
		wdata[i] = 0xFF;
	}
	ssd *mySSD = new ssd(num_blocks, block_size, page_size, ssd_cell_type);

	/*
	mySSD->write_to_disk(wdata, ssd_capacity_in_bytes);
	mySSD->read_from_disk(rdata, ssd_capacity_in_bytes);
	for (int i = 0 ; i < ssd_capacity_in_bytes ; i++) {
		if(wdata[i] != rdata[i]) {
			printf("i=%d w=%d r=%d\n", i, wdata[i], rdata[i]);
		}
	}
	*/

	// NEW CODE-- does block level creation, block level write and read.

	block *b = new block(block_size, page_size , ssd_cell_type, 0, NO_WOM);

	printf("physical block size = %lu\n", b->getPhysicalBlockSize());
	printf("logical block size = %lu\n",b->getLogicalBlockSize());

	int block_capacity_in_bytes = b->getPhysicalBlockSize();

	uint8_t *wbdata = new uint8_t [block_capacity_in_bytes]();
	uint8_t *rbdata = new uint8_t [block_capacity_in_bytes]();

	for (int i = 0 ; i < block_capacity_in_bytes ; i++) {
		wbdata[i] = 0xFF;
	}

	b->writeToBlock(wbdata, block_capacity_in_bytes);
	b->readFromBlock(rbdata, block_capacity_in_bytes);

	for (int i = 0 ; i < block_capacity_in_bytes ; i++) {
		assert(wbdata[i] == rbdata[i]);
	}

	delete []wdata;
	delete []rdata;
	delete []wbdata;
	delete []rbdata;
	delete b;
	delete mySSD;
}
