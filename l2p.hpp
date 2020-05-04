
#ifndef L2P_H
#define L2P_H
#include <map>
#include <climits>
#include <vector>

using namespace std;

class l2p {
	map <unsigned long, unsigned long> l2pMap;
	map <unsigned long, unsigned long> p2lMap;
	unsigned long mapsize;
	unsigned long current_physical_page;
	unsigned long num_pages_per_block;
	unsigned long num_page_allocated;
	bool map_full;

	map <unsigned long, unsigned long> lMapupdate;

	unsigned long getNextEmptyPage();

	public:
		l2p(unsigned long size, unsigned long np_per_block);
		void invalidateL2PMap(unsigned long lbn);
		int setPBN(unsigned long lbn,unsigned long pbn);
		unsigned long getPBN(unsigned long lbn);
		unsigned long getLBN(unsigned long pbn);
		unsigned long map_lbns_to_pbns(unsigned long num_pages, vector <unsigned long> logical_page_numbers);
		unsigned long getNumPageAllocated();
		unsigned long getUpdate();
};
#endif