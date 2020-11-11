#ifndef _KVSTORE_H_
#define _KVSTORE_H_

#include "kvstore_api.h"
#include <iostream>
#include <ctime>
#include <string>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cstdlib>
using namespace std;
namespace fs=std::filesystem;


const int MAX_LEVEL = 15;

int count_times=0;

struct SNode
{
	uint64_t key;
	string value;
	SNode *forword[MAX_LEVEL];
};

struct SkipList
{
	int number=0;
	int size=0;
	int nowLevel;
	SNode *head;
};

//SSTable Index
struct Index
{
	uint64_t key;
    int offset;
};

struct SSTable
{
    uint64_t min=1;
	uint64_t max=0;
};

void SkipList_Init(SkipList *&myList)
{
	myList = new SkipList;
	myList->nowLevel = 0;
	myList->size = 0;
	myList->number=0;
	myList->head = new SNode;
	for (int i = 0; i < MAX_LEVEL; i++)
		myList->head->forword[i] = NULL;
}

void SkipList_Insert(SkipList *myList, uint64_t val, string s)
{
	if (NULL == myList)
		return ;
	
	int k = myList->nowLevel;
	SNode *q, *p = myList->head;
	SNode *upDateNode[MAX_LEVEL];
	while (k >= 0)
	{
		q = p->forword[k];
		
		while (NULL != q && q->key < val)
		{
			p = q;
			q = p->forword[k];
		}
		
		if (NULL != q && q->key == val){
			const char* oldvalue=q->value.c_str();
			const char* newvalue=s.c_str();
			myList->size = myList->size - strlen(oldvalue) + strlen(newvalue) ;
			q->value=s;
			return ;
		}
		

		upDateNode[k] = p;
		--k;
	}
	
	k=1;
	srand((int)time(0));
	int i=rand()%2;
	while (i)
	{
		k+=1;
		i=rand()%2;
	}

	if(k>=MAX_LEVEL)k =(MAX_LEVEL - 2); //生长概率0.5的成长方式

	if (k > myList->nowLevel)
	{
		k = ++myList->nowLevel;
		upDateNode[k] = myList->head;
	}
	p = new SNode;
	p->key = val;
	p->value = s;
	for (int i = 0; i <= k; i++)
	{
		q = upDateNode[i];
		p->forword[i] = q->forword[i];
		q->forword[i] = p;
	}
	//for(int i=k+1;i<=myList->nowLevel;i++)
	for (int i = k + 1; i < MAX_LEVEL; i++)
		p->forword[i] = NULL;

	//update memtable states
	myList->number+=1;
	const char* tmpstr=s.c_str();
	myList->size = myList->size + 8 + 4 + strlen(tmpstr) + 1; //8 is for key, 4 is int of length of value
	return ;
}

SNode *SkipList_Find(SkipList *myList, uint64_t val)
{
	if (NULL == myList)
		return NULL;

	int k = myList->nowLevel;
	SNode *q, *p = myList->head;
	while (k >= 0)
	{
		q = p->forword[k];
		while (NULL != q && q->key < val)
		{
			p = q;
			q = p->forword[k];
		}
		if (NULL != q && q->key == val)
			return q;
		--k;
	}
	return NULL;
}

bool SkipList_Delete(SkipList *myList, uint64_t val)
{
	SNode *ret = SkipList_Find(myList, val);
	if (NULL == ret){
		SkipList_Insert(myList,val,"");
	
		return true;
	}
		
	if(ret->value=="")
		return false;
	SkipList_Insert(myList,val,"");
	
	return true;
}

void SkipList_Release(SkipList *myList)
{
	myList->number=0;
	myList->nowLevel=0;
	myList->size=0;
	if (NULL == myList)
		return ;
	SNode *q, *p = myList->head;
	if (myList->head->forword[0]==NULL)
	{
		delete myList->head;
		delete myList;
		return;
	}
	
	while (p->forword[0] != NULL)
	{
		q = p->forword[0];
		delete p;
		p=q;
	}
	delete p;
	delete myList;
	
}

