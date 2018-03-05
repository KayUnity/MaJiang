#include "singleFileSystem/DS_HugeFile.h"
#include "streams/DS_FileStream.h"
#include "streams/DS_MemoryStream.h"
#include "FormatString.h"
#include "fileUtils/_splitpath.h"
#include "fileUtils/_fileExists.h"

using namespace DataStructures;

HugeFile::HugeFile(HexString fileName, bool readOnly, bool exclusive, bool inMemory, __s64 partitionSize)
    : mFileName(fileName), mReadOnly(readOnly), mExclusive(exclusive), mInMemory(inMemory), mIsFileOpen(false)
{
    mStubSize = 0;
    if (partitionSize != -1)
        mPartitionSize = partitionSize; // specified size
    else
        mPartitionSize = GetMaxFileSize(mFileName);
    
    // used for user data size
    mPartitionDataSize = mPartitionSize - HFP_HEADER_SIZE;
    //File part size is too small. Minimal part size is 100
    assert(mPartitionDataSize >= 100);
    // no files
    mSize = 0;
    mPosition = 0;
}

HugeFile::~HugeFile()
{
    SetInMemory(false);
    Close();
}

void HugeFile::GetFileNoAndOffset(__s64 pos, int &fileNo, __s64 &offset)
{
    fileNo = (int)((pos + mStubSize) / mPartitionDataSize);
    offset = (pos + mStubSize) - fileNo * mPartitionDataSize;
    if (fileNo == 0)
        offset -= mStubSize;
}

void HugeFile::InitPartHeader(HugeFilePartitionHeader &hfPartHeader)
{
    // clear record
    hfPartHeader.partitionSize = 0;
    memset(hfPartHeader.signature, 0, HFP_HEADER_SIZE);
    memset(hfPartHeader.nextFileName, 0, sizeof(hfPartHeader.nextFileName));
    
    // move signature
    memcpy(hfPartHeader.signature, HF_SIGNATURE, HF_SIGNATURE_SIZE);
    // set partition size
    hfPartHeader.partitionSize = mPartitionSize;
}

bool HugeFile::AppendPartFile()
{
    HexString name;
    //--- get name of new file ---
    if (mPartFiles.Size() == 0)
        name = mFileName;
    else
    {
        unsigned int i = mPartFiles.Size();
        while (true)
        {
            name = mFileName + FormatString("%i", i);
            if (!_fileExists(name.C_String()))
                break;
            i++;
        }
    }
    
    //--- create file ---
    unsigned int i = mPartFiles.Size();
    HexString mode;
    if (mReadOnly)
        mode = "rb";
    else
        mode = "wb+";
    Stream *stream;
    if (!mInMemory)
        stream = new FileStream(name.C_String(), mode.C_String());
    else
        stream = new MemoryStream(name.C_String());
    mPartFiles.Insert(stream);
    if (!stream->CanSeek())
        return false;
    // write partition header - no next file
    HugeFilePartitionHeader hfpHeader;
    InitPartHeader(hfpHeader);
    if (i == 0)
        mPartFiles[0]->Seek(mStubSize, SEEK_SET);
    mPartFiles[mPartFiles.Size() - 1]->Write(&hfpHeader, HFP_HEADER_SIZE);
    
    //--- modify header of previous file ---
    if (mPartFiles.Size() != 1)
    {
        // save position
        __s64 pos = mPartFiles[i - 1]->Position();
        //--- prepare header ---
        char drive[16];
        char directory[250];
        char fileName[250];
        char extension[32];
        _splitpath(name.C_String(), drive, directory, fileName, extension);
        HexString s = fileName;
        s += extension;
        strcpy(hfpHeader.nextFileName, s.C_String());
        // write header
        if (i - 1 == 0)
            mPartFiles[i - 1]->Seek(mStubSize, SEEK_SET);
        else
            mPartFiles[i - 1]->Seek(0, SEEK_SET);
        mPartFiles[i - 1]->Write(&hfpHeader, HFP_HEADER_SIZE);
        // restore position
        mPartFiles[i - 1]->Seek(pos, SEEK_SET);
    }
    return true;
}

