#include "MJ.h"
#include <string>

#include <iostream>
#include <fstream>

ArrayClass::ArrayClass(){}
ArrayClass::~ArrayClass(){ Clear(&mArray); }

ArrayClass::ArrayClass(const List< int >* data)
{
	mArray.Push(*data);
}

ArrayClass::ArrayClass(const List< List< int > >* data)
{
	mArray = *data;
}

ArrayClass::ArrayClass(const ArrayClass& other)
{
	Clear(&mArray);
	mArray = other.mArray;
}

ArrayClass& ArrayClass::operator = (const ArrayClass& other)
{
	if (this != &other)
	{
		Clear(&mArray);
		mArray = other.mArray;
	}
	return *this;
}

List< int >& ArrayClass::operator[](const int& index)
{
	return mArray[index];
}

void ArrayClass::Merge(int i, int val, int j, List< int >* data)
{
	data->Clear();
	ToArray(i, data);
	data->Push(val);
	ToArray(j, data);
}

ArrayClass& ArrayClass::operator + (const ArrayClass& other)
{
	List< List< int > > arr = other.mArray;
	int size = arr.Size();
	for (int j = 0; j < size; ++j)
	{
		mArray.Push(arr[j]);
	}
	Clear(&arr);
	return *this;
}

ArrayClass& ArrayClass::operator += (const ArrayClass& other)
{
	return this->operator+ (other);
}

void ArrayClass::ToArray(int i, int j, List< int >* data)
{
	for (int k1 = 0; k1 < mArray[j].Size(); ++k1)
	{
		data->Push(0);
	}
	for (int k1 = 0; k1 < mArray[i].Size(); ++k1)
	{
		data->Push(mArray[i][k1]);
	}
	for (int k1 = 0; k1 < mArray[j].Size(); ++k1)
	{
		data->Push(0);
	}
}

void ArrayClass::ToArray(int index, List< int >* data)
{
	int size = mArray.Size();
	if (index >= 0 && index < size)
	{
		size = mArray[index].Size();
		for (int j = 0; j < size; ++j)
		{
			data->Push(mArray[index][j]);
		}
		data->Push(0);
	}
	else if (index == -1)
	{
		for (int j = 0; j < size; ++j)
		{
			ToArray(j, data);
		}
	}
}

void ArrayClass::ParseArray(List< int > data, List< List< int > >* datas)
{
	unsigned int index = data.GetIndexOf(0);
	if (index > data.Size())
	{
		if (data.Size() > 0)
		{
			datas->Push(data);
		}
		return;
	}
	while (index > 0 && index < data.Size())
	{
		List< int > temp;
		for (int i = 0; i < index; ++i)
		{
			temp.Push(data[0]);
			data.RemoveAtIndex(0);
		}
		data.RemoveAtIndex(0);
		index = data.GetIndexOf(0);
		datas->Push(temp);
		temp.Clear();
	}
}

int ArrayClass::Key()
{
	int ret = 0;
	int len = -1;
	int size = mArray.Size();
	for (int i = 0; i < size; ++i)
	{
		List<int>* dataI = &mArray[i];
		int subSize = dataI->Size();
		for (int j = 0; j < subSize; ++j)
		{
			++len;
			switch ((*dataI)[j])
			{
			case 2:
				ret |= 0x3 << len;
				len += 2;
				break;
			case 3:
				ret |= 0xF << len;
				len += 4;
				break;
			case 4:
				ret |= 0x3F << len;
				len += 6;
				break;
			}
		}
		ret |= 0x1 << len;
		++len;
	}
	return ret;
}

