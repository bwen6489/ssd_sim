#include "ssd.hpp"
#include <iostream>
#include <string>

using namespace std;

ssd :: ssd(int num_blocks, int bs, int ps, int ssd_cell_type)
{
	int num_pages = num_blocks * bs;
	num_blocks_in_ssd = num_blocks;
	num_pages_per_block = bs;
	size_of_block = bs * ps * ssd_cell_type;

	current_block_number = 0;
	max_invalid_cell_threshold = 80;
	max_invalid_block_threshold = 80;
	max_invalid_page_threshold = 80; 

	ssd_l2p = new l2p(num_blocks * bs,num_pages_per_block);

	//reserved_percentage = 0.1;
	//int reserved_block = int(num_blocks * reserved_percentage);

	valid_pages_cnt = num_pages * (1-0.8/*-reserved_percentage*/);

	for (int i = 0 ; i < num_blocks ; i++) {
		block_array.push_back(new block(bs, ps ,ssd_cell_type, 0, NO_WOM));
		block_array[i]->setBlockNumber(i);
	}
/*
	for (int i = 0 ; i < reserved_block ; i++) {
		block_array[i]->setBlockStatus(RESERVED);
	}
*/

	long rewritten=0;
	
	sage=0;
	gc_threshold=0;

}


void ssd :: setValidPages(int n)
{
	valid_pages_cnt=n;
}

int ssd :: getValidPages()
{
	return valid_pages_cnt;
}

void ssd :: incSAge()
{
	sage++;
}

int ssd :: getSAge()
{
	return sage;
}

float ssd:: getAverageErased()
{
	int temp=0;
	for (int i;i<num_blocks_in_ssd;i++)
	{
		temp+=block_array[i]->getBErase();
	}
	return temp;
}

void ssd :: incRewritten(int n)
{
	rewritten+=n;
}

long ssd :: getRewritten()
{
	return rewritten;
}

void ssd :: gc_update(int n)
{
	if (gc_trace.size()>=3)
	{
		gc_threshold=(gc_threshold*5-gc_trace.front()+n)/5;
		gc_trace.pop();
		gc_trace.push(n);
	}
	else
	{
		gc_trace.push(n);
		gc_threshold=(gc_threshold*(gc_trace.size()-1)+n)/gc_trace.size();
	}
}

int ssd :: get_gc_threshold()
{
	return gc_threshold;
}

/* buf is initialized by caller */

