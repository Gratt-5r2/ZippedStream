#include "ZippedAfx.h"
#include <iostream>
using std::string;
using std::cout;
using std::endl;
using std::cin;


#pragma region base
ZippedBlockBase::ZippedBlockBase( FILE* baseStream, const ulong& position ) : Buffer( BLOCK_SIZE_DEFAULT ) {
  BaseStream = baseStream;
  BasePosition = position;
  Position = 0;
  Header.BlockSize = BLOCK_SIZE_DEFAULT;
  Header.LengthSource = 0;
  Header.LengthCompressed = 0;
}

long ZippedBlockBase::Tell() {
  return Position;
}

long ZippedBlockBase::Seek( const long& offset, const uint& origin ) {
  switch( origin ) {
    case SEEK_SET: Position = offset; break;
    case SEEK_CUR: Position += offset; break;
    case SEEK_END: Position = Header.LengthSource - offset - 1; break;
  }

  return Position;
}

bool ZippedBlockBase::Compress( const bool& clearSource ) {
  if( !IsCompressed() ) {
    if( Buffer.Source.GetLength() == 0 )
      return false;

    Buffer.Compress();
    Header.LengthCompressed = Buffer.Compressed.GetLength();
  }

  if( clearSource )
    Buffer.Source.Clear();

  return Header.LengthCompressed > 0;
}

bool ZippedBlockBase::Decompress( const bool& clearCompressed ) {
  if( !IsDecompressed() ) {
    if( Buffer.Compressed.GetLength() == 0 )
      return false;

    Buffer.Decompress( false );
    Header.LengthSource = Buffer.Source.GetLength();
  }

  if( clearCompressed )
    Buffer.Compressed.Clear();

  return Header.LengthSource > 0;
}

bool ZippedBlockBase::IsCompressed() {
  return Buffer.IsCompressed();
}

bool ZippedBlockBase::IsDecompressed() {
  return Buffer.IsDecompressed();
}

ZippedBlockBase::~ZippedBlockBase() {
  // pass
}
#pragma endregion



#pragma region reader
ZippedBlockReader::ZippedBlockReader( FILE* baseStream, const ulong& position ) : ZippedBlockBase( baseStream, position ) {
  IsCached = false;
  CommitHeader();
}

bool ZippedBlockReader::Decompress( const bool& clearCompressed ) {
  if( !IsDecompressed() )
    Buffer.Decompress( true );

  return true;
}

void ZippedBlockReader::CommitHeader() {
  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition, SEEK_SET );
  fread( &Header, 1, sizeof( Header ), BaseStream );
  Buffer.LengthMax = Header.LengthSource;
  fseek( BaseStream, returnPosition, SEEK_SET );
}

void ZippedBlockReader::CommitData() {
  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition + sizeof( Header ), SEEK_SET );
  ulong size = Header.LengthCompressed;
  byte* data = new byte[size];
  fread( data, 1, size, BaseStream );
  Buffer.Compressed.SetBuffer( data, size );
  fseek( BaseStream, returnPosition, SEEK_SET );
}

void ZippedBlockReader::SetBlockSize( const ulong& length ) {
  throw std::exception( "Can not change a block size in the read-only object." );
}

ulong ZippedBlockReader::GetFileSize() {
  return sizeof( Header ) + Header.LengthCompressed;
}

ulong ZippedBlockReader::Read( byte* buffer, const ulong& length ) {
  CacheIn();

  ulong memLeft = Header.LengthSource - Position;
  ulong toRead = min( length, memLeft );
  if( toRead == 0 )
    return 0;

  Buffer.WaitForDecompress();
  memcpy( buffer, Buffer.Source.GetBuffer() + Position, toRead );
  Position += toRead;
  return toRead;
}

ulong ZippedBlockReader::Write( byte* buffer, const ulong& length ) {
  throw std::exception( "Can not write a zipped block in the read-only object." );
}

bool ZippedBlockReader::EndOfBlock() {
  return Position >= Header.LengthSource;
}

bool ZippedBlockReader::CacheIn( const ulong& position ) {
  return ZippedBlockReaderCache::GetInstance()->CacheIn( this );
}

void ZippedBlockReader::CacheOut() {
  if( IsCached ) {
    ZippedBlockReaderCache::GetInstance()->CacheInvalidate( this );
    Buffer.Clear();
  }
  IsCached = false;
}

bool ZippedBlockReader::Cached() {
  return IsCached;
}

ZippedBlockReader::~ZippedBlockReader() {
  CacheOut();
}
#pragma endregion



#pragma region writer
ZippedBlockWriter::ZippedBlockWriter( FILE* baseStream, const ulong& position ) : ZippedBlockBase( baseStream, position ) {
  
}

void ZippedBlockWriter::CommitHeader() {
  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition, SEEK_SET );
  fwrite( &Header, 1, sizeof( Header ), BaseStream );
  fseek( BaseStream, returnPosition, SEEK_SET );
}

void ZippedBlockWriter::CommitData() {
  if( Buffer.Compressed.GetLength() == 0 )
    return;

  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition + sizeof( Header ), SEEK_SET );
  fwrite( Buffer.Compressed.GetBuffer(), 1, Buffer.Compressed.GetLength(), BaseStream );
  fseek( BaseStream, returnPosition, SEEK_SET );
}

ulong ZippedBlockWriter::GetFileSize() {
  return Header.LengthSource;
}

