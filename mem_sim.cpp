//Cache and memory simulator
//Author: Wuzheng Ma
//Email: wuzheng.ma13@imperial.ac.uk
//Date: 10.12.2014

#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<math.h>
#include <stdio.h>
#include<sstream>

using namespace std;

struct cache_struct{
	int data;
	int dirty_bit;
	int LRU_count;
	int byte_address;

};

static int count_LRU = 0;


vector<int> create_memory(){
	//Since the max size of memory is 8MB
	int mem_size = 8 * 1024 * 1024;

	vector<int> memory (mem_size);

	for(int i=0; i<mem_size; i++){
		memory[i] = 0;
	}

	return memory;
} 


vector< vector<cache_struct> > create_cache(int add_bit, int bt_wd, int wd_bk, int bk_set, int set_ca,
	int c_hit, int c_mr, int c_mw){
		
		int horizontal = bk_set*(1+1+wd_bk);
		int vertical = set_ca;
		
		vector< vector<cache_struct> > cache;
		cache.resize(vertical);
		for (int i=0; i<vertical; i++){
			cache[i].resize(horizontal); 
		}

		for(int i=0; i<vertical; i++){
			for(int j=0; j<horizontal; j++){
				cache[i][j].data = 0;
				cache[i][i].dirty_bit = 0;
				cache[i][j].LRU_count = 0;
				cache[i][j].byte_address = 0;
			}
		}

		return cache;
}


void Flush(int bt_wd, int wd_bk, int set_index, int block_start, vector< vector< cache_struct> >& cache, vector<int>& memory){
	//Write the previous data stored in that block back to memory before it is modified 
	int data_stored = 0;
	int data_start_wt = block_start+2;
	int btaddress_store = cache[set_index][data_start_wt].byte_address;
	for(int x=0; x<wd_bk; x++){
		int pre_data = cache[set_index][data_start_wt+x].data;
		for(int j=0; j<bt_wd; j++){
			data_stored = pre_data & 0xFF;
			memory[btaddress_store+j] = data_stored;		//Stores byte to each memory address
			pre_data = pre_data >> 8;
		}
		btaddress_store = btaddress_store + bt_wd;
	}
}

void flush_request(int bt_wd, int wd_bk, int bk_set, int set_ca, vector< vector<cache_struct> >& cache, vector<int>& memory){
	int dirty_bit_pos = 0; 
	for(int i=0; i<set_ca; i++){
		for(int j=0; j<bk_set; j++){
			dirty_bit_pos = j*(wd_bk+2);
			Flush(bt_wd, wd_bk, i, j, cache, memory);
			cache[i][dirty_bit_pos].dirty_bit = 0;
		}
	}
	cout<<"flush-ack"<<endl;
}