int ssd :: init_write(uint8_t *buf, int size)
{
	int needed_blocks = size / size_of_block;
	assert(needed_blocks <= num_blocks_in_ssd); 
	uint8_t *block_buf = buf;
	int total_bytes_written = 0;

	incSAge();

	// uint8_t *rbdata = new uint8_t [size]();

	int lbn=0;

	for (int i = 0; i < needed_blocks; i++) {
		//cout<<i<<endl;
		int offset = block_array[i]->writeToBlock(block_buf, size_of_block);
		block_buf += offset;
		total_bytes_written += offset;
		for (int j=0; j<num_pages_per_block;j++){
			ssd_l2p->setPBN(lbn,j+i*num_pages_per_block);
			lbn++;
		}
		block_array[i]->setBAge(getSAge());
		//cout<<lbn<<endl;
		block_array[i]->setBlockStatus(FULL);
	}

	return total_bytes_written;
}
int ssd :: write_to_disk(uint8_t *buf, vector <int> v)
{

	// calculate number of LBN to be updated. (here).
	incSAge();
	
	int page_size=size_of_block / num_pages_per_block;
	int needed_lbn = v.size();

	for (int i = 0 ; i < needed_lbn ; i++){
		//cout<<v[i]<<endl;
		ssd_l2p->invalidateL2PMap(v[i]);
	}

	bool gc_flag=false;

	cout<<"valid"<<getValidPages()<<endl;
	if (getValidPages() < needed_lbn)
	{
		gc_flag=true;
		gc_update(getValidPages());
		invokeGC(needed_lbn);
	}
	else 
		setValidPages(getValidPages()-needed_lbn);

	cout<<"needed_lbn "<<needed_lbn<<endl;

	
	// check which of them are overwrites (shouldn't have lbn-pbn as LONG_MAX). (l2p.getPBN)
	// invalidate these pages. (block.InvalidatePage()). l2pMap.invalidatePage)

	// allocate new PBNs for all LBNs. (l2pMap.map_lbns_to_pbns)
	// A. get pbn of each new lbn (getPBN())

	// TODO if PBN is of a block marked for gc. request for additional pages.
	// go back to A. asking for new lbns = pbns marked for GC.

	// TODO beyond 4th write, read PBNs, check unprogrammable cells. check number
	// of additional physical pages to be assigned.
	// while goto A, asking for new lbns = unprogrammable cells / page_size.

	// get block on which write has to happen.
	// generate page vector from buf.
	// do write_to_block(block_no, vector_of_pages);

	// Find number of blocks needed to write the buffer
	//int needed_blocks = size / bytes_per_block;
	//assert(needed_blocks <= num_blocks_in_ssd); 
	

	uint8_t *block_buf = buf;
	int total_bytes_written = 0;

	int i = 0;
	int lbn = 0;
	// uint8_t *rbdata = new uint8_t [size]();

	while (lbn<needed_lbn)
	{
		//cout<<block_array[i]->getBErase()<<endl;
		//cout<<"lbn"<<lbn<<endl;
		//cout<<"block"<<i<<block_array[i]->getBlockStatus()<<endl;
		int cur_pbn = i * num_pages_per_block;
		if ((block_array[i]->getBlockStatus()==PARTIAL_FULL) or (block_array[i]->getBlockStatus()==EMPTY))
		{
			block_array[i]->setBAge(getSAge());
			int pbn = block_array[i]->getCurrentPhysicalPage();
			while ((pbn!=num_pages_per_block) and (lbn<needed_lbn))
			{
				//cout<<"lbn"<<lbn<<"pbn"<<cur_pbn+pbn<<endl;
				logical_page *l = new logical_page(page_size);
				l->setBuf(block_buf, page_size);
				block_array[i]->writePage(pbn % num_pages_per_block,l);
				ssd_l2p->setPBN(v[lbn],cur_pbn+pbn);
				pbn++;
				lbn++;
			}
			block_array[i]->setCurrentPhysicalPage(pbn % num_pages_per_block);
			if (pbn % num_pages_per_block == 0)
				block_array[i]->setBlockStatus(FULL);
			else
				block_array[i]->setBlockStatus(PARTIAL_FULL);

		}
		i++;
	}

	assert(buf != NULL);

	cout<<getValidPages()<<" "<<get_gc_threshold()<<endl;
/*
	if ((getValidPages()<1*get_gc_threshold()) and (gc_flag))
	{
		gc_update(get_gc_threshold());
		preinvokeGC();
	}
*/
	return 0;

}

/* buf is initialized by caller */
int ssd :: read_from_disk(uint8_t *buf, int size)
{
	assert(buf != NULL);
	return 0;
}

ssd :: ~ssd()
{
	for (int i = 0 ; i < num_blocks_in_ssd ; i++) {
		delete block_array[i];
	}
}

void swap(float *a,float *b)
{
    float temp;
    temp=a[0];
    a[0]=b[0];
    b[0]=temp;
    temp=a[1];
    a[1]=b[1];
    b[1]=temp;

}

void heapify(float arr[][2], int n, int i) 
{ 
    int largest = i; // Initialize largest as root 
    int l = 2 * i + 1; // left = 2*i + 1 
    int r = 2 * i + 2; // right = 2*i + 2 
  
    // If left child is larger than root 
    if (l < n && arr[l][1] < arr[largest][1]) 
        largest = l; 
  
    // If right child is larger than largest so far 
    if (r < n && arr[r][1] < arr[largest][1]) 
        largest = r; 
  
    // If largest is not root 
    if (largest != i) { 
        swap(arr[i], arr[largest]); 
  
        // Recursively heapify the affected sub-tree 
        heapify(arr, n, largest); 
    } 
} 
  
// main function to do heap sort 
void heapSort(float arr[][2], int n) 
{ 
    // Build heap (rearrange array) 
    for (int i = n / 2 - 1; i >= 0; i--) 
        heapify(arr, n, i); 
  
    // One by one extract an element from heap 
    for (int i = n - 1; i >= 0; i--) { 
        // Move current root to end 
        swap(arr[0], arr[i]); 
  
        // call max heapify on the reduced heap 
        heapify(arr, i, 0); 
    } 
} 