// ret
// 下位
//   3bit  0: 三个一样的 个数(0～4)
//   3bit  3: 顺子的个数(0～4)
//   4bit  6: 将的个数(1～13)
//   4bit 10: 保留的位置１(0～13)
//   4bit 14: 保留的位置２(0～13)
//   4bit 18: 保留的位置３(0～13)
//   4bit 22: 保留的位置４(0～13)
//   1bit 26: 胡牌的类型
//	 1bit 27: 胡牌的类型
//   1bit 28: 胡牌的类型
//   1bit 29: 胡牌的类型
//   1bit 30: 胡牌的类型
void ArrayClass::FindPos(List< int >* results)
{
	int size = Size();
	int jiang = 0;
	int ret;
	for (int i = 0; i < size; ++i)
	{
		int subSize = mArray[i].Size();
		for (int j = 0; j < subSize; ++j)
		{
			// 确定 将
			if (mArray[i][j] >= 2)
			{
				// 标识  顺子 和 三个 的 谁先确定
				for (int flag = 0; flag <= 1; ++flag)
				{
					List < List< int > > temp = mArray;
					temp[i][j] -= 2;
					int p = 0;
					List< int > sameList;
					List< int > seqList;
					for (int k = 0; k < temp.Size(); ++k)
					{
						for (int m = 0; m < temp[k].Size(); ++m)
						{
							// 三个 优先
							if (flag == 0)
							{
								// 先 记录 三个
								if (temp[k][m] >= 3)
								{
									temp[k][m] -= 3;
									sameList.Push(p);
								}
								// 找顺子
								while (temp[k].Size() - m >= 3 &&
									temp[k][m] >= 1 &&
									temp[k][m + 1] >= 1 && 
									temp[k][m + 2] >= 1)
								{
									temp[k][m] -= 1;
									temp[k][m + 1] -= 1;
									temp[k][m + 2] -= 1;
									seqList.Push(p);
								}
							}
							else // 顺子优先
							{
								// 找顺子
								while (temp[k].Size() - m >= 3 &&
									temp[k][m] >= 1 &&
									temp[k][m + 1] >= 1 &&
									temp[k][m + 2] >= 1)
								{
									temp[k][m] -= 1;
									temp[k][m + 1] -= 1;
									temp[k][m + 2] -= 1;
									seqList.Push(p);
								}
								// 先 记录 三个
								if (temp[k][m] >= 3)
								{
									temp[k][m] -= 3;
									sameList.Push(p);
								}
							}
							++p;
						}
					}
					// 全部为0
					if (!AnyGreater(0, &temp))
					{
						ret = sameList.Size() + (seqList.Size() << 3) + (jiang << 6);
						int len = 10;
						for (int i = 0; i < sameList.Size(); ++i)
						{
							ret |= sameList[i] << len;
							len += 4;
						}
						for (int i = 0; i < seqList.Size(); ++i)
						{
							ret |= seqList[i] << len;
							len += 4;
						}
						// 确定 牌的 类型
						if (size == 1)
						{

						}
						else if (size <= 3 && seqList.Size() >= 3)
						{

						}
						else if (seqList.Size() == 4 && seqList[0] == seqList[1] && seqList[2] == seqList[3])
						{

						}
						else if (seqList.Size() >= 2 && seqList.Size() + sameList.Size() == 4)
						{

						}

					}
					Clear(&temp);
				}
				++jiang;
			}
		}
	}
}

void ArrayClass::Delete(int i)
{
	assert(i < Size());
	mArray[i].Clear();
	mArray.RemoveAtIndex(i);
}

int ArrayClass::Size()
{
	return mArray.Size();
}

void ArrayClass::Clear(List< List< int > >* arr)
{
	int size = arr->Size();
	for (int i = 0; i < size; ++i)
	{
		(*arr)[i].Clear();
	}
	arr->Clear();
}

void ArrayClass::Print()
{
	int size = mArray.Size();
	printf("[ ");
	for (int i = 0; i < size; ++i)
	{
		const char* temp = ToString(&mArray[i],",").C_String();
		printf("[%s] ", temp);
	}
	printf(" ] \n");
}

class SequnceElement
{
public:
	SequnceElement* mParent;
	int mRoot;
	List< SequnceElement* > mChildren;
	SequnceElement() : mRoot(-1), mParent(0){}
	~SequnceElement(){ Clear(); }
	SequnceElement(int root) : mRoot(root), mParent(0){}
	SequnceElement(int root, SequnceElement* parent) : mRoot(root), mParent(parent){}

	void Clear()
	{
		int size = mChildren.Size();
		for (int i = 0; i < size; ++i)
		{
			delete mChildren[i];
		}
		mChildren.Clear();
	}

	void AddChild(List< int >* children)
	{
		int size = children->Size();
		SequnceElement* element = 0;
		for (int i = 0; i < size; ++i)
		{
			element = new SequnceElement((*children)[i], this);
			mChildren.Push(element);
			List< int > temp = *children;
			temp.RemoveAtIndex(i);
			element->AddChild(&temp);
			temp.Clear();
		}
	}

	bool HasChildren()
	{
		return mChildren.Size() > 0;
	}

	void TravalNode(List< SequnceElement* >* nodes)
	{
		if (HasChildren())
		{
			for (int i = 0; i < mChildren.Size(); ++i)
			{
				mChildren[i]->TravalNode(nodes);
			}
		}
		else
		{
			nodes->Push(this);
		}
	}

	void Traval(List< List< int > >* datas)
	{
		List< SequnceElement* > nodes;
		TravalNode(&nodes);
		SequnceElement* parent;
		for (int i = 0; i < nodes.Size(); ++i)
		{
			List< int > val;
			parent = nodes[i]->mParent;
			val.Push(nodes[i]->mRoot);
			while (parent->mRoot != -1)
			{
				val.Push(parent->mRoot);
				parent = parent->mParent;
			}
			datas->Push(val);
			val.Clear();
		}
		nodes.Clear();
	}
};