void write_request(int address, int add_bit, int bt_wd, int wd_bk, int bk_set, int set_ca, int c_hit, int c_mr, int c_mw,
	int data, vector< vector<cache_struct> >& cache, vector<int>& memory){
	
		int time = 0;
		int tag = 0;
		bool tag_match = false;
		count_LRU++;

		int word_address = address / bt_wd;
		int block_address = word_address / wd_bk;
		int word_index = word_address % wd_bk;					//the word index in one block
		int block_index;
		int set_index;
		if(bk_set == 1){
			block_index = block_address % (set_ca / bk_set);
			set_index = block_index;		//Directed mapping, only one block per set
		}else{
			set_index = block_address % set_ca;
		}
		tag = block_address;

		if(data >= pow(16.0, (2*bt_wd))){		//Check whether data is out of range
			cout<<"Invalid data, data is out of range."<<endl;
		}

		//Print out the state of cache before writing to it.
		stringstream ss;
		int tag_pos;
		ss<<"# ";
		for(int i=0; i<set_ca; i++){
			ss<<"S"<<i<<"{";
			for(int j=0; j<bk_set; j++){
				tag_pos = j * (wd_bk + 2) + 1;
				if(cache[i][tag_pos-1].data == 1){				//Check valid bit
					if(cache[i][tag_pos-1].dirty_bit == 1){		//Check Dirty bit
						ss<<"B"<<cache[i][tag_pos].data<<"/d ";
					}else{
						ss<<"B"<<cache[i][tag_pos].data<<" ";
					}
				}else{
					ss<<"NULL ";
				}
			}
			ss<<"}, ";
		}
		ss<<"write-ack B"<<block_address;
		cout<<ss.str()<<endl;


		//if tag matches, and valid bit is 1, then it is a cache hit
		//for(unsigned int i=1; i<cache[set_index].size(); i=i+wd_bk+1+1){
		
		if(bk_set == 1){			//Dirct mapped cache
			int cur_tag_pos = 1;			
			if(cache[set_index][0].data == 1){							//Valid bit valids
				if(cache[set_index][cur_tag_pos].data == tag){			//Tag matches		
					cache[set_index][cur_tag_pos+1+word_index].data = data;				//Wirte data
					cache[set_index][cur_tag_pos+1+word_index].byte_address = address;	//Save the byte address
					cache[set_index][0].dirty_bit = 1;						//Set dirty bit to one, so next time cache data has
																				//to be written back to mem firstly
					time = c_hit;
					cout<<"write-ack "<<set_index<<" Hit! "<<time<<endl;
				}else{	   

					if(cache[set_index][0].dirty_bit == 0){		//Check Dirty bit
						cache[set_index][cur_tag_pos+1+word_index].data = data;	
						cache[set_index][cur_tag_pos+1+word_index].byte_address = address;
						cache[set_index][0].dirty_bit = 1;
						cache[set_index][cur_tag_pos].data = tag;
						time = c_hit + c_mr;
						cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;
					}else{
						Flush(bt_wd, wd_bk, set_index, cur_tag_pos-1, cache, memory);
						//Then modifies the block
						cache[set_index][cur_tag_pos+1+word_index].data = data;
						cache[set_index][cur_tag_pos+1+word_index].byte_address = address;
						cache[set_index][cur_tag_pos+1+word_index].dirty_bit = 1;
						cache[set_index][cur_tag_pos].data = tag;
						time = c_hit + c_mw + c_mr;
						cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;	
					}
			
				}
			}else{
				cache[set_index][cur_tag_pos+1+word_index].data = data;				//Wirte data
				cache[set_index][cur_tag_pos+1+word_index].byte_address = address;	//Save byte_address
				cache[set_index][0].dirty_bit = 1;									//Set Dirty bit
				cache[set_index][cur_tag_pos].data = tag;							//Set Tag
				cache[set_index][cur_tag_pos-1].data = 1;							//Set Valid bit
				time = c_hit + c_mr;
				cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;
			}
		}else{						// associative cache
			int cur_tag_pos = 0;
			for(int i=0; i<bk_set; i++){
				cur_tag_pos = i * (wd_bk + 2) + 1;
					if((cache[set_index][cur_tag_pos].data == tag) && (cache[set_index][i*(wd_bk+2)].data == 1)){				//Check tag
						tag_match = true;				
						cache[set_index][cur_tag_pos+1+word_index].data = data;
						cache[set_index][cur_tag_pos+1+word_index].byte_address = address;
						cache[set_index][cur_tag_pos-1].LRU_count = count_LRU;
						cache[set_index][cur_tag_pos-1].dirty_bit = 1;
						time = c_hit;
						cout<<"'write-ack' "<<set_index<<" Hit! "<<time<<endl;
						break;					
					//Tag miss or valid bit low	
					}else if((cache[set_index][cur_tag_pos].data != tag) && (cache[set_index][i*(wd_bk+2)].data == 1)){	//tag miss
						tag_match = false;
						continue;
					}else if(cache[set_index][i*(wd_bk+2)].data == 0){	//valid bit low
						cache[set_index][cur_tag_pos+1+word_index].data = data;				//Wirte data
						cache[set_index][cur_tag_pos+1+word_index].byte_address = address;	//Save byte_address
						cache[set_index][cur_tag_pos-1].dirty_bit = 1;									//Set Dirty bit
						cache[set_index][cur_tag_pos].data = tag;							//Set Tag
						cache[set_index][cur_tag_pos-1].data = 1;							//Set Valid bit
						cache[set_index][cur_tag_pos-1].LRU_count = count_LRU;
						tag_match = true;
						time = c_hit + c_mr;
						cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;		
						break;
					}
			}

			if(tag_match == false){
				//tag miss, LRU policy is used 	
				int count_offset = wd_bk+1+1;			//count value for each block
																//is stored at a index which is proportional to count_offset
					int write_index = 0;
					int min = cache[set_index][0].LRU_count;
					for(int k=0; k<bk_set; k++){		
						if(cache[set_index][k*count_offset].LRU_count < min){
							min = cache[set_index][k*count_offset].LRU_count;
							write_index = k;
						}
					}
					if(cache[set_index][write_index*count_offset].dirty_bit == 0){
						cache[set_index][write_index*count_offset+2+word_index].data = data;
						cache[set_index][write_index*count_offset+2+word_index].byte_address = address;
						cache[set_index][write_index*count_offset+1].data = tag;
						cache[set_index][write_index*count_offset].LRU_count = count_LRU;
						cache[set_index][write_index*count_offset].dirty_bit = 1;
						time = c_hit + c_mr;
						cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;	//write-ack 0 miss 5
					}else{
						Flush(bt_wd, wd_bk, set_index, write_index*count_offset, cache, memory);
							//Then modifies the block
						cache[set_index][write_index*count_offset+2+word_index].data = data;
						cache[set_index][write_index*count_offset+2+word_index].byte_address = address;
						cache[set_index][write_index*count_offset+1].data = tag;
						cache[set_index][write_index*count_offset].LRU_count = count_LRU;
						cache[set_index][write_index*count_offset].dirty_bit = 1;
						time = c_hit + c_mw + c_mr;
						cout<<"write-ack "<<set_index<<" Miss! "<<time<<endl;	//write-ack 0 miss 5
					}
					
			}
	
		}
}