void ssd :: invokeGC(int n)
{
	// go through all blocks. arrange them in decreasing order of number of invalidated pages.
	// TODO: block.c get interface for getting number of invalid pages in each block.
	// select top X blocks for erasing. get number of valid pages in this block.
	// TODO: block.c get interface for getting vector of valid pages in block.
	// read all valid pages in a buffer.
	// TODO: get interface for reading page data from a block. getPageData(page_no);
	// call write_to_ssd() with buffer read, for all pages in current block. set block status to READY_TO_ERASE
	// set blockStatus to ERASING. reset all pages in block. make sure all pages are invalidated.
	// set blockStatus to EMPTY.

	cout<<endl<<"gc"<<endl;
	float a[num_blocks_in_ssd][2];
//	memset(a, 0, sizeof(a));
	int invalid_page_cnt[num_blocks_in_ssd];

	int pbn;
	for (int i = 0 ; i < num_blocks_in_ssd ; i++) {
		/*
		if (block_array[i]->getBlockStatus()==RESERVED)
			block_array[i]->setBlockStatus(EMPTY);
			*/
		a[i][0]=i;
		pbn=i*num_pages_per_block;
		invalid_page_cnt[i]=0;
		for (int j = 0; j < num_pages_per_block ;j++) {
			if ((ssd_l2p->getLBN(pbn+j)==LONG_MAX) and (block_array[i]->checkPageEmpty(j))==0)
				invalid_page_cnt[i]++;
		}
		//greedy:
		a[i][1]=invalid_page_cnt[i];

		//CB:
		float ub=(float)(num_pages_per_block-invalid_page_cnt[i]+1)/(float)(num_pages_per_block+1);

		//if ((float)(ssd_l2p->getUpdate())/(float)(getSAge())>0.8)
		//	a[i][1]=(getSAge()-block_array[i]->getBAge())*(1-ub)/(2*ub);

		//cout<<invalid_page_cnt[i]<<" "<<a[i][1]<<" "<<ub<<endl;

		//cout<<"i="<<i<<" invalid="<<invalid_page_cnt[i]<<" status="<<block_array[i]->getBlockStatus()<<endl;
	}

	heapSort(a,num_blocks_in_ssd);

	int page_size=size_of_block / num_pages_per_block;

	uint8_t *buf;
	/*
	int reserved_block = int(num_blocks_in_ssd * reserved_percentage);
	int reserve = 0;
	*/
	logical_page *l = new logical_page(page_size);
	
	vector <int> lbn;
	vector <logical_page *> valid_pages;
	
	int free_pages=getValidPages();
	int i = 0;
	while ((free_pages<n)/* or (reserve<reserved_block)*/)
	{
		//random
		//i=rand() % num_blocks_in_ssd;
		if (block_array[a[i][0]]->getBlockStatus()==EMPTY)
			continue;
		cout<<"free_pages"<<free_pages<<endl;
		free_pages+=invalid_page_cnt[int(a[i][0])];
		pbn=a[i][0]*num_pages_per_block;
		for (int j = 0; j < num_pages_per_block ;j++)
		{
			//cout<< ssd_l2p->getLBN(pbn+j);
			if (ssd_l2p->getLBN(pbn+j)!=LONG_MAX)
			{
				block_array[a[i][0]]->readPage(j,l);
				valid_pages.push_back(l);
				lbn.push_back(ssd_l2p->getLBN(pbn+j));
				ssd_l2p->invalidateL2PMap(ssd_l2p->getLBN(pbn+j));
			}
		}
		cout<<"choose"<<a[i][0]<<endl;
		block_array[a[i][0]]->setBlockStatus(MARKED_FOR_GC);
		block_array[a[i][0]]->eraseBlock();
		block_array[a[i][0]]->setBAge(getSAge());
		
		/*if (reserve<reserved_block)
			{
				block_array[a[i][0]]->setBlockStatus(RESERVED);
				reserve++;
			}
		else*/
		block_array[a[i][0]]->setBlockStatus(EMPTY);
		i++;
	}
	setValidPages(free_pages-n);

	i = 0;
	int cntlbn = 0;

	incRewritten(lbn.size());

	cout<<"choose end"<<endl;


	while (cntlbn<lbn.size())
	{
		if ((block_array[i]->getBlockStatus()==PARTIAL_FULL) or (block_array[i]->getBlockStatus()==EMPTY))
		{
			//cout<<"i"<<i<<endl;
			int pbn = block_array[i]->getCurrentPhysicalPage();
			while ((pbn!=num_pages_per_block) and (cntlbn<lbn.size()))
			{
				//cout<<"pbn"<<i*num_pages_per_block+pbn<<endl;
				block_array[i]->writePage(pbn,valid_pages[cntlbn]);
				ssd_l2p->setPBN(lbn[cntlbn],i*num_pages_per_block+pbn);
				cntlbn++;
				pbn++;
			}
			block_array[i]->setCurrentPhysicalPage(pbn % num_pages_per_block);
			if (pbn % num_pages_per_block == 0)
				block_array[i]->setBlockStatus(FULL);
			else
				block_array[i]->setBlockStatus(PARTIAL_FULL);
			block_array[i]->setBAge(getSAge());
		}
		i++;
	}


}


