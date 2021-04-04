#pragma once

#include "ZippedBuffer.h"

const ulong BLOCK_SIZE_DEFAULT = 1024 * 1024 * 2;  // 2MB



class ZSTREAMAPI ZippedBlockBase {
  friend class ZippedStreamBase;
  friend class ZippedStreamReader;
  friend class ZippedStreamWriter;
  friend class ZippedBlockStack;
protected:
  struct {
    ulong LengthSource;
    ulong LengthCompressed;
    ulong BlockSize;
  }
  Header;

  ulong Position;
  ZippedBuffer Buffer;
  FILE* BaseStream;
  ulong BasePosition;

public:
  ZippedBlockBase( FILE* baseStream, const ulong& position );
  virtual long Tell();
  virtual long Seek( const long& offset, const uint& origin = SEEK_SET );
  virtual bool Compress( const bool& clearSource = true );
  virtual bool Decompress( const bool& clearCompressed = true );
  virtual bool IsCompressed();
  virtual bool IsDecompressed();
  virtual void CommitHeader() = 0;
  virtual void CommitData() = 0;
  virtual void SetBlockSize( const ulong& length ) = 0;
  virtual ulong GetFileSize() = 0;
  virtual ulong Read( byte* buffer, const ulong& length ) = 0;
  virtual ulong Write( byte* buffer, const ulong& length ) = 0;
  virtual bool EndOfBlock() = 0;
  virtual void CacheIn( const ulong& position = -1 ) = 0;
  virtual void CacheOut() = 0;
  virtual bool Cached() = 0;
  virtual ~ZippedBlockBase();
};



class ZSTREAMAPI ZippedBlockReader : public ZippedBlockBase {
public:
  ZippedBlockReader( FILE* baseStream, const ulong& position );
  virtual void CommitHeader();
  virtual void CommitData();
  virtual ulong GetFileSize();
  virtual void SetBlockSize( const ulong& length );
  virtual ulong Read( byte* buffer, const ulong& length );
  virtual ulong Write( byte* buffer, const ulong& length );
  virtual bool EndOfBlock();
  virtual void CacheIn( const ulong& position = -1 );
  virtual void CacheOut();
  virtual bool Cached();
  virtual ~ZippedBlockReader();
};



class ZSTREAMAPI ZippedBlockWriter : public ZippedBlockBase {
public:
  ZippedBlockWriter( FILE* baseStream, const ulong& position = 0 );
  virtual void CommitHeader();
  virtual void CommitData();
  virtual ulong GetFileSize();
  virtual void SetBlockSize( const ulong& length );
  virtual ulong Read( byte* buffer, const ulong& length );
  virtual ulong Write( byte* buffer, const ulong& length );
  virtual bool EndOfBlock();
  virtual void CacheIn( const ulong& position = -1 );
  virtual void CacheOut();
  virtual bool Cached();
};



const ulong STACK_SIZE_DEFAULT = 1024 * 1024 * 20; // 20MB
const ulong STACK_COUNT_MAX    = 1024;



class ZSTREAMAPI ZippedBlockStack {
  ulong CacheSizeMax;
  ulong CacheSize;
  ulong CacheCount;
  ZippedBlockBase* Stack[STACK_COUNT_MAX];
  ZippedBlockStack();

public:
  void StackIn( ZippedBlockBase* block );
  void StackOut( ZippedBlockBase* block );
  void StackOutLast();
  void SetMemoryLimit( const ulong& size );
  static ZippedBlockStack* GetInstance();
};
