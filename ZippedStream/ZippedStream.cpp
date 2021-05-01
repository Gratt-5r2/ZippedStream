#include "ZippedAfx.h"
#include <iostream>
using std::string;
using std::cout;
using std::endl;
using std::cin;

EXTERN_C {
ZSTREAMAPI ulong ZIPPED_THREADS_COUNT         = 8;
ZSTREAMAPI ulong BLOCK_SIZE_DEFAULT           = 1024 * 1024 / 4; // 0.25MB
ZSTREAMAPI ulong CACHE_READER_SIZE_DEFAULT    = 1024 * 1024 * 8; // 8MB
ZSTREAMAPI ulong CACHE_READER_STACK_COUNT_MAX = 1024;
}


#pragma region base
ZippedStreamBase::ZippedStreamBase( FILE* baseStream, long position ) {
  ZIPASSERT( baseStream != Null, "Can not create a zipped stream. Base stream is Null." );
  BaseStream         = baseStream;
  BasePosition       = position;
  Position           = 0;
  Blocks             = Null;
  Header.Length      = 0;
  Header.BlockSize   = BLOCK_SIZE_DEFAULT;
  Header.BlocksCount = 0;
}

long ZippedStreamBase::Tell() {
  return Position;
}

long ZippedStreamBase::Seek( const long& offset, const uint& origin ) {
  long newPosition = Position;
  switch( origin ) {
    case SEEK_SET: newPosition = offset; break;
    case SEEK_CUR: newPosition += offset; break;
    case SEEK_END: newPosition = Header.Length - offset - 1; break;
  }

  if( newPosition >= 0 && newPosition < (long&)Header.Length )
    Position = newPosition;

  return Position;
}

bool ZippedStreamBase::Compress( const bool& clearSource ) {
  bool success = true;
  for( uint i = 0; i < Header.BlocksCount; i++ )
    if( !Blocks[i]->Compress( clearSource ) )
      success = false;

  return success;
}

bool ZippedStreamBase::Decompress( const bool& clearCompressed ) {
  bool success = true;
  for( uint i = 0; i < Header.BlocksCount; i++ )
    if( !Blocks[i]->Decompress( clearCompressed ) )
      success = false;

  return success;
}

void ZippedStreamBase::SetBlockSize( const ulong& length ) {
  for( uint i = 0; i < Header.BlocksCount; i++ )
    Blocks[i]->SetBlockSize( length );

  Header.BlockSize = length;
}

ulong ZippedStreamBase::GetBlockSize() {
  return Header.BlockSize;
}

void ZippedStreamBase::Close( const bool& closeBaseStream ) {
  FILE* baseStream = BaseStream;
  delete this;
  if( closeBaseStream )
    fclose( baseStream );
}

ulong ZippedStreamBase::GetStreamSize() {
  ulong totalSize = sizeof( Header );
  for( uint i = 0; i < Header.BlocksCount; i++ ) {
    auto& header = Blocks[i]->Header;
    if( header.LengthCompressed != 0 )
      totalSize += sizeof( header ) + header.LengthCompressed;
  }

  return totalSize;
}

ZippedStreamBase::~ZippedStreamBase() {
  for( uint i = 0; i < Header.BlocksCount; i++ ) {
    delete Blocks[i];
    Blocks[i] = Null;
  }

  delete[] Blocks;
}
#pragma endregion



#pragma region reader
ZippedStreamReader::ZippedStreamReader( FILE* baseStream, long position ) : ZippedStreamBase( baseStream, position ) {
  CommitHeader();
  CommitData();
}

void ZippedStreamReader::CommitHeader() {
  long returnPosition = ftell( BaseStream );
  fseek( BaseStream, BasePosition, SEEK_SET );
  fread( &Header, 1, sizeof( Header ), BaseStream );
  fseek( BaseStream, returnPosition, SEEK_SET );
}

void ZippedStreamReader::CommitData() {
  long returnPosition = ftell( BaseStream );
  long position = BasePosition + sizeof( Header );

  Blocks = new ZippedBlockBase*[Header.BlocksCount];
  for( uint i = 0; i < Header.BlocksCount; i++ ) {
    Blocks[i] = new ZippedBlockReader( BaseStream, position );
    position += Blocks[i]->GetFileSize();
  }

  fseek( BaseStream, returnPosition, SEEK_SET );
}