ArrayClass ArrayClass::Perms()
{
	ArrayClass result;
	int size = Size();
	if (size > 0)
	{
		List< int > child;
		for (int i = 0; i < size; ++i)
		{
			child.Push(i);
		}
		SequnceElement temp;
		temp.AddChild(&child);
		child.Clear();
		List< List< int > > data;
		temp.Traval(&data);
		size = data.Size();
		for (int i = 0; i < size; ++i)
		{
			ArrayClass temp;
			List< int > one;
			for (int j = data[i].Size() - 1; j >= 0; --j)
			{
				int index = data[i][j];
				temp.mArray.Push(mArray[index]);
			}
			temp.ToArray(-1, &one);
			result.mArray.Push(one);
			one.Clear();
		}
		Clear(&data);
		result.Uniqe();
	}
	return result;
}

ArrayesClass ArrayClass::ToArrayes()
{
	ArrayesClass arrayes;
	int size = mArray.Size();
	if (size > 0)
	{
		for (int i = 0; i < size; ++i)
		{
			List< List< int > > temp;
			ParseArray(mArray[i], &temp);
			arrayes.AddArray(&temp);
			Clear(&temp);
		}
	}
	return arrayes;
}

void Print(List< int >* data)
{
	printf("\n");
	int size = data->Size();
	for (int i = 0; i < size; ++i)
	{
		printf("%d ", (*data)[i]);
	}
}

void ArrayClass::RemoveNot2()
{
	int size = mArray.Size();
	if (size > 0)
	{
		List< int > cache;
		for (int i = 0; i < size; ++i)
		{
			List< List< int > > temp;
			ParseArray(mArray[i], &temp);
			ArrayClass ac(&temp);
			List< int > temp1;
			ac.ToArray(-1, &temp1);
			::Delete(0, &temp1);
			for (int j = 0; j < temp1.Size(); ++j)
			{
				if (temp1[j] != 2)
				{
					cache.Push(i);
					break;
				}
			}
			temp1.Clear();
			Clear(&temp);
		}
		for (int i = cache.Size() - 1; i >= 0; --i)
		{
			mArray.RemoveAtIndex(cache[i]);
		}
		cache.Clear();
	}
}

void ArrayClass::Uniqe()
{
	HashTable<HexString, int> temp;
	List< List< int > > cache;
	int size = mArray.Size();
	for (int i = 0; i < size; ++i)
	{
		HexString str = ToString(&mArray[i], "-");
		if (!temp.InHashTable(str))
		{
			cache.Push(mArray[i]);
			temp.Add(str, 0);
		}
	}
	Clear(&mArray);
	mArray = cache;
	Clear(&cache);
	temp.Clear();
}

ArrayClass ArrayClass::Pattern()
{
	if (Size() == 1)
	{
		this->mArray[0].Push(0);// flag
		return *this;
	}
	ArrayClass result;
	result += Perms();
	int size = Size();
	HexString key;
	HashTable<HexString, int> h1;
	for (int i = 0; i < size; ++i)
	{
		for (int j = i + 1; j < size; ++j)
		{
			List< int > tempD;
			Merge(i, 0, j, &tempD);
			key = ToString(&tempD);
			tempD.Clear();
			if (!h1.InHashTable(key))
			{
				h1.Add(key, 0);
				HashTable<HexString, int> h2;
				List< int > tt;
				ToArray(i, j, &tt);
				List< int > t;
				/// 卷积
				for (int k = 0; k < mArray[i].Size() + mArray[j].Size() + 1; ++k)
				{
					t = tt;
					for (int m = 0; m < mArray[j].Size(); ++m)
					{
						t[k + m] += mArray[j][m];
					}
					::Delete(0, &t);
					key = ToString(&t);
					// printf("****** %s\n", key.C_String());
					if (t.Size() > 9 || AnyGreater(4, &t))
					{
						t.Clear();
						continue;
					}
					if (!h2.InHashTable(key))
					{
						h2.Add(key, 0);
						ArrayClass t2 = *this;
						t2.Delete(i);
						t2.Delete(j - 1); // 因为前面删除了 i，索引前移了一个，故 j - 1 而不是 j
						ArrayClass t3(&t);
						t.Clear();
						t3 = t3 + t2;
						// t3.Print();
						result += t3.Pattern();
					}
				}
				tt.Clear();
				h2.Clear();
			}
		}
	}
	h1.Clear();
	return result;
}

HexString ToString(List< int >* data, char* delim)
{
	const int size = data->Size();
	char* temp = (char*)malloc(size * sizeof(int));
	memset(temp, 0, size * sizeof(int));
	char* p = temp;
	for (int i = 0; i < size; ++i)
	{
		p += sprintf(p, "%d%s", (*data)[i], delim);
	}
	HexString re(temp);
	free(temp);
	return re;
}