void HugeFile::DeleteLastPartFile()
{
    //No parts to delete
    assert(mPartFiles.Size() > 1);
    
    int i = mPartFiles.Size() - 1;
    HexString name = mPartFiles[i]->GetFileName();
    // close file and free object
    delete mPartFiles[i];
    mPartFiles.RemoveAtIndex(i);
    // delete file
    if (!mInMemory)
    {
		remove(name.C_String());
    }
    //--- modify header of previous file ---
    // save position
    __s64 pos = mPartFiles[i - 1]->Position();
    // write partition header - no next file
    HugeFilePartitionHeader hfpHeader;
    InitPartHeader(hfpHeader);
    if (i - 1 == 0)
        mPartFiles[i - 1]->Seek(mStubSize, SEEK_SET);
    else
        mPartFiles[i - 1]->Seek(0, SEEK_SET);
    mPartFiles[i - 1]->Write(&hfpHeader, HFP_HEADER_SIZE);
    // restore position
    mPartFiles[i - 1]->Seek(pos, SEEK_SET);
}

bool HugeFile::OpenFilesChain(const HexString fileName)
{
    char drive[16];
    char directory[250];
    char filename[250];
    char extension[32];
    _splitpath(fileName.C_String(), drive, directory, filename, extension);
    
    HexString curFolder = drive;
    curFolder += directory;
    HexString curFileName = filename;
    curFileName += extension;
    bool result = true;
    do
    {
        //--- open file ---
        unsigned int i = mPartFiles.Size();
        Stream *stream;
        HexString mode;
        if (mReadOnly)
            mode = "rb";
        else
            mode = "rb+";
        HexString newFileName = curFolder + curFileName;
        if (mInMemory)
            stream = new MemoryStream(newFileName.C_String());
        else
            stream = new FileStream(newFileName.C_String(), mode.C_String());
        mPartFiles.Insert(stream);
        // open disk file and copy it to in-memory file?
        if (!stream->CanSeek())
        {
            // file is corrupted
            result = false;
            // free its object
            delete mPartFiles[i];
            // and remove it from list
            mPartFiles.RemoveAtIndex(i);
            break;
        }
        // calculate stub size
        if (i == 0)
        {
            CalculateStubSize(mPartFiles[i]);
            mPartFiles[i]->Seek(mStubSize, SEEK_SET);
        }
        
        // read header
        HugeFilePartitionHeader hfpHeader;
        mPartFiles[i]->Read(&hfpHeader, HFP_HEADER_SIZE);
        // get next file name
        curFileName = hfpHeader.nextFileName;
        // init partition size
        if (i == 0)
        {
            mPartitionSize = hfpHeader.partitionSize;
            mPartitionDataSize = mPartitionSize - HFP_HEADER_SIZE;
        }
        // check signature and size
        if ((memcmp(hfpHeader.signature, HF_SIGNATURE, 4) != 0) || (mPartFiles[i]->Length() < HFP_HEADER_SIZE))
        {
            // file is corrupted
            result = false;
            // free its object
            delete mPartFiles[i];
            // and remove it from list
            mPartFiles.RemoveAtIndex(i);
            break;
        }
        // truncated file in chain?
        if ((mPartFiles[i]->Length() < HFP_HEADER_SIZE) && (curFileName.GetLength() > 0))
        {
            // file is truncated
            result = false;
            break;
        }
    }
    while (curFileName.GetLength() > 0);
    return result;
}

