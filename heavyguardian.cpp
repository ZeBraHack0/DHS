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
static int HEAVY_MEM = 5 * 1024;


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
	ReadInTraces("../../../data/");
	ofstream out("./hg.txt");

	for(int i = 0; i<10; i++)
	{
		out<<"memory: "<<HEAVY_MEM<<endl;
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
				vector<hg_node>hg(BUCKET_NUM);
				for (int i = 0; i < packet_cnt; ++i)
				{
					int hash = BKDRHash(keys[i]);
					gb_cnt[hash] += 1;
					lc_cnt[hash] += 1;
					if(hit_cnt.count(hash) == 0)hit_cnt[hash] = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
					hg[hash % BUCKET_NUM].insert(finger_print(hash), hash);
					//if (i % 100000 == 0)printf("flow frequency:%d\n", hg[hash % BUCKET_NUM].query(finger_print(hash), hash));
					//hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
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
								int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
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
								int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash) - hit_cnt[hash];
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
					int efq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
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
					int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
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
					int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
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
			
			/* free memory */
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
				delete[] keys[i];
			delete[] keys;
			printf("throughput is %lf mbps\n", th);
			//out<<"throughput:"<<th<<"\n"<<endl;
			gb_throughput += th;
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
		HEAVY_MEM += 5*1024;
	}
}