void ZippedBlockWriter::SetBlockSize( const ulong& length ) {
  ZIPASSERT( Header.LengthCompressed == 0, "Can not change a block size after compression." );
  Header.BlockSize = length;
  Buffer.LengthMax = length;
}

ulong ZippedBlockWriter::Read( byte* buffer, const ulong& length ) {
  throw std::exception( "Can not read a zipped block in the write-only object." );
}

ulong ZippedBlockWriter::Write( byte* buffer, const ulong& length ) {
  ulong writed = Buffer.Source.Write( buffer, length );
  Header.LengthSource += writed;
  return writed;
}

bool ZippedBlockWriter::EndOfBlock() {
  return Position >= Header.BlockSize;
}

bool ZippedBlockWriter::CacheIn( const ulong& position ) {
  ZIPASSERT( Buffer.Compressed.GetLength() > 0, "Buffer must be compressed before caching." );
  if( position != -1 )
    BasePosition = position;

  CommitHeader();
  CommitData();
  Buffer.Clear();
  return true;
}

void ZippedBlockWriter::CacheOut() {
  ulong bufferSize = Header.LengthCompressed;
  byte* buffer = new byte[bufferSize];
  Buffer.Compressed.SetBuffer( buffer, bufferSize );

  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition + sizeof( Header ), SEEK_SET );
  fread( Buffer.Compressed.GetBuffer(), 1, Buffer.Compressed.GetLength(), BaseStream );
  fseek( BaseStream, returnPosition, SEEK_SET );
}

bool ZippedBlockWriter::Cached() {
  return
    Header.LengthCompressed > 0 &&
    Buffer.Source.GetLength() == 0 &&
    Buffer.Compressed.GetLength() == 0;
}
#pragma endregion



#pragma region cache
ZippedBlockReaderCache::ZippedBlockReaderCache() {
  StackSize = 0;
  for( uint i = 0; i < StackSizeMax; i++ )
    Stack[i] = Null;

  CacheSizeMax = CACHE_READER_SIZE_DEFAULT;
  CacheSize = 0;
}

uint ZippedBlockReaderCache::GetBlocksCount() {
  return StackSize;
}

bool ZippedBlockReaderCache::CacheIn( ZippedBlockReader* block ) {
  if( GetTopBlock() == block )
    return false;

  if( block->IsCached )
    return false;

  Push( block );
  
  static const uint SizeForThread = 1024 * 1024 * 2;
  uint cacheSizeLimit = ZIPPED_THREADS_COUNT > 1 ?
    SizeForThread * ZIPPED_THREADS_COUNT:
    CacheSizeMax;

  if( CacheSize > cacheSizeLimit || StackSize >= StackSizeMax - 1 ) {
    CacheReduce();
  }

  return true;
}

void ZippedBlockReaderCache::CacheOut( ZippedBlockReader* block ) {
  if( block->IsCached )
    Pop( block );
}

void ZippedBlockReaderCache::CacheInvalidate( ZippedBlockReader* block ) {
  for( uint i = 0; i < StackSize; i++ ) {
    if( Stack[i] == block ) {
      CacheSize -= block->Header.LengthSource;
      Stack[i] = Null;
      break;
    }
  }
}

void ZippedBlockReaderCache::CacheOutLast() {
  if( StackSize > 0 )
    Pop( Stack[0] );
}

void ZippedBlockReaderCache::CacheReduce() {
  uint toRemove = StackSize / 2;
  for( uint i = 0; i < toRemove && i < StackSize - 1; i++ ) {
    if( Stack[i] != Null ) {
      auto& block = Stack[i];
      CacheSize -= block->Header.LengthSource;
      block->Buffer.Clear();
      block->IsCached = false;
    }
    else
      toRemove++;
  }
  
  Move( 0, toRemove, StackSize - toRemove );
  StackSize -= toRemove;
}

void ZippedBlockReaderCache::SetMemoryLimit( const ulong& size ) {
  CacheSizeMax = size;
}

ZippedBlockReader* ZippedBlockReaderCache::GetTopBlock() {
  if( StackSize == 0 )
    return Null;

  return Stack[StackSize - 1];
}

void ZippedBlockReaderCache::Push( ZippedBlockReader* block ) {
  block->IsCached = true;
  block->CommitData();
  block->Decompress();
  CacheSize += block->Header.LengthSource;
  Stack[StackSize++] = block;
}

void ZippedBlockReaderCache::Pop( ZippedBlockReader* block ) {
  CacheSize -= block->Header.LengthSource;
  for( uint i = 0; i < StackSize; i++ ) {
    if( Stack[i] == block ) {
      block->Buffer.Clear();
      block->IsCached = false;
      if( i < --StackSize )
        Move( i, i + 1, StackSize - i - 1 );

      break;
    }
  }

}

void ZippedBlockReaderCache::Move( const uint& toID, const uint& fromID, const uint& count ) {
  void* from  = &Stack[fromID];
  void* to    = &Stack[toID];
  uint length = count * 4;
  memcpy( to, from, length );
}

ZippedBlockReaderCache* ZippedBlockReaderCache::GetInstance() {
  static ZippedBlockReaderCache* stack = new ZippedBlockReaderCache();
  return stack;
}

void ZippedBlockReaderCache::ShowDebug() {
  for( uint i = 0; i < StackSize; i++ )
    printf(" %n", Stack[i] );
}
#pragma endregion