bool HugeFile::InternalOpen(const HexString fileName, bool create, bool ignoreErrors)
{
    assert(!mIsFileOpen);
    bool result;
    
    // if memory files are still kept open - just set flags
    if (mInMemory && (mPartFiles.Size() > 0) && !create)
        result = true;
    else
        if (mPartFiles.Size() != 0)
            assert(false);
        else
            // create file?
            if (create)
            {
                // if newly created file - delete current one
                DeleteFile();
                result = AppendPartFile(); // create file
                mSize = 0;
            }
            else
            {
                // open files chain
                if (OpenFilesChain(fileName) || ignoreErrors)
                {
                    if (mPartFiles.Size() > 0)
                    {
                        mSize = 0;
                        for (unsigned int i=0; i<mPartFiles.Size(); i++)
                            mSize = mSize + mPartFiles[i]->Length() - HFP_HEADER_SIZE;
                        mSize -= mStubSize;
                        result = true;
                    }
                    else
                        result = false;
                }
                else
                {
                    // corrupted file
                    result = false;
                }
            }
    mPosition = 0;
    mIsFileOpen = result;
    return result;
}

void HugeFile::SetExclusive(bool value)
{
    mExclusive = value;
}

void HugeFile::SetInMemory(bool value)
{
    if (value == mInMemory)
        return;
    // exchange data between disk and memory files
    if (mPartFiles.Size() > 0)
    {
        // part files
        for (unsigned int i=0; i<mPartFiles.Size(); i++)
        {
            // part file name
            HexString s = mPartFiles[i]->GetFileName();
            // create new file
            Stream *stream;
            HexString mode = "rwb";
            if (mReadOnly)
                mode = "rb";
            if (value)
                stream = new MemoryStream(s.C_String());
            else
                stream = new FileStream(s.C_String(), mode.C_String());
            // copy old file to new one
            mPartFiles[i]->Seek(0, SEEK_SET);
            stream->CopyFrom(mPartFiles[i], 0);
            // set position
            stream->Seek(mPartFiles[i]->Position(), SEEK_SET);
            // free old file
            delete mPartFiles[i];
            // set element to the new file
            mPartFiles[i] = stream;
        }
    }
    mInMemory = value;
}

__s64 HugeFile::GetPosition()
{
    return Seek(0, SEEK_CUR);
}

void HugeFile::SetPosition(__s64 pos)
{
    Seek(pos, SEEK_SET);
}

void HugeFile::SetReadOnly(bool value)
{
    mReadOnly = value;
}

void HugeFile::SetSize(__s64 value)
{
    int fileNo;
    __s64 offset;
    // increase size?
    if (value > mSize)
    {
        // get reqired No of part file
        GetFileNoAndOffset(value, fileNo, offset);
        // append required number of part files
        while ((int)mPartFiles.Size()-1 < fileNo)
        {
            // set size of part file
            mPartFiles[mPartFiles.Size()-1]->SetLength(mPartitionSize);
            AppendPartFile();
        }
        // set size of last part file
        if (fileNo == 0)
            mPartFiles[fileNo]->SetLength(offset + HFP_HEADER_SIZE + mStubSize);
        else
            mPartFiles[fileNo]->SetLength(offset + HFP_HEADER_SIZE);
    }
    else
        if (value < mSize)
        {
            // get reqired No of part file
            GetFileNoAndOffset(value, fileNo, offset);
            // delete required number of part files
            while ((int)mPartFiles.Size()-1 > fileNo)
                DeleteLastPartFile();
            // set size of last part file
            if (fileNo == 0)
                mPartFiles[fileNo]->SetLength(offset + HFP_HEADER_SIZE + mStubSize);
            else
                mPartFiles[fileNo]->SetLength(offset + HFP_HEADER_SIZE);
        }
    mSize = value;
    mPosition = value;
}

__s64 HugeFile::GetSize()
{
  return mSize;
}

