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
#define d 1


struct FIVE_TUPLE { char key[13]; };
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int TOTAL_MEM = 50 * 1024;
static int HEAVY_MEM = TOTAL_MEM *0.5;


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
	vector<unsigned int> fp;
	vector<unsigned short> pvote;
	vector<unsigned short> nvote; 
	vector<bool> flag;
	hg_node()
	{
		fp = vector<unsigned int>(7, 0);
		pvote = vector<unsigned short>(7, 0);
		nvote = vector<unsigned short>(7, 0);
		flag = vector<bool>(7, 0);
	}
	int insert(unsigned int f, unsigned int& swap_key, unsigned int& swap_value)
	{
		int min_size = INT_MAX;
		int min_pos = -1;
		for(int i = 0; i<7; i++)
		{
			if(f==fp[i])
			{
				pvote[i] += 1;
				return 1;
			}
			else if(pvote[i] == 0)
			{
				pvote[i] = 1;
				fp[i] = f;
				return 1;
			}
			if(min_size > pvote[i])
			{
				min_pos = i;
				min_size = pvote[i];
			}
		}
		
		nvote[min_pos] += 1;
		if(nvote[min_pos]/pvote[min_pos] >= lambda)
		{
			swap_key = fp[min_pos];
			swap_value = pvote[min_pos];
			fp[min_pos] = f;
			flag[min_pos] = 1;
			nvote[min_pos] = 1;
			pvote[min_pos] = 1;
			return 2;
		}
		else return 0;
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
		light = vector<vector<unsigned short>>(d, vector<unsigned short>((TOTAL_MEM-HEAVY_MEM)/d, 0));
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
					htok[hash] = i;
					gb_cnt[hash] += 1;
					lc_cnt[hash] += 1;
					if(hit_cnt.count(hash) == 0)hit_cnt[hash] = hg.query(keys[i]);
					hg.insert(keys[i]);
					if(i && i % window == 0)
					{
						//heavy hitter
						if(1)
						{
							double th = window * hh;
							int tp = 0, fp =0, tn = 0, fn = 0;
							for(auto it:gb_cnt)
							{
								bool f1 = 0, f2 = 0;
								if(it.second >= th)f1 = 1;
								int hash = it.first;
								int idx = htok[hash];
								int efreq = hg.query(keys[idx]);
								if(efreq >= th)f2 = 1;
								if(f1 && f2)tp++;
								else if(f1 && !f2)fn++;
								else if(!f1 && f2)fp++;
								else if(!f1 && !f2)tn++;
							}
							double recall = (double)tp/(tp+fn);
							double precision = (double)tp/(tp+fp);
							double fscore = 2*(recall*precision)/(recall + precision);
							//cout<<recall<<" "<<precision<<endl;
							//out<<i/window <<"th heavy hitter FSOCRE:"<<fscore<<endl;
							gb_heavy_hitter[i/window-1] += fscore;
							hh_cnt[i/window-1] += 1;
						}
						//heavy changer
						if(1)
						{
							double th = window * hc;
							int tp = 0, fp =0, tn = 0, fn = 0;
							for(auto it:lc_cnt)
							{
								bool f1 = 0, f2 = 0;
								if(it.second >= th)f1 = 1;
								int hash = it.first;
								int idx = htok[hash];
								int efreq = hg.query(keys[idx]) - hit_cnt[hash];
								ph_cnt[hash] = efreq;
								if(efreq >= th)f2 = 1;
								if(f1 && f2)tp++;
								else if(f1 && !f2)fn++;
								else if(!f1 && f2)fp++;
								else if(!f1 && !f2)tn++;
							}
							double recall = (double)tp/(tp+fn);
							double precision = (double)tp/(tp+fp);
							double fscore = 2*(recall*precision)/(recall + precision);
							//cout<<recall<<" "<<precision<<endl;
							//out<<i/window <<"th heavy hitter FSOCRE:"<<fscore<<endl;
							gb_heavy_changer[i/window-1] += fscore;
							hc_cnt[i/window-1] += 1;
						}
						
						lc_cnt = unordered_map<int,int>();
						hit_cnt = unordered_map<int,int>();
					}
				}
				clock_gettime(CLOCK_MONOTONIC, &time2);
				resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
				th = (double)1000.0 * test_cycles * packet_cnt / resns;
				//ARE
				vector<pair<int,int>>topk;
				for(auto it:gb_cnt)
				{
					topk.push_back(make_pair(it.first, it.second));
				}
				sort(topk.begin(), topk.end(), cmp2);
				double ARE = 0;
				for(int i =0; i<k; i++)
				{
					int hash = topk[i].first;
					int idx = htok[hash];
					int efq = hg.query(keys[idx]);
					//printf("real freq:%d, estimated freq:%d\n", topk[i].second, efq);
					ARE += (double)abs(topk[i].second - efq)/topk[i].second;
				}
				ARE /= k;
				//out<<"ARE:"<<ARE<<endl;
				cout<<"ARE:"<<ARE<<endl;
				gb_ARE += ARE;
				//WMRE
				int max_freq = 0;
				for(auto it:gb_cnt)
				{
					freq[it.second] += 1;
					int hash = it.first;
					int idx = htok[hash];
					int efreq = hg.query(keys[idx]);
					freq_e[efreq] += 1;
					max_freq = max(max_freq, it.second);
					max_freq = max(max_freq, efreq);
				}
				double wmre = 0, wmd = 0;
				for(int i = 1; i<=max_freq; i++)
				{
					wmre += (double)abs(freq[i] - freq_e[i]);
					wmd += ((double)freq[i] + freq_e[i])/2;
				}
				//out<<"WMRE:"<<wmre/wmd<<endl;
				cout<<"WMRE:"<<wmre/wmd<<endl;
				gb_WMRE += wmre/wmd;
				//entropy
				int flow_num = gb_cnt.size();
				cout<<"flow_num:"<<flow_num<<endl;
				double e = 0, ee = 0;
				for(int i = 1; i<=max_freq; i++)
				{
					if(freq[i])cout<<i<<" "<<freq[i]<<" "<<freq_e[i]<<endl; 
					e += freq[i]?-1*((double)i*freq[i]/flow_num)*log2((double)freq[i]/flow_num):0;
					ee += freq_e[i]?-1*((double)i*freq_e[i]/flow_num)*log2((double)freq_e[i]/flow_num):0;
				}
				//out<<"entropy ARE:"<<fabs(e-ee)/e<<endl;
				cout<<"entropy ARE:"<<fabs(e-ee)/e<<endl;
				//out<<"real entropy:"<<e<<endl; 
				gb_entropy += fabs(e-ee)/e;
				//FSCORE
				unordered_map<int,bool>ef;
				double th = packet_cnt * hh;
				int tp = 0, fp =0, tn = 0, fn = 0;
				for(auto it:gb_cnt)
				{
					bool f1 = 0, f2 = 0;
					if(it.second >= th)f1 = 1;
					int hash = it.first;
					int idx = htok[hash];
					int efreq = hg.query(keys[idx]);
					if(efreq >= th)f2 = 1;
					if(f1 && f2)tp++;
					else if(f1 && !f2)fn++;
					else if(!f1 && f2)fp++;
					else if(!f1 && !f2)tn++;
				}
				double recall = (double)tp/(tp+fn);
				double precision = (double)tp/(tp+fp);
				double fscore = 2*(recall*precision)/(recall + precision);
				//cout<<recall<<" "<<precision<<endl;
				//out<<"Total FSOCRE:"<<fscore<<endl;
				cout<<"Total FSOCRE:"<<fscore<<endl;
				gb_fscore += fscore;
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
		out<<"heavy changer FSOCRE:"<<endl;
		for(int i = 0; i<10; i++)
		{
			if(hc_cnt[i] > 0)out<<gb_heavy_changer[i]/hc_cnt[i]<<endl;
		}
		out<<"heavy hitter FSOCRE:"<<endl;
		for(int i = 0; i<10; i++)
		{
			if(hh_cnt[i] > 0)out<<gb_heavy_hitter[i]/hh_cnt[i]<<endl;
		}
		out<<"ARE:"<<gb_ARE/10<<endl;
		out<<"WMRE:"<<gb_WMRE/10<<endl;
		out<<"entropy ARE:"<<gb_entropy/10<<endl;
		out<<"Total FSOCRE:"<<gb_fscore/10<<endl;
		out<<"throughput:"<<gb_throughput/10<<endl;
		out<<endl;
		TOTAL_MEM += 50*1024;
		HEAVY_MEM = TOTAL_MEM*0.9;
	}
}

