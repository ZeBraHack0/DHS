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

using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 10

#define landa_h 8
#define landa_l 64
#define b 1.08 
#define test_cycles 1
#define k 1000
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define BUCKET_NUM (HEAVY_MEM / 64)
#define TOT_MEM_IN_BYTES (600 * 1024)


struct FIVE_TUPLE { char key[13]; };
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int HEAVY_MEM = 150 * 1024;


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

inline unsigned short finger_print(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash & 65535;
}

class hg_node
{
public:
	vector<pair<unsigned short, unsigned short>>heavy;
	vector<unsigned char>light;
	hg_node()
	{
		heavy = vector<pair<unsigned short, unsigned short>>(landa_h, make_pair(0, 0));
		light = vector<unsigned char>(landa_l / 2, 0);
	}
	void insert(unsigned short fp, int hash)
	{
		//printf("fingerprint:%d, hash value: %d\n", fp, hash);
		int heavy_empty = -1;
		int wg = -1;
		int wgf = -1;
		for (int i = 0; i < landa_h; i++)
		{
			if (heavy[i].first == fp)
			{
				heavy[i].second += 1;
				return;
			}
			if (heavy[i].second == 0 && heavy_empty == -1)
			{
				heavy_empty = i;
			}
			else if (heavy[i].second)
			{
				if (wg == -1 || heavy[i].second > wgf)
				{
					wgf = heavy[i].second;
					wg = i;
				}
			}
		}
		if (heavy_empty >= 0)
		{
			heavy[heavy_empty].second = 1;
			heavy[heavy_empty].first = fp;
		}
		else
		{
			//exponential decay
			double ran = 1.0 * rand() / RAND_MAX;
			//printf("random number:%f\n", ran);
			bool replace = 0;
			if (ran < pow(b, wgf * -1))
			{
				heavy[wg].second -= 1;
				if (heavy[wg].second == 0)
				{
					heavy[wg].first = fp;
					heavy[wg].second = 1;
					replace = 1;
				}
			}
			if(replace != 1) //insert into the light part
			{
				int index = hash % (landa_l);
				if (index % 2)
				{
					int bits = light[index / 2] & 15;
					if (bits < 15)light[index / 2] = light[index / 2] + 1;
				}
				else
				{
					int bits = (light[index / 2] & 240);
					if (bits < 240)light[index / 2] = light[index / 2] + 16;
				}
			}
		}
	}
	int query(unsigned short fp, int hash)
	{
		for (int i = 0; i < landa_h; i++)
		{
			if (heavy[i].first == fp)
			{
				return heavy[i].second;
			}
		}
		int index = hash % (landa_l);
		if (index % 2)
		{
			int bits = light[index / 2] & 15;
			return bits;
		}
		else
		{
			int bits = (light[index / 2] & 240);
			return bits >> 4;
		}
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
	ofstream out("./hg.txt");

	for(int i = 0; i<10; i++)
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