void HugeFile::CalculateStubSize(Stream *file)
{
    mStubSize = 0;
    char *buf = (char *)malloc(0xffff);
    __s64 offset = 0;
    file->Seek(offset, SEEK_SET);
    char sgn[5];
    strcpy(sgn, HF_SIGNATURE);
    // find local file header for first file in archive
    while (file->Position() < file->Length())
    {
        __s64 pos = file->Position();
        size_t size = file->Read(buf, 0xffff);
        // find local file header signature
        int i = 0;
        int k = -1;
        while (i < (int)size)
        {
            k = -1;
            if (buf[+2] == sgn[0])
            {
                int j = 1;
                while (j <= 3)
                {
                    if ((i + j) >= (int)size)
                        break;
                    if (buf[i + j] != sgn[j])
                        break;
                    j ++;
                    // signature found
                    if (j > 3)
                        k = i;
                }
                // chaeck signature
                if (k >= 0)
                {
                    bool checkFile = false;
                    size_t oldPos = file->Position();
                    file->Seek(pos + k, SEEK_SET);
                    if (file->Length() - file->Position() >= HFP_HEADER_SIZE + HF_SIGNATURE_SIZE)
                    {
                        HugeFilePartitionHeader hfpHeader;
                        file->Read(&hfpHeader, HFP_HEADER_SIZE);
                        // check huge file signature
                        if (memcmp(hfpHeader.signature, HF_SIGNATURE, HF_SIGNATURE_SIZE) == 0)
                        {
                            char name[8];
                            // check SFS signature
                            file->Read(name, HFP_HEADER_SIZE);
                            if (memcmp(name, SFS_SIGNATURE, HF_SIGNATURE_SIZE) == 0)
                                checkFile = true;
                        }
                    }
                    file->Seek(oldPos, SEEK_SET);
                    if (checkFile)
                        // local file header for first file in archive found!
                        break;
                    else
                        k = -1;
                }
            }
            i ++;
        }   // end of searching local header
        // stub size = difference between supposed offset for header
        // and real one
        if (k >= 0)
        {
            mStubSize = k + pos - offset;
            break;
        }
    }
    file->Seek(offset, SEEK_SET);
    free(buf);
}

__s64 HugeFile::CopyFrom(HugeFile *source, __s64 count)
{
    __s64 oldPos = GetPosition();
    __s64 oldPos1 = source->GetPosition();
    __s64 sourceSize;
    if (count <= 0)
    {
        source->SetPosition(0);
        sourceSize = source->GetSize();
    }
    else
        sourceSize = source->GetPosition() + count;
    __s64 result = 0;
    SetSize(0);
    SetPosition(0);
    char *buf = (char *)malloc(FILE_BLOCK_SIZE);
    while (source->GetPosition() < sourceSize)
    {
        size_t outSize;
        if (sourceSize - source->GetPosition() > FILE_BLOCK_SIZE)
            outSize = FILE_BLOCK_SIZE;
        else
            outSize = (size_t)(sourceSize - source->GetPosition());
        source->Read(buf, outSize);
        Write(buf, outSize);
        result += outSize;
    }
    free(buf);
    SetPosition(oldPos);
    source->SetPosition(oldPos1);
    
    return result;
}

void HugeFile::SaveToStream(Stream *stream)
{
    __s64 oldPos = GetPosition();
    __s64 oldPos1 = stream->Position();
    HugeFilePartitionHeader hfpHeader;
    InitPartHeader(hfpHeader);
    stream->Write(&hfpHeader, HFP_HEADER_SIZE);
    SetPosition(0);
    int outBytes = 0;
    __s64 inSize = GetSize();
    char *buf = (char *)malloc(FILE_BLOCK_SIZE);
    while (outBytes < inSize)
    {
        size_t outSize;
        if (inSize - outBytes > FILE_BLOCK_SIZE)
            outSize = FILE_BLOCK_SIZE;
        else
            outSize = (size_t)(GetSize() - outBytes);
        ReadBuffer(buf, outSize);
        stream->Write(buf, outSize);
        outBytes += outSize;
    }
    free(buf);
    SetPosition(oldPos);
    stream->Seek(oldPos1, SEEK_SET);
}