void read_request(int address, int add_bit, int bt_wd, int wd_bk, int bk_set, int set_ca, int c_hit, int c_mr, int c_mw,
	vector< vector<cache_struct> >& cache, vector<int>& memory){

		int tag;
		int time = 0;
		int data_read = 0;
		bool tag_match = false;
		count_LRU++;
		int word_address = address / bt_wd;
		int word_index = word_address % wd_bk;
		int block_address = word_address / wd_bk;
		int block_index;
		int set_index;
		if(bk_set == 1){
			block_index = block_address % (set_ca / bk_set);
			set_index = block_index;		//Directed mapping, only one block per set
		}else{
			set_index = block_address % set_ca;
		}
		tag = block_address;

		if(address >= (8 * 1024 * 1024)){
			cout<<"Invalid address, address is out of range."<<endl;
		}
		//Print out the state of cache before reading from it.
		stringstream ss;
		int tag_pos;
		ss<<"# ";
		for(int i=0; i<set_ca; i++){
			ss<<"S"<<i<<"{";
			for(int j=0; j<bk_set; j++){
				tag_pos = j * (wd_bk + 2) + 1;
				if(cache[i][tag_pos-1].data == 1){				//Check valid bit
					if(cache[i][tag_pos-1].dirty_bit == 1){		//Check Dirty bit
						ss<<"B"<<cache[i][tag_pos].data<<"/d ";
					}else{
						ss<<"B"<<cache[i][tag_pos].data<<" ";
					}
				}else{
					ss<<"NULL ";
				}
			}
			ss<<"}, ";
		}
		ss<<"read-ack B"<<block_address;
		cout<<ss.str()<<endl;


		int cur_tag_pos = 0;
		for(int i=0; i<bk_set; i++){
			cur_tag_pos = i * (wd_bk + 2) + 1;				// && (cache[set_index][i*(wd_bk+2)].data == 1)
			if(cache[set_index][cur_tag_pos].data == tag){	//Check tag and valid bit
				tag_match = true;
				for(int j=0; j<wd_bk; j++){													//Find word to be read 
					if(cache[set_index][cur_tag_pos+1+j].byte_address == address){			//Specific word matched
						data_read = cache[set_index][cur_tag_pos+1+j].data;					//Then read data
						cache[set_index][cur_tag_pos-1].LRU_count = count_LRU;
						time = c_hit;
						cout<<"read-ack "<<hex<<data_read<<" "<<set_index<<" Hit! "<<time<<endl;
						break;
					}
				}
				break;
			}else if((cache[set_index][cur_tag_pos].data != tag) && (cache[set_index][i*(wd_bk+2)].data == 1)){	//tag miss
				tag_match = false;
				continue;
			}
		}
		if(tag_match == false){			//Tag miss in cache line, data needs to be written back from memory
			int count_offset = wd_bk+1+1;			//count value for each block
													//is stored at a index which is proportional to count_offset
			int write_index = 0;
			int min = cache[set_index][0].LRU_count;
			for(int k=0; k<bk_set; k++){		
				if(cache[set_index][k*count_offset].LRU_count < min){
					min = cache[set_index][k*count_offset].LRU_count;
					write_index = k;
				}
			}

			if(cache[set_index][write_index*count_offset].dirty_bit == 1){					//If the dirty bit is high, write the current 
				Flush(bt_wd, wd_bk, set_index, write_index*count_offset, cache, memory);	//data in that block back to memory firstly	 
				time = c_hit + c_mw + c_mr;
			}else{
				time = c_hit + c_mr;
			}
																							
			//Then read the required data from memory
			int data_from_mem = 0;
			for(int x=0; x<wd_bk; x++){
				for(int k=(bt_wd-1); k>=0; k--){
					data_from_mem = data_from_mem << 8;
					data_from_mem = data_from_mem | memory[address+k];
					
				}
				cache[set_index][write_index*count_offset+2+x].data = data_from_mem;				//Wirte data
				cache[set_index][write_index*count_offset+2+x].byte_address = address+(bt_wd*x);	//Save byte_address
			}
			data_read = cache[set_index][write_index*count_offset+2].data;
																			//Also
			cache[set_index][write_index*count_offset].data = 1;			//Set Valid bit
			cache[set_index][write_index*count_offset].dirty_bit = 0;		//Clear Dirty bit
			cache[set_index][write_index*count_offset+1].data = tag;		//Set Tag
			cache[set_index][write_index*count_offset].LRU_count = count_LRU;	//Set count
			tag_match = true;
			cout<<"read-ack "<<hex<<data_read<<" "<<set_index<<" Miss! "<<time<<endl;	
		}

}