void ssd :: preinvokeGC()
{
	cout<<endl<<"preinvokeGC"<<endl;

	float a[num_blocks_in_ssd][2];
//	memset(a, 0, sizeof(a));
	int invalid_page_cnt[num_blocks_in_ssd];

	int pbn;
	for (int i = 0 ; i < num_blocks_in_ssd ; i++) {

		a[i][0]=i;
		pbn=i*num_pages_per_block;
		invalid_page_cnt[i]=0;
		for (int j = 0; j < num_pages_per_block ;j++) {
			if ((ssd_l2p->getLBN(pbn+j)==LONG_MAX) and (block_array[i]->checkPageEmpty(j))==0)
				invalid_page_cnt[i]++;
		}
		cout<<invalid_page_cnt[i]<<endl;

		//greedy:
		a[i][1]=invalid_page_cnt[i];

		//CB:
		float ub=(num_pages_per_block-a[i][1]+1)/(num_pages_per_block+1);
		//a[i][1]=(getSAge()-block_array[i]->getBAge())*(1-ub)/(2*ub);

		//cout<<"i="<<i<<" invalid="<<invalid_page_cnt[i]<<" status="<<block_array[i]->getBlockStatus()<<endl;
	}

	heapSort(a,num_blocks_in_ssd);

	int page_size=size_of_block / num_pages_per_block;

	uint8_t *buf;

	logical_page *l = new logical_page(page_size);
	
	vector <int> lbn;
	vector <logical_page *> valid_pages;
	
	int free_pages=getValidPages();
	int i = 0;
	while ((i<5))
	{
		cout<<"free_pages"<<free_pages<<endl;
		free_pages+=invalid_page_cnt[int(a[i][0])];
		pbn=a[i][0]*num_pages_per_block;
		for (int j = 0; j < num_pages_per_block ;j++)
		{
			//cout<< ssd_l2p->getLBN(pbn+j);
			if (ssd_l2p->getLBN(pbn+j)!=LONG_MAX)
			{
				block_array[a[i][0]]->readPage(j,l);
				valid_pages.push_back(l);
				lbn.push_back(ssd_l2p->getLBN(pbn+j));
				ssd_l2p->invalidateL2PMap(ssd_l2p->getLBN(pbn+j));
			}
		}
		cout<<"choose"<<a[i][0]<<endl;
		block_array[a[i][0]]->setBlockStatus(MARKED_FOR_GC);
		block_array[a[i][0]]->eraseBlock();
		block_array[a[i][0]]->setBAge(getSAge());
		
		/*if (reserve<reserved_block)
			{
				block_array[a[i][0]]->setBlockStatus(RESERVED);
				reserve++;
			}
		else*/
			block_array[a[i][0]]->setBlockStatus(EMPTY);
		i++;
	}
	setValidPages(free_pages);

	i = 0;
	int cntlbn = 0;

	incRewritten(lbn.size());

	cout<<"rewritten"<<lbn.size();

	while (cntlbn<lbn.size())
	{
		if ((block_array[i]->getBlockStatus()==PARTIAL_FULL) or (block_array[i]->getBlockStatus()==EMPTY))
		{
			//cout<<"i"<<i<<endl;
			int pbn = block_array[i]->getCurrentPhysicalPage();
			while ((pbn!=num_pages_per_block) and (cntlbn<lbn.size()))
			{
				//cout<<"pbn"<<i*num_pages_per_block+pbn<<endl;

				block_array[i]->writePage(pbn,valid_pages[cntlbn]);
				ssd_l2p->setPBN(lbn[cntlbn],i*num_pages_per_block+pbn);
				cntlbn++;
				pbn++;
			}
			block_array[i]->setCurrentPhysicalPage(pbn % num_pages_per_block);
			if (pbn % num_pages_per_block == 0)
				block_array[i]->setBlockStatus(FULL);
			else
				block_array[i]->setBlockStatus(PARTIAL_FULL);
			block_array[i]->setBAge(getSAge());
		}
		i++;
	}


}