void HugeFile::LoadFromStream(Stream *stream)
{
    __s64 oldPos = GetPosition();
    __s64 oldPos1 = stream->Position();
    stream->Seek(0, SEEK_SET);
    HugeFilePartitionHeader hfpHeader;
    stream->Read(&hfpHeader, HFP_HEADER_SIZE);
    //header corrupted or this is not SFS file
    if (strncmp(hfpHeader.signature, HF_SIGNATURE, HF_SIGNATURE_SIZE) != 0)
        assert(false);
    SetSize(0);
    SetPosition(0);
    // Stream.Position := HFPHeaderSize;
    char *buf = (char *)malloc(FILE_BLOCK_SIZE);
    while (stream->Position() < stream->Length())
    {
        size_t outSize;
        if (stream->Length() - stream->Position() > FILE_BLOCK_SIZE)
            outSize = FILE_BLOCK_SIZE;
        else
            outSize = (size_t)(stream->Length() - stream->Position());
        stream->Read(buf, outSize);
        WriteBuffer(buf, outSize);
    }
    free(buf);
    SetPosition(oldPos);
    stream->Seek(oldPos1, SEEK_SET);
}

void HugeFile::Close()
{
    // keep in-memory files open
    if (! mInMemory)
    {
        for (unsigned int i=0; i<mPartFiles.Size(); i++)
            delete mPartFiles[i];
        mPartFiles.Clear();
        mSize = 0;
    }
    mPosition = 0;
    mIsFileOpen = false;
}

void HugeFile::FlushBuffers()
{
    for (unsigned int i=0; i<mPartFiles.Size(); i++)
    {
        //roc todo
        //mPartFiles[i]->FlushBuffers();
    }
}

bool HugeFile::Open(bool create, bool ignoreErrors)
{
    return InternalOpen(mFileName, create, ignoreErrors);
}

size_t HugeFile::Read(void *buffer, size_t count)
{
    long int readSize = 0;
    // increase file size if required
    //Stream read error
    assert(mPosition <= mSize);
    // read
    while (readSize < (long int)count)
    {
        int fileNo;
        __s64 offset;
        // get part file No and offset there
        GetFileNoAndOffset(mPosition, fileNo, offset);
        // data rest in last part file
        __s64 restSize = mPartitionDataSize - offset;
        if (fileNo == 0)
            restSize -= mStubSize;
        // get size of data portion to read
        int portionSize;
        if (restSize < count - readSize)
            portionSize = (int)restSize;
        else
            portionSize = (int)(count - readSize);
        
        // if not first iteration - then make seek to part BOF
        if (readSize > 0)
            mPartFiles[fileNo]->Seek(HFP_HEADER_SIZE, SEEK_SET);
        // read portion
        size_t sz = mPartFiles[fileNo]->Read(((char *)buffer + readSize), portionSize);
        readSize += sz;
        mPosition += sz;
        if (sz != portionSize)
            break;
    }
    return readSize;
}

void HugeFile::ReadBuffer(void *buffer, size_t count)
{
    if ((count != 0) && (Read(buffer, count) != count))
        //Stream read error
        assert(false);
}

__s64 HugeFile::Seek(__s64 offset, int origin)
{
    // calc global position
    switch (origin)
    {
        case SEEK_SET:
            assert(offset >= 0);
            mPosition = offset;
            break;
        case SEEK_CUR:
            mPosition = mPosition + offset;
            break;
        case SEEK_END:
            mPosition = mSize + offset;
            break;
        default:
            assert(false);
    }
    int fileNo;
    __s64 fileOffset;
    // get corresponding FileNo and FileOffset
    GetFileNoAndOffset(mPosition, fileNo, fileOffset);
    // if possible - do physical seek
    if (mPosition <= mSize)
    {
        if (fileNo == 0)
            mPartFiles[fileNo]->Seek(mStubSize + fileOffset + HFP_HEADER_SIZE, SEEK_SET);
        else
            mPartFiles[fileNo]->Seek(fileOffset + HFP_HEADER_SIZE, SEEK_SET);
    }
    // return new position
    return mPosition;
}

