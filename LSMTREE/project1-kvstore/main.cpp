
#include <iostream>
#include <ctime>
#include <string>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <map>
using namespace std;
// #include "kvstore.h"

using namespace std;
struct CStudent
{
	uint64_t age;
    string szName;
};
int main()
{
	cout<<(4<pow(2,1+1))<<endl;
    // CStudent s;
	// s.age=20;
	// s.szName="mikeshaw";
	// CStudent*p=&s;
    // ofstream outFile("students.dat", ios::out | ios::binary);
    // outFile.write((char*)&p->age, sizeof(s.age));
	// const char*tmp=s.szName.c_str();
	// uint64_t size=strlen(tmp)+1;
	// cout<<size<<endl;
 	// outFile.write((char*)&p->szName, size);
    // outFile.close();

    // ifstream inFile("students.dat",ios::in|ios::binary); //二进制读方式打开
	// char*buffer;
	// uint64_t key;
	// buffer=new char[size];
    // if(!inFile) {
    //     cout << "error" <<endl;
    //     return 0;
    // }

	// // inFile.read((char *)&buffer, 8);
	// inFile.read((char *)&key, 8);
   	// inFile.read((char *)&buffer, size);
	// cout << key << " "<< buffer << endl;   
    // cout<<"1"<<endl;
    // inFile.close();
	// cout<<"2"<<endl;
	// delete []buffer;
	// cout<<"3"<<endl;
    return 0;
}

	// CStudent s;      
	// char*buffer; 
    // ifstream inFile("students.dat",ios::in|ios::binary); //二进制读方式打开
	// buffer=new char[8];
    // if(!inFile) {
    //     cout << "error" <<endl;
    //     return 0;
    // }

	// // inFile.read((char *)&buffer, 8);
    // while(inFile.read((char *)&buffer, 8)) { //一直读到文件结束
		
  
    //     cout << buffer << endl;   
    // }
    // inFile.close();
	// delete buffer;
    // return 0;


// int main()
// {
	// uint64_t val;
	// SkipList *myList;
	// SkipList_Init(myList);

	// while (cin >> val)
	// {   
    //     cout<<"输入999进行下一项测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	if (!SkipList_Insert(myList, val, "1"))
	// 		cout << val << "失败" << endl;
	// 	else
	// 		cout << val << "成功" << endl;
	// }
    // SNode * tmp=NULL;
	// while (cin >> val)
	// {
	//     cout<<"输入999进行下一项测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	tmp=SkipList_Find(myList, val);
    //     if(tmp==NULL)cout << val << "失败" << endl;
    //     else
    //     {
    //         cout<<tmp->value<<endl;
    //     }
        
	// }
    // while (cin >> val)
	// {
	// 	cout<<"输入999结束测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	if (!SkipList_Delete(myList, val))
	// 		cout << val << "删除失败" << endl;
	// 	else
	// 		cout << val << "删除成功" << endl;
	// }
	//以上测试通过

	//测试接口
	// KVStore Kv("test");
	// while (cin >> val)
	// {   
    //     cout<<"输入999进行下一项测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	Kv.put(val,"123");
	// 	cout<<"done!"<<endl;
	
	// }
	// string tmp;
	// while (cin >> val)
	// {
	//     cout<<"输入999进行下一项测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	tmp=Kv.get(val);
    //     if(tmp=="")cout << val << "失败" << endl;
    //     else
    //     {
    //         cout<<tmp<<endl;
    //     }
        
	// }
    // while (cin >> val)
	// {
	// 	cout<<"输入999结束测试"<<endl;
	// 	if (val == 999)
	// 		break;
	// 	if (!Kv.del(val))
	// 		cout << val << "删除失败" << endl;
	// 	else
	// 		cout << val << "删除成功" << endl;
	// }
	// string val="mikeshaw";
	// cout<<sizeof(val)<<endl;
	// const char* val1 = val.c_str();
	// cout<<strlen(val1)<<endl;
	// cout<<2*1024*1024<<endl;
	// return 0;
// }
