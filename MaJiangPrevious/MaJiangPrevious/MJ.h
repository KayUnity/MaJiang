#ifndef _MJ_H
#define _MJ_H

#include "DS_List.h"
#include "DS_HashTable.h"
#include <string>

using namespace DataStructures;
using  namespace std;

class ArrayesClass;
class ArrayClass;

HexString ToString(List< int >* data, char* delim = "");
void Delete(int val, List< int >* data);
bool AnyGreater(int val, List< int >* data);
bool AnyGreater(int val, List< List< int > >* data);
// ArrayClass pattern(ArrayClass arr);

class ArrayClass
{
public:
	List< List< int > > mArray;
	ArrayClass();
	~ArrayClass();
	ArrayClass(const List< int >* data);
	ArrayClass(const List< List< int > >* data);
	ArrayClass(const ArrayClass& other);
	ArrayClass& operator = (const ArrayClass& other);
	List< int >& operator[](const int& index);
	void Merge(int i, int val, int j, List< int >* data);
	ArrayClass& operator + (const ArrayClass& other);
	ArrayClass& operator += (const ArrayClass& other);
	void ToArray(int i, int j, List< int >* data);
	void ToArray(int index, List< int >* data);
	void ParseArray(List< int > data, List< List< int > >* datas);
	ArrayClass Pattern();
	int Key();
	void FindPos(List< int >* results);
	void Delete(int i);
	int Size();
	static void Clear(List< List< int > >* arr);
	void Print();
	void Uniqe();
	void RemoveNot2();
	ArrayClass Perms();
	ArrayesClass ToArrayes();
};

class ArrayesClass
{
public:
	List<ArrayClass*> mArrayes;
	ArrayesClass();
	~ArrayesClass();

	void Initialize();
	ArrayesClass(const ArrayesClass&& other);
	ArrayesClass& operator= (const ArrayesClass& other);

	void AddArray(const List< List< int > >* data);
	void Calculate(HashTable<int, List<int>>* tables);
	void Clear();

};


#endif