size_t HugeFile::Write(const void *buffer, size_t count)
{
    size_t writtenSize = 0;
    if (count <= 0)
        return 0;
    // write beyond end of the file
    if (mPosition > mSize)
    {
        __s64 offset = mPosition;
        SetPosition(0);
        SetSize(offset);
        SetPosition(offset);
        if (GetPosition() != offset)
            return 0;
    }
    // write
    while (writtenSize < count)
    {
        int fileNo;
        __s64 offset;
        // get part file No and offset there
        GetFileNoAndOffset(mPosition, fileNo, offset);
        // if necessary append another part file
        if (fileNo >= (int)mPartFiles.Size())
            AppendPartFile();
        else
            // if not first iteration - then make seek to part BOF
            if (writtenSize > 0)
                mPartFiles[fileNo]->Seek(HFP_HEADER_SIZE, SEEK_SET);
        // free rest in last part file
        size_t restSize = (size_t)(mPartitionDataSize - offset);
        // get size of data portion to write
        size_t portionSize;
        if (restSize < count - writtenSize)
            portionSize = restSize;
        else
            portionSize = count - writtenSize;
        // write portion
        size_t sz = mPartFiles[fileNo]->Write((char *)buffer + writtenSize, portionSize);
        writtenSize += sz;
        mPosition += sz;
        // update file size variable
        if (mPosition > mSize)
            mSize = mPosition;
        if (sz != portionSize)
            break;
    }
    return writtenSize;
}

void HugeFile::WriteBuffer(const void *buffer, size_t count)
{
    if ((count != 0) && (Write(buffer, count) != count))
    //WriteBuffer - Stream write error
        assert(false);
}

bool HugeFile::CopyFile(const HexString newName)
{
    bool result = true;
    // extract file name w/o path
    char drive[32];
    char folder[255];
    char filename[255];
    char extension[32];
    _splitpath(mFileName.C_String(), drive, folder, filename, extension);
    HexString pureFileName = filename;
    pureFileName += extension;
    HexString existingPath = drive;
    existingPath += folder;
    _splitpath(newName.C_String(), drive, folder, filename, extension);
    HexString newFileName = filename;
    newFileName += extension;
    HexString newPath = drive;
    newPath += folder;
    // copy first part file
    // in-memory file?
    if (mInMemory)
    {
        if (mPartFiles.Size() != 1)
            //Cannot copy memory SFS file.
            assert(false);
        HugeFile *hf = new HugeFile(newName, false, true, false);
        hf->Open(true);
        __s64 i = GetPosition();
        SetPosition(0);
        hf->CopyFrom(this, mSize);
        hf->Close();
        delete hf;
        SetPosition(i);
        return true;
    }
    else
    {
        Stream *src = new FileStream(mFileName.C_String(), "rb");
        Stream *dst = new FileStream(newName.C_String(), "rwb+");
        assert(src->CanSeek() && dst->CanSeek());
        dst->CopyFrom(src, 0);
        delete src;
        delete dst;
    }
    // process parts
    HexString curFileName = newName;
    int i = 0;
    while (true)
    {
        FileStream *partFile = new FileStream(curFileName.C_String(), "rb");
        if (!partFile->CanSeek())
        {
            delete partFile;
            result = false;
            break;
        }
        // read header
        if (i == 0)
            partFile->Seek(mStubSize, SEEK_SET);
        else
            partFile->Seek(0, SEEK_SET);
        HugeFilePartitionHeader hfpHeader;
        partFile->Read(&hfpHeader, HFP_HEADER_SIZE);
        // rename file
        HexString s1 = hfpHeader.nextFileName;
        // last file in chain?
        if (s1 == "")
        {
            delete partFile;
            break;
        }
        HexString s = s1;
        int p = s.FindIString(pureFileName);
        if (p < 0)
        {
            delete partFile;
            result = false;
            break;
        }
        s.Replace(p, (unsigned)pureFileName.GetLength(), newFileName.C_String());
        
        Stream *src = new FileStream((existingPath + s1).C_String(), "rb");
        Stream *dst = new FileStream((newPath + s).C_String(), "rwb+");
        assert(src->CanSeek() && dst->CanSeek());
        bool succ = dst->CopyFrom(src, 0) == src->Length();
        delete src;
        delete dst;
        
        if (!succ)
        {
            result = false;
            delete partFile;
            break;
        }
        // update header
        strcpy(hfpHeader.nextFileName, s.C_String());
        // write header
        if (i == 0)
            partFile->Seek(mStubSize, SEEK_SET);
        else
            partFile->Seek(0, SEEK_SET);
        if (partFile->Write(&hfpHeader, HFP_HEADER_SIZE) != HFP_HEADER_SIZE)
        {
            result = false;
            delete partFile;
            break;
        }
        // close file
        delete partFile;
        curFileName = newPath + hfpHeader.nextFileName;
        i ++;
    }
    return result;
}