ZippedBlockBase* ZippedStreamReader::GetBlockToRead() {
  uint blockID = Position / Header.BlockSize;
  uint blockPosition = Position - blockID * Header.BlockSize;
  auto block = Blocks[blockID];
  block->Seek( blockPosition );

  uint cachedCount = blockID + ZIPPED_THREADS_COUNT;
  uint blockCount  = Header.BlocksCount;
  for( uint i = blockID + 1; i <= cachedCount && i < blockCount; i++ )
    Blocks[i]->CacheIn();

  return block;
}

ulong ZippedStreamReader::Read( byte* buffer, const ulong& length ) {
  if( length == 0 )
    return 0;

  ulong toRead = length;
  ulong readedTotal = 0;
  while( toRead > 0 ) {
    auto block = GetBlockToRead();
    ulong readed = block->Read( buffer, toRead );
    if( readed == 0 )
      break;

    buffer += readed;
    toRead -= readed;
    Position += readed;
    readedTotal += readed;
  }

  return readedTotal;
}

ulong ZippedStreamReader::Write( byte* buffer, const ulong& length ) {
  throw std::exception( "Can not write a zipped file from the read-only object." );
}

bool ZippedStreamReader::EndOfFile() {
  return Position >= (long&)Header.Length;
}
#pragma endregion



#pragma region writer
ZippedStreamWriter::ZippedStreamWriter( FILE* baseStream, long position ) : ZippedStreamBase( baseStream, position ) {
  LengthCompressed = 0;
}

long ZippedStreamWriter::Seek( const long& offset, const uint& origin ) {
  ZIPASSERT( Header.BlocksCount == 0, "Can not change a zipped stream position after start of writing." );
  return ZippedStreamBase::Seek( offset, origin );
}

void ZippedStreamWriter::CommitHeader() {
  fseek( BaseStream, BasePosition, SEEK_SET );
  fwrite( &Header, 1, sizeof( Header ), BaseStream );
  fseek( BaseStream, BasePosition + GetStreamSize(), SEEK_SET );
}

void ZippedStreamWriter::CommitData() {
  for( uint i = 0; i < Header.BlocksCount; i++ )
    FlushBlock( Blocks[i] );
}

ulong ZippedStreamWriter::Read( byte* buffer, const ulong& length ) {
  throw std::exception( "Can not read a zipped file from the write-only object." );
}

void ZippedStreamWriter::FlushBlock( ZippedBlockBase* block ) {
  if( block->Cached() )
    return;

  block->BasePosition = BasePosition + GetStreamSize();
  block->Compress();
  block->CacheIn();
}

ZippedBlockBase* ZippedStreamWriter::GetBlockToWrite() {
  uint blockID = Position / Header.BlockSize;
  uint blockPosition = Position - blockID * Header.BlockSize;
  if( blockID >= Header.BlocksCount ) {
    ZIPASSERT( blockID == Header.BlocksCount, "Can not create a far zipped writer block." );
    Blocks = (ZippedBlockBase**)shi_realloc( Blocks, ++Header.BlocksCount * 4 );
    Blocks[blockID] = new ZippedBlockWriter( BaseStream );
    Blocks[blockID]->SetBlockSize( Header.BlockSize );

    if( blockID > 0 )
      FlushBlock( Blocks[blockID - 1] );
  }

  Blocks[blockID]->Seek( blockPosition );
  return Blocks[blockID];
}

ulong ZippedStreamWriter::Write( byte* buffer, const ulong& length ) {
  if( length == 0 )
    return 0;
  
  ulong toWrite = length;
  ulong writedTotal = 0;
  while( toWrite > 0 ) {
    auto block = GetBlockToWrite();
    ulong writed = block->Write( buffer, toWrite );
    ZIPASSERT( writed > 0, "Bad write operation. Return value can not be Zero." );

    buffer += writed;
    toWrite -= writed;
    writedTotal += writed;
    Position += writed;
  }

  Header.Length += writedTotal;
  return writedTotal;
}

bool ZippedStreamWriter::EndOfFile() {
  return false;
}

void ZippedStreamWriter::Flush() {
  CommitData();
  CommitHeader();
}

ZippedStreamWriter::~ZippedStreamWriter() {
  Flush();
}
#pragma endregion