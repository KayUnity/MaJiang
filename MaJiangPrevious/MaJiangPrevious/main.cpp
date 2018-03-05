#include "MJ.h"
#include <stdio.h>

int main()
{
	HashTable<int, List<int>> table;
	ArrayesClass clazz;
	clazz.Initialize();
	clazz.Calculate(&table);
	getchar();
	return 0;
}