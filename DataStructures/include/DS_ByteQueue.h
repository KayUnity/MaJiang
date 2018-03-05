#ifndef __BYTE_QUEUE_H
#define __BYTE_QUEUE_H


/// The namespace DataStructures was only added to avoid compiler errors for commonly named data structures
/// As these data structures are stand-alone, you can use them outside of HexNet for your own projects if you wish.
namespace DataStructures
{
	class ByteQueue
	{
	public:
		ByteQueue();
		~ByteQueue();
		ByteQueue& operator = (const ByteQueue& other);
		void WriteBytes(const char *in, unsigned length);
		bool ReadBytes(char *out, unsigned length, bool peek);
		unsigned GetBytesWritten(void) const;
		void IncrementReadOffset(unsigned length);
		void Clear(void);
		void Print(void);
		void ReadAllBytes(char* dst, int length, bool peek = false);
	protected:
		char *data;
		unsigned readOffset, writeOffset, lengthAllocated;
	};
}

#endif
