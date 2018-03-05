#ifndef __DS_QUICKSORT_H__
#define __DS_QUICKSORT_H__

#include "CommonTypeDefines.h"

namespace DataStructures
{

	const int sort_cut_off = 10;
	const int sort_max_level = 32;

	template <class T>
	struct pair
	{
		pair(const T& _first, const T& _second) : first(_first), second(_second) {}
		T first;
		T second;
	};

	template <class Iter, class Ty>
	inline void interator_swap(Iter a, Iter b)
	{
		Ty save = *a;
		*a = *b;
		*b = save;
	}

	template <class Iter, class Ty>
	inline void insertion_sort(Iter first, Iter last)
	{
		// insertion sort [first, last]
		if (first != last)
		{
			Iter next = first;
			do
			{
				Ty val = *++next;
				Iter next1 = next;

				for (; next1 != first && val < *(next1 - 1); next1--)
					*next1 = *(next1 - 1);

				*next1 = val;
			}while (next != last);
		}
	}

	template <class Iter, class Ty>
	inline void shellsort(Iter first, Iter last)
	{
		// shell sort [first, last]
		const int hmax = (int)((last - first + 1) / 9);
		int h;
		for(h = 1; h <= hmax; h = 3*h+1);
		for(; h > 0; h /= 3)
		{
			for(Iter p = first + h; p < last; ++p)
			{
				Ty v = *p;
				Iter p1 = p;
				while( p1 - first >= h && v < *(p1 - h))
				{
					*p1 = *(p1 - h);
					p1 -= h;
				}
				*p1 = v;
			}
		}
	}

	template <class Iter, class Ty>
	inline void med3(Iter first, Iter mid, Iter last)
	{
		// sort median of three elements to middle
		if (*mid < *first)
			interator_swap<Iter, Ty>(mid, first);
		if (*last < *mid)
			interator_swap<Iter, Ty>(last, mid);
		if (*mid < *first)
			interator_swap<Iter, Ty>(mid, first);
	}

	template <class Iter, class Ty>
	inline void median(Iter first, Iter mid, Iter last)
	{
		// sort median element to middle
		if (40 < last - first)
		{
			// median of nine
			int step = (int)((last - first + 1) / 8);
			med3<Iter, Ty>(first, first + step, first + 2 * step);
			med3<Iter, Ty>(mid - step, mid, mid + step);
			med3<Iter, Ty>(last - 2 * step, last - step, last);
			med3<Iter, Ty>(first + step, mid, last - step);
		}
		else
			med3<Iter, Ty>(first, mid, last);
	}

	template <class Iter, class Ty>
	inline pair<Iter> partition(Iter first, Iter last)
	{
		// partition [first, last]
		Iter mid = first + (last - first + 1) / 2; // fort median to mid;
		median<Iter, Ty>(first, mid, last);
		Ty pivot = *mid;
		Iter pfirst = first;
		Iter plast = last - 1;
		Iter gfirst = first;
		Iter glast = plast;

		for (;;)
		{
			while (*++pfirst < pivot);
			while (pivot < *--plast)
			{
				if (plast == pfirst)
					break;
			}

			if (pfirst >= plast)
				break;

			interator_swap<Iter, Ty>(pfirst, plast);

			if (*pfirst == pivot)
				interator_swap<Iter, Ty>(++gfirst,  pfirst);

			if (*plast == pivot)
				interator_swap<Iter, Ty>(--glast, plast);
		}

		interator_swap<Iter, Ty>(pfirst, last - 1);

		plast = pfirst++ - 1;

		for (Iter p = first; p < gfirst; p++, plast--)
			interator_swap<Iter, Ty>(p, plast);

		for (Iter p = last - 1; p > glast; p--, gfirst++)
			interator_swap<Iter, Ty>(p, pfirst);

		return pair<Iter>(pfirst, plast);
	}

	template <class Iter, class Ty>
	inline void quicksort_r(Iter first, Iter last, int ideal)
	{
		// order [first, last]
		int count;
		for(; sort_max_level < (count = (int)(last - first + 1)) && 0 < ideal; )
		{
			// divide and conquer by quicksort
			pair<Iter> mid = partition<Iter, Ty>(first, last);
			ideal /= 2, ideal += ideal / 2; // allow 1.5 log2(N) divisions

			if (mid.first - first <= last - mid.second)
			{
				// loop on second half
				quicksort_r<Iter, Ty>(first, mid.first, ideal);
				first = mid.second;
			}
			else
			{
				// loop on first half
				quicksort_r<Iter, Ty>(mid.second, last, ideal);
				last = mid.first;
			}
		}

		if (sort_max_level < count)
			shellsort<Iter, Ty>(first, last);
		else if (1 < count)
			insertion_sort<Iter, Ty>(first, last);
	}

	template <class Iter, class Ty>
	void QuickSort(Iter first, Iter last, int size)
	{
		if (size <= sort_cut_off)
			insertion_sort<Iter, Ty>(first, last);
		else
			quicksort_r<Iter, Ty>(first, last, size);
	}

}

#endif