bool HugeFile::RenameFile(const HexString newName)
{
	// extract file name w/o path
	char drive[32];
	char folder[255];
	char filename[255];
	char extension[32];
	_splitpath(mFileName.C_String(), drive, folder, filename, extension);
	HexString pureFileName = filename;
	pureFileName += extension;
	HexString existingPath = drive;
	existingPath += folder;
	_splitpath(newName.C_String(), drive, folder, filename, extension);
	HexString newFileName = filename;
	newFileName += extension;
	HexString newPath = drive;
	newPath += folder;

	// rename first part file
	// in-memory file?
	if (mInMemory)
	{
		return false;
	}

	if (rename(mFileName, newPath+newFileName) != 0)
		return false;

	// process parts
	HexString curFileName = newName;
	int i = 0;
	while (true)
	{
		FileStream *partFile = new FileStream(curFileName.C_String(), "rb");
		if (!partFile->CanSeek())
		{
			delete partFile;
			return false;
		}
		// read header
		if (i == 0)
			partFile->Seek(mStubSize, SEEK_SET);
		else
			partFile->Seek(0, SEEK_SET);
		HugeFilePartitionHeader hfpHeader;
		partFile->Read(&hfpHeader, HFP_HEADER_SIZE);
		// rename file
		HexString s1 = hfpHeader.nextFileName;
		// last file in chain?
		if (s1 == "")
		{
			delete partFile;
			break;
		}
		HexString s = s1;
		int p = s.FindIString(pureFileName);
		if (p < 0)
		{
			delete partFile;
			return false;
		}
		s.Replace(p, (unsigned)pureFileName.GetLength(), newFileName.C_String());

		if (rename(existingPath + s1, newPath + s) != 0)
		{
			delete partFile;
			return false;
		}
		// update header
		strcpy(hfpHeader.nextFileName, s.C_String());
		// write header
		if (i == 0)
			partFile->Seek(mStubSize, SEEK_SET);
		else
			partFile->Seek(0, SEEK_SET);
		if (partFile->Write(&hfpHeader, HFP_HEADER_SIZE) != HFP_HEADER_SIZE)
		{
			delete partFile;
			return false;
		}
		// close file
		delete partFile;
		curFileName = newPath + hfpHeader.nextFileName;
		i++;
	}

	mFileName = newName;
    return true;
}

bool HugeFile::DeleteFile()
{
    if (mInMemory)
    {
        for (unsigned int i=0; i<mPartFiles.Size(); i++)
            delete mPartFiles[i];
        mPartFiles.Clear();
        mSize = 0;
        return true;
    }
    else
    {
        bool result = true;
        char drive[32];
        char folder[255];
        char filename[255];
        char extension[32];
        _splitpath(mFileName.C_String(), drive, folder, filename, extension);
        HexString path = drive;
        path += folder;
        HexString curFileName = mFileName;
        // process parts
        int i = 0;
        while (true)
        {
            Stream *partFile = new FileStream(curFileName.C_String(), "rb");
            if (!partFile->CanSeek())
            {
                delete partFile;
                result = false;
                break;
            }
            // read header
            if (i == 0)
                partFile->Seek(mStubSize, SEEK_SET);
            else
                partFile->Seek(0, SEEK_SET);
            HugeFilePartitionHeader hfpHeader;
            partFile->Read(&hfpHeader, HFP_HEADER_SIZE);
            HexString s1 = hfpHeader.nextFileName;
            // close file
            delete partFile;
            if (remove(curFileName.C_String()) != 0)
            {
                result = false;
                break;
            }
            // last file in chain?
            if (s1 == "")
                break;
            curFileName = path + hfpHeader.nextFileName;
            i ++;
        }
        return result;
    }
}