void debug_req(vector< vector<cache_struct> >& cache, vector<int>& memory){
    cout<<"Dubug-ack begins!"<<endl;
    
    cout<<endl;
    cout<<"The Debug-req could not be done at the moment due to laziness of the development crew! :0 "<<endl;
    cout<<endl;
    
    cout<<"Debug-ack ends!"<<endl;
}




int main(int argc, char* argv[]){
	
	//Create cache by using 2D-array
	string c_mw_s = argv[--argc];
	int c_mw = stoi(c_mw_s.c_str());
	
	string c_mr_s = argv[--argc];
	int c_mr = stoi(c_mr_s.c_str());

	string c_hit_s = argv[--argc];
	int c_hit = stoi(c_hit_s.c_str());
	
	string set_ca_s = argv[--argc];
	int set_ca = stoi(set_ca_s.c_str());
	
	string bk_set_s = argv[--argc];
	int bk_set = stoi(bk_set_s.c_str());
	
	string wd_bk_s = argv[--argc];
	int wd_bk = stoi(wd_bk_s.c_str());

	string bt_wd_s = argv[--argc];
	int bt_wd = stoi(bt_wd_s.c_str());
	
	string add_bit_s = argv[--argc];
	int add_bit = stoi(add_bit_s.c_str());


	vector<int> memory = create_memory();
	vector< vector<cache_struct> > cache = create_cache(add_bit, bt_wd, wd_bk, bk_set, set_ca, c_hit, c_mr, c_mw);

	//Parsing the input
	string cmd;
	while(cin>>cmd){
		if(cmd == "#")
			continue;
		if(cmd == "write-req"){	//Write
			string addr, data;
			cin>>addr;
			cin>>data;
			int int_addr = stoi(addr, nullptr, 0);
			int int_data = stoi(data, nullptr, 16);
			write_request(int_addr, add_bit, bt_wd, wd_bk, bk_set, set_ca, c_hit, c_mr, c_mw,
			int_data, cache, memory);  
		}
		else if(cmd == "read-req"){	//read
			string addr;
			cin>>addr;
			int int_addr = stoi(addr, nullptr, 0);
			read_request(int_addr, add_bit, bt_wd, wd_bk, bk_set, set_ca, c_hit, c_mr, c_mw, cache, memory);		
		}
		else if(cmd == "flush-req"){	//flush		
			flush_request(bt_wd, wd_bk, bk_set, set_ca, cache, memory);
		}
        else if (cmd == "debug-req"){	//debug
            debug_req(cache, memory);
        }
	}


	return 0;
}