Index* SkipList_CreateIndex(SkipList *myList){
	Index * q = (struct Index *)malloc(myList->number * sizeof(struct Index));
	int i=0;
	int offset_pos=24; //8 for itself, 8 for min key , 8 for max key. 
	SNode*p = myList->head->forword[0];
	while (p!=NULL)
	{
		const char* tmpstr=p->value.c_str();
		int size = strlen(tmpstr)+1;

		q[i].key=p->key;
		q[i].offset=offset_pos;
		
		i++;
		offset_pos=offset_pos+8+size+4;//4 is length of size
		p=p->forword[0];
	}
	return q;
	
}


class KVStore : public KVStoreAPI {
	// You can add your implementation here
private:
	
public:
	SkipList *memtable;
	SkipList *memory;
	Index *disk;
	SSTable **key_range;
	string p_dir;
	int dir_level=0;
	int sstable_level=1;
	KVStore(const std::string &dir);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

	void createfile();

	void compaction(int level);
};


KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
	p_dir=dir;
	string menu = dir+"/level0";
	if (!fs::exists(menu))
	{
		fs::create_directories(menu);
	}
	
	SkipList_Init(memtable);
	SkipList_Init(memory);
	key_range=new SSTable*[20];
	key_range[0]=new SSTable[4];

	//get key_range according to local data.

	string check_path="./data/level"+to_string(dir_level+1);
	while (fs::exists(check_path))
	{
		dir_level++;
		key_range[dir_level]=new SSTable[int(pow(2,dir_level+2))];
		check_path="./data/level"+to_string(dir_level+1);
	}

	for (int i = 0; i <= dir_level; i++){
		for (int j = 1; j <= pow(2,i+1); j++){
			string cur_ss_path="./data/level"+to_string(i)+"/SSTable"+to_string(j)+".hex";
			string cur_uss_path="./data/level"+to_string(i)+"/usedss"+to_string(j)+".hex";
			if(fs::exists(cur_ss_path)){
				ifstream inFile(cur_ss_path,ios::in|ios::binary);
				inFile.seekg(8,ios::beg);
				uint64_t range_min;
				uint64_t range_max;
				inFile.read((char*)&range_min,8);
				inFile.read((char*)&range_max,8);
				key_range[i][j].min=range_min;
				key_range[i][j].max=range_max;
				inFile.close();
			}
			else{
				if (fs::exists(cur_uss_path)){
					ifstream inFile(cur_uss_path,ios::in|ios::binary);
					inFile.seekg(8,ios::beg);
					uint64_t range_min;
					uint64_t range_max;
					inFile.read((char*)&range_min,8);
					inFile.read((char*)&range_max,8);
					key_range[i][j].min=range_min;
					key_range[i][j].max=range_max;
					inFile.close();
					fs::rename(cur_uss_path,cur_ss_path);
				}
			}	
		}
	}

}