__s64 DataStructures::GetMaxFileSize(HexString fileName)
{
    //roc todo, now just return 2GB, to more calculation later
    return 0x7fffffff;
    /*
     type
     TOSType = (osUnknown, osWin95, osWin98, osWinME, osWinNT351, osWinNT4, osWin2K, osWinXP, osWinNET );
     const
     TwoGigabytes   = $7FFFFFFF;
     FourGigabytes  = $FFFFFFFF;
     MaxHDFloppy    = $00163E00;
     MaxInt64Value  = $7FFFFFFFFFFFFFFE ; // 2^64
     var
     OsType: TOSType;
     OSVersion        : TOSVersionInfo;
     MaxFileNameLen   : DWord;
     FileSysFlags     : Dword;
     FileSysName      : array[0..MAX_PATH - 1] of AnsiChar;
     VolumeName       : array[0..MAX_PATH - 1] of AnsiChar;
     FileDrive        : string;
     MaxFileSize      : Int64;
     Size1,Size2,Size3: Int64;
     begin
     // Get OS Version
     FillChar(OSVersion, SizeOf(TOSVersionInfo), 0);
     OSVersion.dwOSVersionInfoSize := SizeOf(TOSVersionInfo);
     GetVersionEx(OSVersion);
     
     // Set version to Unknown for default
     OsType := osUnknown;
     
     case OSVersion.dwMajorVersion of
     3: OsType := osWinNT351;
     4: case OSVersion.dwMinorVersion of
     0:   if OSVersion.dwPlatformId = 1 then OsType := osWin95
     else OsType := osWinNT4;
     10:  OsType := osWin98;
     90:  OsType := osWinME;
     end;
     5: case OSVersion.dwMinorVersion of
     0:   OsType := osWin2K;
     1:   OsType := osWinXP;
     2:   OsType := osWinNET;
     end;
     end;
     if OsType = osUnknown then
     raise Exception.Create('Can''t Identify OS version');
     
     FileDrive := ExtractFileDrive(FileName);
     FileDrive := FileDrive + '\';
     MaxFileSize := 0;
     if GetVolumeInformation(PChar(FileDrive), VolumeName, Length(VolumeName),
     nil, Maxfilenamelen, FileSysFlags, FileSysName, SizeOf(FileSysName)) then
     begin
     if FileSysName = 'FAT32' then
     if OsType in [osWin2K, osWinXP, osWinNET] then
     MaxFileSize := FourGigabytes // Win2K max FAT32 file size = 4GB
     else
     MaxFileSize := TwoGigabytes  // Win95/98 max FAT32 file size = 2GB
     else if FileSysName = 'NTFS' then
     MaxFileSize := MaxInt64Value // NTFS max file size = 2^64
     else if FileSysName = 'FAT16' then // NT max FAT16 partition = 4GB; Max File Size = 2GB
     MaxFileSize := TwoGigabytes
     else if FileSysName = 'CDFS' then // Can't write to a CD-ROM drive
     MaxFileSize := 0
     else if FileSysName = 'FS_UDF' then // Rewriteble
     begin
     GetDiskFreeSpaceEx(PChar(FileDrive), Size1,Size2,@Size3);
     MaxFileSize := Size1;  // Set Max File Size to Free Size on device
     end
     else if FileSysName = 'FAT' then
     if (FileDrive = 'A:\') or (FileDrive = 'B:\') then
     MaxFileSize := MaxHDFloppy
     else
     MaxFileSize := TwoGigabytes
     end;
     Result := MaxFileSize;
     //ShowMessage('OS:' + IntToStr(integer(OsType)) + ' FS: ' + FileSysName + ' MaxFileSize: ' + IntToStr(MaxFileSize));
     end;
     */
}

