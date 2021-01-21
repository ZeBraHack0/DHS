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

#define landa_h 16
#define b 1.08
#define test_cycles 1
#define k 1000
#define c1 1 
#define c2 1
#define c3 1
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define BUCKET_NUM (HEAVY_MEM / landa_h)


struct FIVE_TUPLE { char key[13]; };
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int HEAVY_MEM = 50 * 1024;
unsigned int trace_hash = 87210383;
ofstream trace("./trace.txt");

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
	vector<unsigned char>heavy;
	unsigned int usage;//num-/used-
	hg_node()
	{
		heavy = vector<unsigned char>(landa_h, 0);
		usage = 0;
		//here we withdraw the num2 because it can be inferred
		//usage += 15;
		//all level 2 initially
		//usage += (2<<8);
		//usage += (1<<16);
	}
	void levelup(int level, int f, int idx)
	{
		double ran = 1.0 * rand() / RAND_MAX;
		switch(level)
		{
			case 1:
				{
					//if(ran > c1)return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = landa_h/2 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int start = 0;
					int end = start + num2*2;
					//if(f&255==0)trace<<"state:"<<usage2<<" "<<num2<<endl;
					if(usage2 < num2)//exist empty space
					{
						for(int i = start;i<end;i+=2)
						{
							//if(i >= landa_h)printf("error warning!\n");
							if(heavy[i] == 0 && heavy[i+1] == 0)
							{
								heavy[i] = f&255;
								heavy[i+1] = 1;
								usage += 1;
								return;
							}
						}
					}
					else //no empty space
					{
						//find weakest guardian
						if(num2 == 0)return;
						int min_f = -1;
						int min_fq = -1;
						for(int i = start;i<end;i+=2)
						{
							//if(i >= landa_h)printf("error warning!\n");
							if(min_f == -1)
							{
								min_f = i;
								min_fq = heavy[i+1];
							}
							else if(heavy[i+1] < min_fq)
							{
								min_f = i;
								min_fq = heavy[i+1];
							}
						}
						//exponential decay
						//trace<<"exponential decay"<<endl;
						//if(min_f==-1 || min_fq < 0)printf("minus 1 warning!\n");
						//if(finger_print(trace_hash)&255 == heavy[min_f])trace<<min_fq<<endl;
						if (ran < pow(b, log2(min_fq) * -1))
						{
							heavy[min_f+1] -= 1; 
							if(heavy[min_f+1] <= 0)
							{
								heavy[min_f] = (f&255);
								heavy[min_f+1] = 1;
							}
						}
					}
					break;
				}
			case 2:
				{
					if(ran > c2)return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = landa_h/2 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int usage3 = (usage>>12) & 15;
					//cout<<"level 3"<<endl;
					if(num3 == usage3 && num2 > 3)
					{
						int usage4 = (usage>>20) & 15;
						num3 += 2;
						num2 -= 3;
						int rest = 0;
						if(usage2 > num2)
						{
							rest = usage2 - num2;
							usage2 = num2;
						}
						//rest == 0: nothing happen
						//rest > 0: kill minimum rest entreis
						if(rest)
						{
							vector<int> weaks(rest, -1);
							vector<int> widx(rest, -1);
							for(int i = 0; i< (num2+3)*2; i+=2)
							{
								//if(i >= landa_h)printf("error warning!\n");
								for(int j = 0; j<rest; j++)
								{
									if(widx[j] == -1)
									{
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
									else if(heavy[i+1] < weaks[j] && heavy[i+1] > 0)
									{
										for(int l = rest-1; l>j; l--)
										{
											widx[l] = widx[l-1];
											weaks[l] = weaks[l-1];
										}
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
								}
							}
							int kill = 0;
							sort(widx.begin(), widx.end());
							for(int i = widx[kill]; i< (num2+3)*2; i+=2)
							{
								//if(i >= landa_h)printf("error warning!\n");
								if(i == widx[kill])
								{
									kill++;
									trace<<"killed: "<<(int)heavy[i+1]<<endl;
									heavy[i] = 0;
									heavy[i+1] = 0;
									continue;
								}
								else if(kill)
								{
									heavy[i-kill*2] = heavy[i];
									heavy[i+1-kill*2] = heavy[i+1];
									heavy[i] = 0;
									heavy[i+1] = 0;
								}
							}
						}
						
						usage = 0;
						usage += usage2;
						usage += (num3<<8);
						usage += (usage3<<12);
						usage += (num4<<16);
						usage += (usage4<<20);
					}
					
					
					int start = num2*2;
					int end = start + num3*3;
					if(usage3 < num3)//exist empty space
					{
						for(int i = start;i<end;i+=3)
						{
							//if(i >= landa_h)printf("error warning!\n");
							if(heavy[i] == 0 && heavy[i+1] == 0 && heavy[i+2] == 0)
							{
								f &= 4095; 
								heavy[i] = (f>>4);
								heavy[i+1] = ((f&15)<<4) + 1;
								heavy[i+2] = 0;

								usage += (1<<12);
								return;
							}
						}
						//cout<<"error warning 3!"<<endl;
					}
					else //no empty space
					{
						//find weakest guardian
						int min_f = -1;
						int min_fq = -1;
						for(int i = start;i<end;i+=2)
						{
							//if(i >= landa_h)printf("error warning!\n");
							int freq = ((int)(heavy[i+1]&15)<<8)+heavy[i+2];
							if(min_f == -1)
							{
								min_f = i;
								min_fq = freq;
							}
							else if(freq < min_fq && freq>0)
							{
								min_f = i;
								min_fq = freq;
							}
						}
						//exponential decay
						//if(min_f==-1 || min_fq < 0)printf("minus 2 warning!\n");
						if (ran < pow(b, log2(min_fq) * -1))
						{
							min_fq -= 1; 
							if(min_fq <= 255)
							{
								f &= 4095; 
								heavy[min_f] = (f>>4);
								heavy[min_f+1] = ((f&15)<<4) + 1;
								heavy[min_f+2] = 0;

							}
							else
							{
								heavy[min_f+1] = (heavy[min_f+1]&240)+(min_fq>>8);
								heavy[min_f+2] = (min_fq&255); 
							}
						}
					}
					break;
				}
			case 3:
				{
					if(ran > c3)return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = landa_h/2 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int usage3 = (usage>>12) & 15;
					int usage4 = (usage>>20) & 15;
					//cout<<"level 4"<<endl;
					
					if(num4 == usage4)
					{
						if(num2 > 2)
						{
							heavy[idx] = 0;
							heavy[idx+1] = 0;
							heavy[idx+2] = 0;
							usage3 -= 1;
							num4 += 1;
							num2 -= 2;
							int rest = 0;
							if(usage2 > num2)
							{
								rest = usage2 - num2;
								usage2 = num2;
							}
							//rest == 0: nothing happen
	
							//rest > 0: kill minimum rest entreis
							if(rest)
							{
								vector<int> weaks(rest, -1);
								vector<int> widx(rest, -1);
								for(int i = 0; i< (num2+2)*2; i+=2)
								{
									//if(i >= landa_h)printf("error warning!\n");
									for(int j = 0; j<rest; j++)
									{
										if(widx[j] == -1)
										{
											widx[j] = i;
											weaks[j] = heavy[i+1];
											break;
										}
										else if(heavy[i+1] < weaks[j] && heavy[i+1] > 0)
										{
											for(int l = rest-1; l>j; l--)
											{
												widx[l] = widx[l-1];
												weaks[l] = weaks[l-1];
											}
											widx[j] = i;
											weaks[j] = heavy[i+1];
											break;
										}
									}
								}
								int kill = 0;
								sort(widx.begin(), widx.end());
								for(int i = widx[kill]; i< (num2+3)*2; i+=2)
								{
									//if(i >= landa_h)printf("error warning!\n");
									if(i == widx[kill])
									{
										kill++;
										//trace<<"killed: "<<(int)heavy[i+1]<<endl;
										heavy[i] = 0;
										heavy[i+1] = 0;
										continue;
									}
									else if(kill)
									{
										heavy[i-kill*2] = heavy[i];
										heavy[i+1-kill*2] = heavy[i+1];
										heavy[i] = 0;
										heavy[i+1] = 0;
									}
								}
							}
							for(int i = (num2+2)*2; i<(num2+2)*2+num3*3; i+=3)
							{
								//if(i >= landa_h)printf("error warning!\n");
								heavy[i-4] = heavy[i];
								heavy[i+1-4] = heavy[i+1];
								heavy[i+2-4] = heavy[i+2];
								heavy[i] = 0;
								heavy[i+1] = 0;
								heavy[i+2] = 0;
							}
							
							usage = 0;
							usage += usage2;
							usage += (num3<<8);
							usage += (usage3<<12);
							usage += (num4<<16);
							usage += (usage4<<20);
						} 
						else if(num3 > 4)
						{
							heavy[idx] = 0;
							heavy[idx+1] = 0;
							heavy[idx+2] = 0;
							usage3 -= 1;
							num4 += 3;
							num3 -= 4;
							int rest = 0;
							if(usage3 > num3)
							{
								rest = usage3 - num3;
								usage3 = num3;
							}	
							//rest == 0: nothing happen
							rest = 4; //different distribution of level-3 empty space
							//rest > 0: kill minimum rest entreis
							if(rest)
							{
								vector<int> weaks(rest, -1);
								vector<int> widx(rest, -1);
								for(int i = num2*2; i< num2*2 + (num3+4)*3; i+=3)
								{
									if(i >= landa_h)printf("error warning!\n");
									int a = ((int)(heavy[i+1]&15)<<8)+heavy[i+2];
									for(int j = 0; j<rest; j++)
									{
										if(widx[j] == -1)
										{
											widx[j] = i;
											weaks[j] = a;
											break;
										}
										else if(a < weaks[j])
										{
											for(int l = rest-1; l>j; l--)
											{
												widx[l] = widx[l-1];
												weaks[l] = weaks[l-1];
											}
											widx[j] = i;
											weaks[j] = a;
											break;
										}
									}
								}
								int kill = 0;
								sort(widx.begin(), widx.end());
								for(int i = widx[kill]; i< num2*2 + (num3+4)*3; i+=3)
								{
									//if(i >= landa_h)printf("error warning!\n");
									if(i == widx[kill])
									{
										kill++;
										heavy[i] = 0;
										heavy[i+1] = 0;
										heavy[i+2] = 0;
										continue;
									}
									else if(kill)
									{
										heavy[i-kill*3] = heavy[i];
										heavy[i+1-kill*3] = heavy[i+1];
										heavy[i+2-kill*3] = heavy[i+2];
										heavy[i] = 0;
										heavy[i+1] = 0;
										heavy[i+2] = 0;
									}
								}
							}
							
							usage = 0;
							usage += usage2;
							usage += (num3<<8);
							usage += (usage3<<12);
							usage += (num4<<16);
							usage += (usage4<<20);
						}
					}
					else if(num4 > usage4)
					{
						heavy[idx] = 0;
						heavy[idx+1] = 0;
						heavy[idx+2] = 0;
						usage3 -= 1;
					}
					
					
					int start = num2*2 + num3*3;
					int end = start + num4*4;
					if(usage4 < num4)//exist empty space
					{
						for(int i = start;i<end;i+=4)
						{
							//if(i >= landa_h)printf("error warning!\n");
							if(heavy[i] == 0 && heavy[i+1] == 0 && heavy[i+2] == 0 && heavy[i+3] == 0)
							{
								heavy[i] = (f>>8);
								heavy[i+1] = f&255;
								heavy[i+2] = 16;
								heavy[i+3] = 0; 
								usage += (1<<20);
								return;
							}
						}
						//cout<<"error warning 4!"<<endl;
						//cout<<"status"<<usage2<<" "<<num2<<" "<<usage3<<" "<<num3<<" "<<usage4<<" "<<num4<<endl;
					}
					else //no empty space
					{
						if(num4 == 0)return;
						//find weakest guardian
						int min_f = -1;
						int min_fq = -1;
						for(int i = start;i<end;i+=2)
						{
							//if(i >= landa_h)printf("error warning!\n");
							int freq = ((int)heavy[i+2]<<8)+heavy[i+3];
							if(min_f == -1)
							{
								min_f = i;
								min_fq = freq;
							}
							else if(freq < min_fq && freq>0)
							{
								min_f = i;
								min_fq = freq;
							}
						}
						//exponential decay
						//if(min_f==-1 || min_fq <0)printf("minus 3 warning!\n");
						if (ran < pow(b, log2(min_fq) * -1))
						{
							min_fq -= 1; 
							//cout<<"level 4 decay result: "<<min_fq<<endl;
							if(min_fq <= 4095)
							{
								heavy[min_f] = (f>>8);
								heavy[min_f+1] = f&255;
								heavy[min_f+2] = 16;
								heavy[min_f+3] = 0;
							}
							else
							{
								heavy[min_f+2] = (min_fq>>8);
								heavy[min_f+3] = (min_fq&255); 
								//cout<<"check level 4: "<<heavy[min_f+2]<<endl;
							}
						}
					}
					break;
				}
			default:break;
		}
		return; 
	}
	void insert(unsigned short f)
	{
		//if exist a flow
		int num3 = (usage>>8) & 15;
		int num4 = (usage>>16) & 15;
		int num2 = landa_h/2 - num4*2 - num3*3/2;
		int usage2 = usage & 255;
		//if(hash == trace_hash)trace<<"state:"<<usage2<<endl;
			//level 4
		int start = num2*2+num3*3;
		int end = start + num4*4;
		for(int i = start;i<end;i+=4)
		{
			//if(i >= landa_h)printf("error warning!\n");
			unsigned short e = ((unsigned short)heavy[i]<<8)+heavy[i+1];
			if(e==f && (heavy[i+2] +heavy[i+3] > 0))
			{
				if(heavy[i+3]<255)heavy[i+3]++;
				else if(heavy[i+2]!=255)
				{
					heavy[i+2]++;
					heavy[i+3] = 0;
				}
				else
				{
					levelup(4, f, i);
				}
				return;
			}
		}
			//level 3
		start = num2*2;
		end = start + num3*3;
		for(int i = start;i<end;i+=3)
		{
			//if(i >= landa_h)printf("error warning!\n");
			unsigned short e = ((unsigned short)heavy[i]<<4)+(heavy[i+1]>>4);
			if(e==(f&4095) && ((heavy[i+1] & 15) + heavy[i+2] > 0))
			{
				if(heavy[i+2]<255)heavy[i+2]++;
				else if((heavy[i+1] & 15)!= 15)
				{
					heavy[i+1]++;
					heavy[i+2] = 0;
				}
				else
				{
					levelup(3, f, i);
				}
				return;
			}
		}
			//level 2
		start = 0;
		end = start + num2*2;
		for(int i = start;i<end;i+=2)
		{
			//if(i >= landa_h)printf("error warning!\n");
			unsigned short e = heavy[i];
			if(e==(f&255) && heavy[i+1] > 0)
			{
				if(heavy[i+1]<255)heavy[i+1]++;
				else
				{
					levelup(2, f, i);
				}
				return;
			}
		}
		
		//no existing flow
		levelup(1, f, 0);
	}
	int query(unsigned short f)
	{
		int num3 = (usage>>8) & 15;
		int num4 = (usage>>16) & 15;
		int num2 = landa_h/2 - num4*2 - num3*3/2;
		int usage2 = usage & 255;
			//level 4
		int start = num2*2+num3*3;
		int end = start + num4*4;
		for(int i = start;i<end;i+=4)
		{
			unsigned short e = ((unsigned short)heavy[i]<<8)+heavy[i+1];
			if(e==f && (heavy[i+2] +heavy[i+3] > 0))
			{
				return ((int)heavy[i+2]<<8)+heavy[i+3];
			}
		}
			//level 3
		start = num2*2;
		end = start + num3*3;
		for(int i = start;i<end;i+=3)
		{
			unsigned short e = ((unsigned short)heavy[i]<<4)+(heavy[i+1]>>4);
			if(e==(f&4095) && ((heavy[i+1] & 15) + heavy[i+2] > 0))
			{
				return ((int)(heavy[i+1]&15)<<8)+heavy[i+2];
			}
		}
			//level 2
		start = 0;
		end = start + num2*2;
		for(int i = start;i<end;i+=2)
		{
			unsigned short e = heavy[i];
			if(e==(f&255) && heavy[i+1] > 0)
			{
				//if(hash == trace_hash)trace<<"position:"<<i<<" "<<usage2<<" "<<e<<endl;
				return heavy[i+1];
			}
		}
		
		//no existing flow
		return 0;
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
	ofstream out("./dms4.txt");
	//ofstream state("./state4.txt");
	BOBHash32* bob = new BOBHash32(750);
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
			vector<hg_node>hg(BUCKET_NUM);
			clock_gettime(CLOCK_MONOTONIC, &time1);
			for (int i = 0; i < packet_cnt; ++i)
			{
				unsigned int hash = BKDRHash(keys[i]);
				hg[hash % BUCKET_NUM].insert(finger_print(hash));
			}
			clock_gettime(CLOCK_MONOTONIC, &time2);
			resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
			th = (double)1000.0 * test_cycles * packet_cnt / resns;
			/* free memory */
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); ++i)
				delete[] keys[i];
			delete[] keys;
			printf("throughput is %lf mbps\n", th);
			//out<<"throughput:"<<th<<"\n"<<endl;
			gb_throughput += th;
		}
		out<<gb_throughput/10<<endl;
		HEAVY_MEM += 50*1024;
	}
	delete bob;
}