KVStore::~KVStore()
{

	SkipList_Release(memtable);
	SkipList_Release(memory);
	for (int i = 0; i <= dir_level; i++)
	{
		delete []key_range[i];
	}
	delete []key_range;
	
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	//cout<<key<<endl;
	if (memtable->size>2*1024*1024)
	{
		createfile();
		SkipList_Release(memtable);
		SkipList_Init(memtable);
	}
	
	SkipList_Insert(memtable,key,s);
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
	SNode* res=SkipList_Find(memtable,key);
	if (res==NULL){
		//unfound in memtable
			//in level0 we need to search from ss3 first
			for (int j = 3; j >=1 ; j--){
				if(key>=key_range[0][j].min && key<=key_range[0][j].max){
					
					string search_path="./data/level0/SSTable"+to_string(j)+".hex";
					//cout<<search_path<<endl;
					ifstream inFile(search_path,ios::in|ios::binary);
					uint64_t index_offset;
					inFile.read((char*)&index_offset,8);
					inFile.seekg(index_offset,ios::beg);
					Index s;
					int v_length;
					while(!inFile.eof()) { 
						inFile.read((char*)&s.key,8);	
						if(s.key==key){
							inFile.read((char*)&s.offset,4);
							inFile.seekg(s.offset+8,ios::beg);
							inFile.read((char*)&v_length,4);
							char tmp[v_length];
							inFile.read((char*)&tmp,v_length);
							string tmpstr(tmp);									
							inFile.close();
							return tmpstr;
						}
						if(s.key==key_range[0][j].max){		
							break;
						}
						index_offset+=12;
						inFile.seekg(index_offset,ios::beg);
					}
					inFile.close();
				}
			}
		

		for (int i = 1; i <= dir_level; i++){
			for (int j = 1; j <= pow(2,i+1); j++){
			
				if(key>=key_range[i][j].min && key<=key_range[i][j].max){
					
					string search_path="./data/level"+to_string(i)+"/SSTable"+to_string(j)+".hex";
					//cout<<search_path<<endl;
					ifstream inFile(search_path,ios::in|ios::binary);
					uint64_t index_offset;
					inFile.read((char*)&index_offset,8);
					inFile.seekg(index_offset,ios::beg);
					Index s;
					int v_length;
					while(!inFile.eof()) { 
						inFile.read((char*)&s.key,8);	
						if(s.key==key){
							inFile.read((char*)&s.offset,4);
							inFile.seekg(s.offset+8,ios::beg);
							inFile.read((char*)&v_length,4);
							char tmp[v_length];
							inFile.read((char*)&tmp,v_length);
							string tmpstr(tmp);									
							inFile.close();
							return tmpstr;
						}
						if(s.key==key_range[i][j].max){		
							break;
						}
						index_offset+=12;
						inFile.seekg(index_offset,ios::beg);
					}
					inFile.close();
				}
			}
		} 
		return "";
	}
	
	return res->value;
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	return SkipList_Delete(memtable,key);
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	for (int i = 0; i <= dir_level; i++)
		{
			string delete_path="./data/level"+to_string(i);
			fs::remove_all(delete_path);
		}

	string menu = p_dir+"/level0";
	fs::create_directories(menu);

	
	for (int i = 0; i <= dir_level; i++)
	{
		delete []key_range[i];
	}
	delete []key_range;

	dir_level=0;
	sstable_level=1;

	key_range=new SSTable*[20];
	key_range[0]=new SSTable[4];
	
	SkipList_Release(memtable);
	SkipList_Init(memtable);

}


void KVStore::createfile(){
	
	string file_path="./data/level0/SSTable"+to_string(sstable_level)+".hex";
	// cout<<"run to here: create "<<file_path<<endl;
	disk=SkipList_CreateIndex(memtable);
	
	ofstream outFile(file_path, ios::out | ios::binary);
	SNode*p = memtable->head->forword[0];
	uint64_t index_pos = memtable->size+24;
	outFile.write((char*)&index_pos, 8);
	if (p!=NULL){
		outFile.write((char*)&(p->key), 8);
		key_range[0][sstable_level].min=p->key;
	}//store min key
	while (p->forword[0]!=NULL)
	{
		p=p->forword[0];
	}
	outFile.write((char*)&(p->key), 8);
	//store max key
	key_range[0][sstable_level].max=p->key;

	p = memtable->head->forword[0];
	while (p!=NULL)
	{
		outFile.write((char*)&(p->key), 8);
		const char* tmpstr=p->value.c_str();
		int size = strlen(tmpstr)+1;
		outFile.write((char*)&size, 4);
		outFile.write((char*)tmpstr, size);
		p=p->forword[0];
	}
	int count=memtable->number;
	for (int i = 0; i < count; i++)
	{
		outFile.write((char*)&disk[i].key, 8);
		outFile.write((char*)&disk[i].offset, 4);
	}
    outFile.close();

	free(disk);
	sstable_level+=1;
	compaction(0);


}


