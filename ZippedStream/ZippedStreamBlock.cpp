#include "ZippedAfx.h"



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

    Buffer.Decompress();
    Header.LengthSource = Buffer.Source.GetLength();
  }

  if( clearCompressed )
    Buffer.Compressed.Clear();

  return Header.LengthSource > 0;
}

bool ZippedBlockBase::IsCompressed() {
  return Buffer.Compressed.GetLength() > 0;
}

bool ZippedBlockBase::IsDecompressed() {
  return Buffer.Source.GetLength() > 0;
}

ZippedBlockBase::~ZippedBlockBase() {
  // pass
}



ZippedBlockReader::ZippedBlockReader( FILE* baseStream, const ulong& position ) : ZippedBlockBase( baseStream, position ) {
  IsOnStack = false;
  CommitHeader();
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

void ZippedBlockReader::CacheIn( const ulong& position ) {
  ZippedBlockReaderCache::GetInstance()->CacheIn( this );
}

void ZippedBlockReader::CacheOut() {
  ZippedBlockReaderCache::GetInstance()->CacheOut( this );
}

bool ZippedBlockReader::Cached() {
  throw std::exception( "Read-only objects not support caching." );
}

ZippedBlockReader::~ZippedBlockReader() {
  CacheOut();
}



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

void ZippedBlockWriter::CacheIn( const ulong& position ) {
  ZIPASSERT( Buffer.Compressed.GetLength() > 0, "Buffer must be compressed before caching." );
  if( position != -1 )
    BasePosition = position;

  CommitHeader();
  CommitData();
  Buffer.Clear();
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



ZippedBlockReaderCache::ZippedBlockReaderCache() {
  CacheSizeMax = CACHE_READER_SIZE_DEFAULT;
  CacheSize = 0;
}

uint ZippedBlockReaderCache::GetBlocksCount() {
  return Stack.GetNum();
}

void ZippedBlockReaderCache::CacheIn( ZippedBlockReader* block ) {
  if( GetTopBlock() == block )
    return;

  if( block->IsOnStack ) {
    Stack.Remove( block );
    Stack.Insert( block );
    return;
  }

  while( Stack.GetNum() >= CACHE_READER_STACK_COUNT_MAX )
    CacheOutLast();

  Push( block );
  
  while( CacheSize > CacheSizeMax && Stack.GetNum() > 1 )
    CacheOutLast();
}

void ZippedBlockReaderCache::CacheOut( ZippedBlockReader* block ) {
  if( block->IsOnStack )
    Pop( block );
}

void ZippedBlockReaderCache::CacheOutLast() {
  if( Stack.GetNum() )
    Pop( Stack[0] );
}

void ZippedBlockReaderCache::SetMemoryLimit( const ulong& size ) {
  CacheSizeMax = size;
}

ZippedBlockReader* ZippedBlockReaderCache::GetTopBlock() {
  if( Stack.GetNum() == 0 )
    return Null;

  return Stack.GetLast();
}

void ZippedBlockReaderCache::Push( ZippedBlockReader* block ) {
  block->CommitData();
  block->Decompress();
  CacheSize += block->Header.LengthSource;
  Stack.Insert( block );
  block->IsOnStack = true;
}

void ZippedBlockReaderCache::Pop( ZippedBlockReader* block ) {
  CacheSize -= block->Header.LengthSource;
  Stack.Remove( block );
  block->Buffer.Clear();
  block->IsOnStack = false;
}

ZippedBlockReaderCache* ZippedBlockReaderCache::GetInstance() {
  static ZippedBlockReaderCache* stack = new ZippedBlockReaderCache();
  return stack;
}