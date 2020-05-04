#include <vector>
#include <stdint.h>
#include "block.hpp"
#include "l2p.hpp"
#include <queue>

using namespace std;

class ssd {
	private:
		vector <block *> block_array;
		unsigned long num_blocks_in_ssd;
		unsigned long size_of_block;
		unsigned long num_pages_per_block;
		int current_block_number;

		l2p *ssd_l2p;

		float reserved_percentage;

		int valid_pages_cnt;

		int sage;
		long rewritten;

		unsigned max_invalid_cell_threshold;
		unsigned max_invalid_page_threshold;
		unsigned max_invalid_block_threshold;

		void invokeGC(int n);
		void preinvokeGC();

		queue <int> gc_trace;
		int gc_threshold;


	public:
		ssd(int num_blocks, int bs, int ps, int ssd_cell_type);
		~ssd();

		void setValidPages(int n);
		int getValidPages();
		void incSAge();
		int getSAge();

		void incRewritten(int n);
		long getRewritten();

		float getAverageErased();

		int write_to_disk(uint8_t *buf, vector <int> v);
		int read_from_disk(uint8_t *buf, int size);
		int init_write(uint8_t *buf, int size);

		void gc_update(int n);
		int get_gc_threshold();

};

