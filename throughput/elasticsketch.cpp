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
#include "BOBHash32.h"

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
#define BUCKET_NUM (HEAVY_MEM / 6)
#define TOT_MEM_IN_BYTES (600 * 1024)
#define lambda 8
#define d 4


struct FIVE_TUPLE { char key[13]; };
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int TOTAL_MEM = 50 * 1024;
static int HEAVY_MEM = TOTAL_MEM *0.9;


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
	unsigned int fp;
	unsigned short pvote;
	unsigned short nvote; 
	bool flag;
	hg_node()
	{
		fp = 0;
		pvote = 0;
		nvote = 0;
		flag = 0;
	}
	int insert(unsigned int f, unsigned int& swap_key, unsigned int& swap_value)
	{
		if(pvote == 0)
		{
			pvote = 1;
			fp = f;
			return 1;
		}
		else if(f==fp)
		{
			pvote += 1;
			return 1;
		}
		else
		{
			nvote += 1;
			if(nvote/pvote >= lambda)
			{
				swap_key = fp;
				swap_value = pvote;
				fp = f;
				flag = 1;
				nvote = 1;
				pvote = 1;
				return 2;
			}
			else return 0;
		}
	}
};

class Hg
{
public:
	vector<hg_node> heavy;
	vector<vector<unsigned short>> light;
	BOBHash32* hash[d] = {NULL};
	Hg()
	{	
		heavy = vector<hg_node>(BUCKET_NUM);
		light = vector<vector<unsigned short>>(d, vector<unsigned short>((TOTAL_MEM-HEAVY_MEM)/d/2, 0));
		for(int i = 0; i < d; ++i){
			hash[i] = new BOBHash32(i + 750);
		}
	}
	~Hg()
	{
		for (int i = 0; i < d; ++i)
			delete hash[i];
	}
	void insert(char* key)
	{
		unsigned int h1 = BKDRHash(key);
		unsigned int swap_key, swap_value;
		int res = heavy[h1%BUCKET_NUM].insert(h1, swap_key, swap_value);
		//cout<<res<<endl;
		if(res==0)
		{
			for (int i = 0; i < d; i++) {
			const char * tmp = to_string(h1).data();
            int index = (hash[i]->run(tmp,4)) % light[i].size();
            //cout<<index<<endl;
            light[i][index] += 1;
        	}
		}
		else if(res == 2)
		{
			for (int i = 0; i < d; i++) {
            const char * tmp = to_string(swap_key).data();
            int index = (hash[i]->run(tmp,4)) % light[i].size();
            light[i][index] += swap_value;
        	}
		}
	}
	unsigned short query(char* key)
	{
		unsigned int h1 = BKDRHash(key);
		if(heavy[h1%BUCKET_NUM].fp != h1)
		{
			int ret = 1 << 30;
	        for (int i = 0; i < d; i++) {
				const char * tmpkey = to_string(h1).data();
	            int index = (hash[i]->run(tmpkey,4)) % light[i].size();
	            int tmp = light[i][index];
	            ret = min(ret, tmp);
	        }
	        return ret;
		}
		else if(heavy[h1%BUCKET_NUM].flag)
		{
			int ret = 1 << 30;
	        for (int i = 0; i < d; i++) {
				const char * tmpkey = to_string(h1).data();
	            int index = (hash[i]->run(tmpkey,4)) % light[i].size();
	            int tmp = light[i][index];
	            ret = min(ret, tmp);
	        }
	        return heavy[h1%BUCKET_NUM].pvote;
		}
		else
		{
			return heavy[h1%BUCKET_NUM].pvote;
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
	ReadInTraces("../data/");
	ofstream out("./elastic.txt");
	
	for(int i = 0; i<12; i++)
	{
		out<<"memory: "<<HEAVY_MEM<<endl;
		cout<<"memory: "<<HEAVY_MEM<<endl;
		vector<double> gb_heavy_changer(10, 0);
		vector<int>hc_cnt(10,0);
		vector<double> gb_heavy_hitter(10, 0);
		vector<int>hh_cnt(10,0);
		double gb_ARE = 0;
		double gb_WMRE = 0;
		double gb_entropy = 0;
		double gb_fscore = 0;
		double gb_throughput = 0;
	 	
		srand((unsigned)time(NULL));
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
		{
			unordered_map<int,int>gb_cnt;
			unordered_map<int,int>lc_cnt;
			unordered_map<int,int>pre_cnt;
			unordered_map<int,int>hit_cnt;
			unordered_map<int,int>ph_cnt;
			unordered_map<int,int>freq;
			unordered_map<int,int>freq_e;
			unordered_map<int, int>htok;
			timespec time1, time2;
			long long resns;
			int packet_cnt = (int)traces[datafileCnt - 1].size();
			int window = packet_cnt / epoch;
			//printf("packet count:%d\n", packet_cnt);
	
			char** keys = new char * [(int)traces[datafileCnt - 1].size()];
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
			{
				keys[i] = new char[13];
				memcpy(keys[i], traces[datafileCnt - 1][i].key, 13);
			}
	
			clock_gettime(CLOCK_MONOTONIC, &time1);
			double th; 
			for (int t = 0; t < test_cycles; ++t)
			{
				Hg hg = Hg();
				for (int i = 0; i < packet_cnt; ++i)
				{
					int hash = BKDRHash(keys[i]);
					hg.insert(keys[i]);
					
				}
				clock_gettime(CLOCK_MONOTONIC, &time2);
				resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
				th = (double)1000.0 * test_cycles * packet_cnt / resns;
			}
			//printf("throughput is %lf mbps\n", th);
			cout<<"throughput:"<<th<<"\n"<<endl;
			gb_throughput += th;
			/* free memory */
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
				delete[] keys[i];
			delete[] keys;
			cout<<"finish free\n"<<endl;
		}
		out<<"throughput:"<<gb_throughput/10<<endl;
		out<<endl;
		TOTAL_MEM += 50*1024;
		HEAVY_MEM = TOTAL_MEM*0.9;
	}
}