void KVStore::compaction(int level){
	//cout<<"compaction: "<<level<<endl;
	//sstable是否超标？
	int x=pow(2,(level+1))+1;
	string path="./data/level"+to_string(level)+"/SSTable"+to_string(x)+".hex";
	
	if (!fs::exists(path))
	{
		//cout<<"no exist:"<<path<<"so return"<<endl;
		return;
	}
	
	//cout<<" exist:"<<path<<"so go next"<<endl;
	//存在超标量的sstable
	if(level>=0){
	//debug		//cout<<" exist:"<<path<<"so go next"<<endl;
		SkipList_Release(memory);//clear memory
		SkipList_Init(memory);
		
		uint64_t range_min;
		uint64_t range_max;
		uint64_t min;
		uint64_t max;

		int overflow_start=x;
		int overflow_end=x;
		int o_imm=1;
		if (level==0)
		{
			overflow_start=1;
			overflow_end=3;
			o_imm=3;
		}
		else
		{
			string current_overflow="./data/level"+to_string(level)+"/SSTable"+to_string(x+1)+".hex";
			while (fs::exists(current_overflow))
			{
				overflow_end++;
				o_imm++;
				current_overflow="./data/level"+to_string(level)+"/SSTable"+to_string(x+o_imm)+".hex";
			
			}
		}
		
	//debug		//cout<<"overflow_start: "<<overflow_start<<" overflow_end: "<<overflow_end<<endl;
		
		string overflow_path="./data/level"+to_string(level)+"/SSTable"+to_string(overflow_start)+".hex";

		ifstream inFile1(overflow_path,ios::in|ios::binary);
		inFile1.seekg(8,ios::beg);
		inFile1.read((char*)&range_min,8);
		inFile1.read((char*)&range_max,8);
		inFile1.close();

		for (int i = overflow_start+1; i <= overflow_end; i++)
		{
			overflow_path="./data/level"+to_string(level)+"/SSTable"+to_string(i)+".hex";
			ifstream inFile2(overflow_path,ios::in|ios::binary);
			inFile2.seekg(8,ios::beg);
			inFile2.read((char*)&min,8);
			inFile2.read((char*)&max,8);
			if(min<range_min)range_min=min;
			if(max>range_max)range_max=max;
			inFile2.close();
		}
		


		int last_ss=0;//set the last sstable in next level;
		int caseflag=0;//defualt case: exist sstables in level(x+1) that have same range
		int imm=1;


		//is next level exist?
		string dir="./data/level"+to_string(level+1);
		if(!fs::exists(dir)){
	//debug			
			//cout<<"no exist:"<<dir<<" so create it"<<endl;
			fs::create_directories(dir);
			key_range[level+1]=new SSTable[int(pow(2,level+3))];
			dir_level=level+1;
			caseflag=1;
		}

		string next_level_path="./data/level"+to_string(level+1)+"/SSTable"+to_string(imm)+".hex";
		while (fs::exists(next_level_path))
		{
			last_ss++;
			imm++;
			next_level_path="./data/level"+to_string(level+1)+"/SSTable"+to_string(imm)+".hex";
			
		}
		
		if (last_ss==0)caseflag=1;//case1: just put memory into level(x+1)
	
		string sstable1="./data/level"+to_string(level+1)+"/SSTable1.hex";
		if (caseflag==0&&fs::exists(sstable1))
		{
			ifstream inFile(sstable1,ios::in|ios::binary);
			inFile.seekg(8,ios::beg);
			inFile.read((char*)&min,8);
			inFile.close();
			if(min>range_max)caseflag=2;//case2: put memory into the head of level(x+1)/ss1
		}

		if (!caseflag&&last_ss!=0)
		{
			string last_file="./data/level"+to_string(level+1)+"/SSTable"+to_string(last_ss)+".hex";
			ifstream inFile(last_file,ios::in|ios::binary);
			inFile.seekg(16,ios::beg);
			inFile.read((char*)&max,8);
			inFile.close();
			if(max<range_min)caseflag=3;//case3: put memory into the tail of level(x+1)
		}
		
		
		int head=0;
		int tail=0;
		if (caseflag==0&&last_ss!=0){	
			for (int j=1;j<=last_ss;j++){

				string curfile="./data/level"+to_string(level+1)+"/SSTable"+to_string(j)+".hex";
				ifstream inFile(curfile,ios::in|ios::binary);
				inFile.seekg(8,ios::beg);
				inFile.read((char*)&min,8);
				inFile.read((char*)&max,8);
				inFile.close();
				
	//debug			//cout<<"min: "<<min<<" max: "<<max<<endl;
				//cout<<"range_min: "<<range_min<<" range_max: "<<range_max<<endl;
				
				if ( (min>=range_min&&min<=range_max) ||
					 (max>=range_min&&max<=range_max) ||
					 (min<=range_min&&range_min<=max) ||
					 (min<=range_max&&range_max<=max) ){
					if(head==0)head=j;
					tail=j;
				}
				if (min>range_max)break;
			}
		}
	//debug	//cout<<"caseflag: "<<caseflag<<endl;
		//cout<<"head: "<<head<<" tail: "<<tail<<" last_ss: "<<last_ss<<endl;
		//insert level+1 files
		if (caseflag==0)
		{
			for (int j = head; j <= tail; j++)
			{
				string next_level_files="./data/level"+to_string(level+1)+"/SSTable"+to_string(j)+".hex";
				string used_sstable="./data/level"+to_string(level+1)+"/usedss"+to_string(j)+".hex";
				ifstream inFile(next_level_files,ios::in|ios::binary);
				uint64_t index_offset;
				uint64_t max;
				inFile.read((char*)&index_offset,8);
				
				inFile.seekg(16,ios::beg);

				inFile.read((char*)&max,8);

				inFile.seekg(index_offset,ios::beg);
				Index s;
				int v_length;
			
				while(!inFile.eof()) { 
					inFile.read((char*)&s.key,8);

					inFile.read((char*)&s.offset,4);

					inFile.seekg(s.offset+8,ios::beg);

					inFile.read((char*)&v_length,4);

					char tmp[v_length];

					inFile.read((char*)&tmp,v_length);

					string tmpstr(tmp);

					SkipList_Insert(memory,s.key,tmpstr);

					if(s.key==max){
		
					break;
					}

					index_offset+=12;

					inFile.seekg(index_offset,ios::beg);
				}
				inFile.close();
				fs::rename(next_level_files,used_sstable);//delete used sstable
	//debug			//cout<<"run to here: delete used sstable "<<endl;
			}
			
		}
		
	//debug	//cout<<" overflow_start and overflow_end"<<overflow_start<<"    "<<overflow_end<<endl;
		//insert current level files
		for (int i = overflow_start; i <= overflow_end; i++)
		{
			string cur_level_files="./data/level"+to_string(level)+"/SSTable"+to_string(i)+".hex";
			
			ifstream inFile(cur_level_files,ios::in|ios::binary);
			
			uint64_t index_offset;
			uint64_t max;
			inFile.read((char*)&index_offset,8);
			inFile.seekg(16,ios::beg);
			inFile.read((char*)&max,8);
			
			inFile.seekg(index_offset,ios::beg);
			
			Index s;
			int v_length;
			
			while(!inFile.eof()) { 
				inFile.read((char*)&s.key,8);

				inFile.read((char*)&s.offset,4);
				
				inFile.seekg(s.offset+8,ios::beg);
				
				inFile.read((char*)&v_length,4);
				
				char tmp[v_length];

				inFile.read((char*)&tmp,v_length);
			
				string tmpstr(tmp);
				
				SkipList_Insert(memory,s.key,tmpstr);
				
				if(s.key==max){
		
					break;
				}

				index_offset+=12;
				
				inFile.seekg(index_offset,ios::beg);
			}
			inFile.close();
			
		}
		
		//divide memory into new sstables
		//count num of new sstable
		int ss_num;
		int fact_num=0;
		int changeflag=1;
		ss_num=memory->size/(2060*1024);
		if ((memory->size-ss_num*(2*1024*1024))>(2*1024*1024-1)){
			ss_num++;
		}
	//debug		cout<<"ss_sum:"<<ss_num<<endl;
		//generate tmp sstable then rename these file to sstable(x),they are: tmpss1.hex, tmpss2.hex,...,tmpss(ss_num).hex.
		uint64_t size_count=0;
		uint64_t currentmin=0;	bool min_flag=0;
		uint64_t currentmax=0;
		int nodenumber=0;
		string tmpss_path;
		SNode *currentnode=memory->head->forword[0];
		SNode *currentcount=memory->head->forword[0];
		SSTable tmp_index[ss_num+1];
		for (int j = 1; j <= ss_num; j++)
		{

			if (currentcount==NULL)break;
			
			tmpss_path="./data/level"+to_string(level+1)+"/tmpss"+to_string(j)+".hex";
			if(fs::exists(tmpss_path))fs::remove(tmpss_path);
	//debug			cout<<"create files:"<<tmpss_path<<endl;
			ofstream outFile(tmpss_path, ios::out | ios::binary);

			while ((size_count<(2*1024*1024)||j==ss_num)&&currentcount!=NULL)
			{
				if(!min_flag){currentmin=currentcount->key;min_flag=1;}
				currentmax=currentcount->key;
				const char* tmpstr=currentcount->value.c_str();
				int size = strlen(tmpstr)+1;
				size_count=size_count+size+8+4;
				nodenumber++;
				currentcount=currentcount->forword[0];
				if(changeflag){fact_num++;changeflag=0;}

			}

			if(nodenumber==0)break;

			//insert data and index
			//8 for itself, 8 for min key , 8 for max key. 
			int offset_pos=24; 
			uint64_t offset_index=size_count+24;
			outFile.write((char*)&offset_index, 8);

			outFile.write((char*)&currentmin, 8);
			tmp_index[j].min=currentmin;

			outFile.write((char*)&currentmax, 8);
			tmp_index[j].max=currentmax;

			Index * q = (struct Index *)malloc(nodenumber * sizeof(struct Index));
			
			for (int i=0;i<nodenumber;i++)
			{
				const char* tmpstr=currentnode->value.c_str();
				int size = strlen(tmpstr)+1;

				//write ss
				outFile.write((char*)&(currentnode->key), 8);
				outFile.write((char*)&size, 4);
				outFile.write((char*)tmpstr, size);

				//save index
				q[i].key=currentnode->key;
				q[i].offset=offset_pos;
				
				offset_pos=offset_pos+8+size+4;//4 is length of size
				currentnode=currentnode->forword[0];
			}

			//write index
			for (int i = 0; i < nodenumber; i++){
				outFile.write((char*)&q[i].key, 8);
				outFile.write((char*)&q[i].offset, 4);
			}
			free(q);

			outFile.close();

			nodenumber=0;
			size_count=0;
			currentmin=0;
			min_flag=0;
			currentmax=0;
			changeflag=1;
		}
		
		//fix ss_num error if exists
		if(fact_num<ss_num)ss_num=fact_num;

		//now: the files in level1 are:
		//SSTable1~SSTable(head-1),head may == 1
		//SSTable(Tail+1)...SSTable(last_ss),last_ss may == Tail
		//tmpss1.hex, tmpss2.hex,...,tmpss(ss_num).hex.


		switch (caseflag)
		{
			//normal case: put new ss in medium of level1
		case 0:{
			if(last_ss==tail){
				for (int i = 1; i <= ss_num; i++)
				{
					//example:   ss1,ss2,tmpss1,tmpss2 (head=3,ss_sum=2)
					//then:		 tmpss1,tmpss2=>ss3,ss4
					string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
					string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+i-1)+".hex";
					key_range[level+1][head+i-1]=tmp_index[i];
					fs::rename(oldname,newname);
				}
				break;
			}
			if(last_ss>tail){
				if(tail-head+1==ss_num){
					//example:   ss1,ss2,ss5,ss6,tmpss1,tmpss2 (head=3,tail=4,ss_sum=2)
					//then:		 tmpss1,tmpss2=>ss3,ss4
					for (int i = 1; i <= ss_num; i++)
					{
						string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
						string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+i-1)+".hex";
						key_range[level+1][head+i-1]=tmp_index[i];
						fs::rename(oldname,newname);
					}
					break;
				}
				if(tail-head+1>ss_num){
					//example:   ss1,ss2,ss6,ss7,tmpss1,tmpss2 (head=3,tail=5,ss_sum=2)
					//first:     ss6,ss7=>ss5,ss6
					//then:		 tmpss1,tmpss2=>ss3,ss4
					for (int i = tail+1; i <= last_ss; i++)
					{
						string oldname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i)+".hex";
						string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+ss_num+i-tail-1)+".hex";
						key_range[level+1][head+ss_num+i-tail-1]=key_range[level+1][i];
						fs::rename(oldname,newname);
					}
					for (int i = 1; i <= ss_num; i++)
					{
						string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
						string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+i-1)+".hex";
						key_range[level+1][head+i-1]=tmp_index[i];
						fs::rename(oldname,newname);
					}
					break;
				}
				if(tail-head+1<ss_num){
					//example:   ss1,ss2,ss5,ss6,tmpss1,tmpss2,tmpss3 (head=3,tail=4,ss_sum=3)
					//first:     ss5,ss6=>ss6,ss7
					//then:		 tmpss1,tmpss2,tmpss3=>ss3,ss4,ss5
					for (int i = last_ss; i >= tail+1; i--)
					{
						string oldname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i)+".hex";
						string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+ss_num+i-tail-1)+".hex";
						key_range[level+1][head+ss_num+i-tail-1]=key_range[level+1][i];
						fs::rename(oldname,newname);
					}
					for (int i = 1; i <= ss_num; i++)
					{
						string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
						string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(head+i-1)+".hex";
						key_range[level+1][head+i-1]=tmp_index[i];
						fs::rename(oldname,newname);
					}
					break;
				}
			}
			cout<<"something wrong with the tmpss and ss."<<endl;
			break;}
			//special case1: level"+to_string(level+1)+" have no ss, so just put all new ss in the level"+to_string(level+1)+"
		case 1:{
			for (int i = 1; i <= ss_num; i++)
				{
					string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
					string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i)+".hex";
					key_range[level+1][i]=tmp_index[i];
					fs::rename(oldname,newname);
				}
			break;
			}
			//special case2: put all tmpss in the head
		case 2:{
			//example:   ss1,ss2,ss3,ss4,tmpss1,tmpss2,tmpss3 (head=0,tail=0,ss_sum=3)
			//first:     ss1,ss2,ss3,ss4...=>ss4,ss5,ss6,ss7...
			//then:		 tmpss1,tmpss2,tmpss3=>ss1,ss2,ss3
			for (int i = last_ss; i >= 1; i--)
				{
					string oldname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i)+".hex";
					string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(ss_num+i)+".hex";
					key_range[level+1][ss_num+i]=key_range[level+1][i];
					fs::rename(oldname,newname);
				}
			for (int i = 1; i <= ss_num; i++)
				{
					string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
					string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i)+".hex";
					key_range[level+1][i]=tmp_index[i];
					fs::rename(oldname,newname);
				}
			break;
			}
			//special case3: put all tmpss in the tail
		case 3:{
			//example:   ss1,ss2,ss3,ss4,tmpss1,tmpss2,tmpss3 (head=0,tail=0,ss_sum=3)
			//first:     ss1,ss2,ss3,ss4...=>ss1,ss2,ss3,ss4...
			//then:		 tmpss1,tmpss2,tmpss3=>ss1,ss2,ss3
			for (int i = 1; i <= ss_num; i++)
				{
					string oldname="./data/level"+to_string(level+1)+"/tmpss"+to_string(i)+".hex";
					string newname="./data/level"+to_string(level+1)+"/SSTable"+to_string(i+last_ss)+".hex";
					key_range[level+1][i+last_ss]=tmp_index[i];
					fs::rename(oldname,newname);
				}
			break;}

		}

		//delete next level used files (renamed to usedss(x).hex)
		for (int j = head; j <= tail; j++){
			string used_sstable="./data/level"+to_string(level+1)+"/usedss"+to_string(j)+".hex";
			fs::remove(used_sstable);
		}

		//delete current level overflow files
		for (int i = overflow_start; i <= overflow_end; i++)
		{
			overflow_path="./data/level"+to_string(level)+"/SSTable"+to_string(i)+".hex";
			fs::remove(overflow_path);
			SSTable tmp;
			key_range[level][i]=tmp;
		}

		if(level==0)sstable_level=1;
		compaction(level+1);
	}
	
	
	
}


#endif
