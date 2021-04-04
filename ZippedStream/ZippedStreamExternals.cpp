#include "ZippedAfx.h"

EXTERN_C {
int ZSTREAMAPI ZippedStreamOpenRead( FILE* file, long position ) {
  MessageBox(0, 0, 0, 0);
  ZippedStreamBase* stream = new ZippedStreamReader( file, position );
  return (int)stream;
}

int ZSTREAMAPI ZippedStreamOpenWrite( FILE* file, long position ) {
  ZippedStreamBase* stream = new ZippedStreamWriter( file, position );
  return (int)stream;
}

void ZSTREAMAPI ZippedStreamSetBlockSize( int streamHandle, int length ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  stream->SetBlockSize( length );
}

int ZSTREAMAPI ZippedStreamGetBlockSize( int streamHandle ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->GetBlockSize();
}

int ZSTREAMAPI ZippedStreamTell( int streamHandle ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->Tell();
}

int ZSTREAMAPI ZippedStreamSeek( int streamHandle, int offset, int origin = SEEK_SET ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->Seek( offset, origin );
}

int ZSTREAMAPI ZippedStreamGetStreamSize( int streamHandle ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->GetStreamSize();
}

int ZSTREAMAPI ZippedStreamRead( int streamHandle, byte* buffer, int length ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->Read( buffer, length );
}

int ZSTREAMAPI ZippedStreamWrite( int streamHandle, byte* buffer, int length ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->Write( buffer, length );
}

void ZSTREAMAPI ZippedStreamClose( int streamHandle, int closeBaseStream = True ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
}

int ZSTREAMAPI ZippedStreamEndOfFile( int streamHandle ) {
  ZippedStreamBase* stream = (ZippedStreamBase*)streamHandle;
  return stream->EndOfFile() ? True : False;
}
}