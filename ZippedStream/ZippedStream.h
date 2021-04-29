#pragma once

EXTERN_C {
extern ZSTREAMAPI ulong ZIPPED_THREADS_COUNT;
extern ZSTREAMAPI ulong BLOCK_SIZE_DEFAULT;
extern ZSTREAMAPI ulong CACHE_READER_SIZE_DEFAULT;
extern ZSTREAMAPI ulong CACHE_READER_STACK_COUNT_MAX;
}

#include "ZippedStreamException.h"
#include "ZippedStreamBlock.h"



class ZSTREAMAPI ZippedStreamBase {
protected:
  struct {
    ulong Length;
    ulong BlockSize;
    uint BlocksCount;
  }
  Header;
  long Position;
  ZippedBlockBase** Blocks;
  FILE* BaseStream;
  long BasePosition;

public:
  ZippedStreamBase( FILE* baseStream, long position = 0 );
  virtual long Tell();
  virtual long Seek( const long& offset, const uint& origin = SEEK_SET );
  virtual bool Compress( const bool& clearSource = true );
  virtual bool Decompress( const bool& clearCompressed = true );
  virtual void SetBlockSize( const ulong& length );
  virtual ulong GetBlockSize();
  virtual void Close( const bool& closeBaseStream = true );
  virtual ulong GetStreamSize();
  virtual void CommitHeader() = 0;
  virtual void CommitData() = 0;
  virtual ulong Read( byte* buffer, const ulong& length ) = 0;
  virtual ulong Write( byte* buffer, const ulong& length ) = 0;
  virtual bool EndOfFile() = 0;
  virtual ~ZippedStreamBase();
};



class ZSTREAMAPI ZippedStreamReader : public ZippedStreamBase {
protected:
  ZippedBlockBase* GetBlockToRead();

public:
  ZippedStreamReader( FILE* baseStream, long position = 0 );
  virtual void CommitHeader();
  virtual void CommitData();
  virtual ulong Read( byte* buffer, const ulong& length );
  virtual ulong Write( byte* buffer, const ulong& length );
  virtual bool EndOfFile();
};



class ZSTREAMAPI ZippedStreamWriter : public ZippedStreamBase {
  ZippedBlockBase* GetBlockToWrite();
  ulong LengthCompressed;
  void FlushBlock( ZippedBlockBase* block );
public:
  ZippedStreamWriter( FILE* baseStream, long position = 0 );
  virtual long Seek( const long& offset, const uint& origin = SEEK_SET );
  virtual void CommitHeader();
  virtual void CommitData();
  virtual ulong Read( byte* buffer, const ulong& length );
  virtual ulong Write( byte* buffer, const ulong& length );
  virtual bool EndOfFile();
  virtual void Flush();
  virtual ~ZippedStreamWriter();
};
