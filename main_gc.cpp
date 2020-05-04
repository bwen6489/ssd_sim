#include "ssd.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>

using namespace std;

long gc_test(ssd *mySSD,vector <int> v)
{
	int input_bytes = v.size() * 10 * 3;

	uint8_t *wdata = new uint8_t [input_bytes]();
	//uint8_t *rdata = new uint8_t [input_bytes]();

	for (int i = 0 ; i < input_bytes ; i++) {
		wdata[i] = 0xFF;
	}

	mySSD->write_to_disk(wdata, v);
	
	delete []wdata;
	return 0;
}


int main()
{
	int num_blocks = 25;
	int block_size = 100;
	int page_size = 50;
	int ssd_cell_type = 3;	
	ssd *mySSD = new ssd(num_blocks, block_size, page_size, ssd_cell_type);
/*
	int ssd_capacity_in_bytes = num_blocks * block_size * page_size * 3;

	uint8_t *wdata = new uint8_t [ssd_capacity_in_bytes]();
	uint8_t *rdata = new uint8_t [ssd_capacity_in_bytes]();

	for (int i = 0 ; i < ssd_capacity_in_bytes ; i++) {
		wdata[i] = 0xFF;
	}
	ssd *mySSD = new ssd(num_blocks, block_size, page_size, ssd_cell_type);

	
	mySSD->write_to_disk(wdata, ssd_capacity_in_bytes);
	mySSD->read_from_disk(rdata, ssd_capacity_in_bytes);
	for (int i = 0 ; i < ssd_capacity_in_bytes ; i++) {
		if(wdata[i] != rdata[i]) {
			printf("i=%d w=%d r=%d\n", i, wdata[i], rdata[i]);
		}
	}
	*/

	string line;
    stringstream ss;

    fstream infile("50-50.txt");

    cout<<"start"<<endl;

    int ssd_capacity_in_bytes = num_blocks * block_size * page_size * 3 * 0.8;

	uint8_t *wdata = new uint8_t [ssd_capacity_in_bytes]();

	for (int i = 0 ; i < ssd_capacity_in_bytes ; i++) {
		wdata[i] = 0xFF;
	}

	mySSD->init_write(wdata, ssd_capacity_in_bytes);

	cout<<"prewrite finish"<<endl;

        
    int i=0;
    while (getline(infile, line))
        {
            ss << line;

            int m;
            vector<int> v;
            while ( ss >> m ) {
                //cout << m << " ";
                v.push_back(m);
            }
            ss.str() = "";
            ss.clear();

            cout << "\n";
            gc_test(mySSD,v);
        }

    infile.close();

    cout<<endl<<"total rewritten"<<mySSD->getRewritten()<<endl;
    //cout<<"average erase"<<mySSD->getAverageErased()<<endl;

    
	//delete []rdata;
	delete mySSD;
	return 0;
}
