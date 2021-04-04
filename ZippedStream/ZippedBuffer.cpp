#include "ZippedAfx.h"

void ZippedBufferProto::SetBuffer( byte* buffer, const ulong& length ) {
  Clear();
  Buffer = buffer;
  Length = length;
}

byte* ZippedBufferProto::GetBuffer() {
  return Buffer;
}

ulong ZippedBufferProto::GetLength() {
  return Length;
}

ulong ZippedBufferProto::Write( byte* buffer, const ulong& length ) {
  if( !Buffer )
    Buffer = new byte[Parent->LengthMax];
  
  uint memLeft = Parent->LengthMax - Length;
  uint toWrite = min( length, memLeft );
  memcpy( Buffer + Length, buffer, toWrite );
  Length += toWrite;
  return toWrite;
}

void ZippedBufferProto::Clear() {
  if( Buffer != Null )
    delete[] Buffer;
  Buffer = Null;
  Length = 0;
}

ZippedBufferProto::~ZippedBufferProto() {
  Clear();
}




ZippedBuffer::ZippedBuffer() {
  LengthMax = BLOCK_SIZE_DEFAULT;
  Source.Buffer = Null;
  Source.Length = 0;
  Source.Parent = this;
  Compressed.Buffer = Null;
  Compressed.Length = 0;
  Compressed.Parent = this;
}

ZippedBuffer::ZippedBuffer( const ulong& length ) {
  LengthMax = length;
  Source.Buffer = Null;
  Source.Length = 0;
  Source.Parent = this;
  Compressed.Buffer = Null;
  Compressed.Length = 0;
  Compressed.Parent = this;
}

void ZippedBuffer::Compress() {
  Compressed.Length = compressBound( Source.Length );
  Compressed.Buffer = (byte*)shi_realloc( Compressed.Buffer, Compressed.Length );
  ZIPASSERT( Compressed.Buffer != Null, "Can not alloc buffer. Out of memory." );
  int result = compress( Compressed.Buffer, &Compressed.Length, Source.Buffer, Source.Length );
  ZIPASSERT( result == Z_OK, "Compress failed!" );
  Compressed.Buffer = (byte*)shi_realloc( Compressed.Buffer, Compressed.Length );
}

void ZippedBuffer::Decompress() {
  Source.Length = compressBound( LengthMax );
  Source.Buffer = (byte*)shi_realloc( Source.Buffer, Source.Length );
  ZIPASSERT( Source.Buffer != Null, "Can not alloc buffer. Out of memory." );
  int result = uncompress( Source.Buffer, &Source.Length, Compressed.Buffer, Compressed.Length );
  ZIPASSERT( result == Z_OK, "Decompress failed." );
  Source.Buffer = (byte*)shi_realloc( Source.Buffer, Source.Length );
}

void ZippedBuffer::Clear() {
  Source.Clear();
  Compressed.Clear();
}

ZippedBuffer::~ZippedBuffer() {
  // pass
}