void Delete(int val, List< int >* data)
{
	unsigned int index = data->GetIndexOf(val);
	while (index < data->Size())
	{
		data->RemoveAtIndex(index);
		index = data->GetIndexOf(val);
	}
}

bool AnyGreater(int val, List< int >* data)
{
	int size = data->Size();
	for (int i = 0; i < size; ++i)
	{
		if ((*data)[i] > val)
		{
			return true;
		}
	}
	return false;
}

bool AnyGreater(int val, List< List< int > >* data)
{
	List< int > temp;
	int size = data->Size();
	for (int i = 0; i < size; ++i)
	{
		int subSize = (*data)[i].Size();
		for (int j = 0; j < subSize; ++j)
		{
			temp.Push((*data)[i][j]);
		}
	}
	bool bl = AnyGreater(0, &temp);
	temp.Clear();
	return bl;
}

ArrayesClass::ArrayesClass(){}

ArrayesClass::~ArrayesClass(){ Clear(); }

ArrayesClass::ArrayesClass(const ArrayesClass&& other)
{
	Clear();
	int size = other.mArrayes.Size();
	for (int i = 0; i < size; ++i)
	{
		mArrayes.Push(new ArrayClass(*other.mArrayes[i]));
	}
}

void ArrayesClass::Initialize()
{
	List< List< int > > data;
	List< int > temp1;
	temp1.Push(1);
	temp1.Push(1);
	temp1.Push(1);
	List< int > temp2;
	temp2.Push(2);
	List< int > temp3;
	temp3.Push(3);

	// [[1,1,1],[1,1,1],[1,1,1],[1,1,1],[2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1, 1, 1], [1, 1, 1], [1, 1, 1], [3], [2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1, 1, 1], [1, 1, 1], [3], [3], [2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1,1,1],[3],[3],[3],[2]]
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[3],[3],[3],[3],[2]]
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);

	// [[1,1,1],[1,1,1],[1,1,1],[2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1,1,1],[1,1,1],[3],[2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1,1,1],[3],[3],[2]]
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[3],[3],[3],[2]]
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);

	// [[1,1,1],[1,1,1],[2]]
	data.Push(temp1);
	data.Push(temp1);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1,1,1],[3],[2]]
	data.Push(temp1);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[3],[3],[2]]
	data.Push(temp3);
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[1,1,1],[2]]
	data.Push(temp1);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[3],[2]]
	data.Push(temp3);
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[2]]
	data.Push(temp2);
	AddArray(&data);
	ArrayClass::Clear(&data);
	// [[2], [2], [2], [2], [2], [2], [2]]
	for (int i = 0; i < 7; ++i)
	{
		data.Push(temp2);
	}
	AddArray(&data);
	ArrayClass::Clear(&data);
	temp1.Clear();
	temp2.Clear();
	temp3.Clear();
}

ArrayesClass& ArrayesClass::operator= (const ArrayesClass& other)
{
	if (this != &other)
	{
		Clear();
		int size = other.mArrayes.Size();
		for (int i = 0; i < size; ++i)
		{
			mArrayes.Push(new ArrayClass(*other.mArrayes[i]));
		}
	}
	return *this;
}

void ArrayesClass::AddArray(const List< List< int > >* data)
{
	ArrayClass* arr = new ArrayClass(data);
	mArrayes.Push(arr);
}

void ArrayesClass::Calculate(HashTable<int, List<int>>* tables)
{
	ArrayClass result;
	int size = mArrayes.Size();
	for (int i = 0; i < size; ++i)
	{
		if (mArrayes[i]->Size() == 7)
		{
			ArrayClass temp = mArrayes[i]->Pattern();
			temp.RemoveNot2();
			result += temp;
		}
		else
		{
			result += mArrayes[i]->Pattern();
		}
	}
	result.Uniqe();
	*this = result.ToArrayes();
	size = mArrayes.Size();
	printf("size: %d \n", size);
	std::ofstream file("a.txt");
	for (int i = 0; i < size; ++i)
	{
		int key = mArrayes[i]->Key();
		List<int> values;
		mArrayes[i]->FindPos(&values);
		tables->Add(key, values);
		char dst[20] = {0};
		int len = sprintf(dst, "0x%x\n", key);
		file << "0x" << hex << key << std::endl;
		values.Clear();
	}
	file.flush();
	file.close();
}

void ArrayesClass::Clear()
{
	int size = mArrayes.Size();
	for (int i = 0; i < size; ++i)
	{
		delete mArrayes[i];
	}
	mArrayes.Clear();
}

int calc_key(ArrayClass arr)
{
	return arr.Key();
}

List<int> find_hai_pos(ArrayClass arr)
{
	List<int> res;
	return res;
}