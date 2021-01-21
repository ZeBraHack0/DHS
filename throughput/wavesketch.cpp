#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdint.h>
#include <random>
#include <string>
#include <memory>
#include <iostream>
#include <cmath>
#include <math.h>
#include<time.h>
#include<unordered_map>
#include<algorithm>
#include<fstream>
#include <bitset>

using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 10

#define landa_d 16
#define b 1.08 
#define test_cycles 1
#define k 1000
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define BUCKET_NUM (HEAVY_MEM / 134)
#define TOT_MEM_IN_BYTES (600 * 1024)
#define INT_MAX ((int)(~0U>>1))


struct FIVE_TUPLE { char key[13]; };
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static const int COUNT[2] = {1, -1};
static const int factor = 1;
static int HEAVY_MEM = 50 * 1024;

inline unsigned int BKDRHash(char* str)
{
	unsigned int seed = 131;
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

inline unsigned int finger_print(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash & 0x7FFFFFFF;
}

class hg_node
{
public:
	vector<unsigned int>items; //32*16
	vector<int>counters; //32*16
	bitset<landa_d>real; //16
	int incast; //32
	
	hg_node()
	{
		items = vector<unsigned int>(landa_d, 0);
		counters = vector<int>(landa_d, 0);
		incast = 0;
		real.reset();
	}
	void insert(unsigned int item, int hash)
	{
		unsigned int choice = item & 1;
        int min_num = INT_MAX;
        unsigned int min_pos = -1;
        
        for(unsigned int i = 0; i < landa_d; ++i){
            if(counters[i] == 0){
                items[i] = item;
                counters[i] = 1;
                real[i] = 1;
                return;
            }
            else if(items[i] == item){
                if(real[i])            
                    counters[i]++;
                else{
                    counters[i]++;
                    incast += COUNT[choice];
                }
                return;
            }

            if(counters[i] < min_num){
                min_num = counters[i];
                min_pos = i;
            }
        }

        /*
        if(incast * COUNT[choice] >= min_num){
            //count_type pre_incast = incast;
            if(real[min_pos]){
                uint32_t min_choice = hash_(items[min_pos], 17) & 1;
                incast += COUNT[min_choice] * counters[min_pos];
            }
            items[min_pos] = item;
            counters[min_pos] += 1;
            real[min_pos] = 0;
        }
        incast += COUNT[choice];
        */

        if(incast * COUNT[choice] >= int(min_num * factor)){
            //count_type pre_incast = incast;
            if(real[min_pos]){
                unsigned int min_choice = items[min_pos] & 1;
                incast += COUNT[min_choice] * counters[min_pos];
            }
            items[min_pos] = item;
            counters[min_pos] += 1;
            real[min_pos] = 0;
        }
        incast += COUNT[choice];
	}
	int query(unsigned int item, int hash)
	{
		unsigned int choice = item & 1;

            for(unsigned int i = 0; i < landa_d; ++i){
                if(items[i] == item){
                    return counters[i];
                }
            }

            return 0;//incast * COUNT[choice];
	}
};

void ReadInTraces(const char* trace_prefix)
{
	for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName, "%s%d.dat", trace_prefix, datafileCnt - 1);
		FILE* fin = fopen(datafileName, "rb");

		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();
		while (fread(&tmp_five_tuple, 1, 13, fin) == 13)
		{
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
		}
		fclose(fin);

		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}

bool cmp1(pair<int,int>p1, pair<int,int>p2)
{
	return p1.first > p2.first;
}

bool cmp2(pair<int,int>p1, pair<int,int>p2)
{
	return p1.second > p2.second;
}

int main()
{
	ReadInTraces("data/");
	ofstream out("./ws.txt");
	for(int i = 0; i<12; i++)
	{
		//out<<"memory: "<<HEAVY_MEM<<endl;
		double gb_throughput = 0;
	 	
		srand((unsigned)time(NULL));
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
		{
			timespec time1, time2;
			long long resns;
			int packet_cnt = (int)traces[datafileCnt - 1].size();
			//printf("packet count:%d\n", packet_cnt);
	
			char** keys = new char * [(int)traces[datafileCnt - 1].size()];
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
			{
				keys[i] = new char[13];
				memcpy(keys[i], traces[datafileCnt - 1][i].key, 13);
			}
			double th; 
			clock_gettime(CLOCK_MONOTONIC, &time1);
			for (int t = 0; t < test_cycles; ++t)
			{
				vector<hg_node>hg(BUCKET_NUM);
				for (int i = 0; i < packet_cnt; ++i)
				{
					int hash = BKDRHash(keys[i]);
					hg[hash % BUCKET_NUM].insert(finger_print(hash), hash);
				}
			}
			clock_gettime(CLOCK_MONOTONIC, &time2);
			resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
			th = (double)1000.0 * test_cycles * packet_cnt / resns;
			/* free memory */
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
				delete[] keys[i];
			delete[] keys;
			//printf("throughput is %lf mbps\n", th);
			//out<<"throughput:"<<th<<"\n"<<endl;
			gb_throughput += th;
		}
		out<<gb_throughput/10<<endl;
		HEAVY_MEM += 50*1024;
	}
}
