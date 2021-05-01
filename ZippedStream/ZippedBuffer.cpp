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
  LengthMax         = BLOCK_SIZE_DEFAULT;
  Source.Buffer     = Null;
  Source.Length     = 0;
  Source.Parent     = this;
  Compressed.Buffer = Null;
  Compressed.Length = 0;
  Compressed.Parent = this;
  AsyncContext      = Null;
}

ZippedBuffer::ZippedBuffer( const ulong& length ) {
  LengthMax         = length;
  Source.Buffer     = Null;
  Source.Length     = 0;
  Source.Parent     = this;
  Compressed.Buffer = Null;
  Compressed.Length = 0;
  Compressed.Parent = this;
  AsyncContext      = Null;
}

void ZippedBuffer::Compress() {
  Compressed.Length = compressBound( Source.Length );
  Compressed.Buffer = (byte*)shi_realloc( Compressed.Buffer, Compressed.Length );
  ZIPASSERT( Compressed.Buffer != Null, "Can not alloc buffer. Out of memory." );
  int result = compress( Compressed.Buffer, &Compressed.Length, Source.Buffer, Source.Length );
  ZIPASSERT( result == Z_OK, "Compress failed!" );
  Compressed.Buffer = (byte*)shi_realloc( Compressed.Buffer, Compressed.Length );
}

void ZippedBuffer::Decompress( bool async ) {
  if( !async || ZIPPED_THREADS_COUNT <= 1 ) {
    DecompressAsync();
    Compressed.Clear();
    return;
  }

  DecompressContextMutex.Enter();
  AsyncContext = &ZippedBuffer_AsyncHelper::GetInstance().Start( this, &ZippedBuffer::DecompressAsync );
  DecompressContextMutex.Leave();
}

void ZippedBuffer::DecompressAsync() {
  Source.Length = compressBound( LengthMax );
  Source.Buffer = (byte*)shi_realloc( Source.Buffer, Source.Length );
  ZIPASSERT( Source.Buffer != Null, "Can not alloc buffer. Out of memory." );
  int result = uncompress( Source.Buffer, &Source.Length, Compressed.Buffer, Compressed.Length );
  ZIPASSERT( result == Z_OK, "Decompress failed." );
  Source.Buffer = (byte*)shi_realloc( Source.Buffer, Source.Length );

  DecompressContextMutex.Enter();
  if( AsyncContext ) {
    if( AsyncContext->UseOneBuffer )
      Compressed.Clear();
    AsyncContext = Null;
  }
  DecompressContextMutex.Leave();
}

void ZippedBuffer::Clear() {
  WaitForDecompress();
  Source.Clear();
  Compressed.Clear();
}

bool ZippedBuffer::IsCompressed() {
  return Compressed.GetLength() > 0;
}

bool ZippedBuffer::IsDecompressed() {
  if( DecompressIsActive() )
    return false;

  return Source.GetLength() > 0;
}

void ZippedBuffer::WaitForDecompress() {
  DecompressContextMutex.Enter();
  HANDLE event = AsyncContext ? AsyncContext->WaitForEnd : Null;
  DecompressContextMutex.Leave();

  if( event != Null )
    WaitForSingleObject( event, INFINITE );
}

bool ZippedBuffer::DecompressIsActive() {
  DecompressContextMutex.Enter();
  bool value = AsyncContext != Null;
  DecompressContextMutex.Leave();
  return value;
}

ZippedBuffer::~ZippedBuffer() {
  WaitForDecompress();
}







void AsyncContext::SetLowPriority() {
  Thread.SetPriority( Common::THREAD_LOW );
}

void AsyncContext::SetHighPriority() {
  Thread.SetPriority( Common::THREAD_HIGH );
}

ZippedBuffer_AsyncHelper::ZippedBuffer_AsyncHelper( const uint& threads_count ) {
  Iterator = 0;
  for( uint i = 0; i < threads_count; i++ )
    auto& con = Contexts.Create();

  for( uint i = 0; i < threads_count; i++ ) {
    auto& con = Contexts[i];
    con.Index = i;
    con.WaitForStart = CreateEvent( Null, True, False, Null );
    con.WaitForEnd   = CreateEvent( Null, True, True,  Null );
    con.Thread.Init( &AsyncProcedure );
    con.Thread.Detach( &con );
  }
}

AsyncContext& ZippedBuffer_AsyncHelper::GetNextThread() {
  AsyncContext& context = Contexts[Iterator++];
  if( Iterator >= Contexts.GetNum() )
    Iterator = 0;

  return context;
}

AsyncContext& ZippedBuffer_AsyncHelper::Start( ZippedBuffer* owner, void(__thiscall ZippedBuffer::* func)() ) {
  AsyncContext& con = GetNextThread();
  WaitForSingleObject( con.WaitForEnd, INFINITE );
  ResetEvent( con.WaitForEnd );
  con.Buffer = owner;
  con.Function = func;
  con.UseOneBuffer = True;
  SetEvent( con.WaitForStart );
  return con;
}

void ZippedBuffer_AsyncHelper::AsyncProcedure( AsyncContext& context ) {
  while( true ) {
    WaitForSingleObject( context.WaitForStart, INFINITE );
    ResetEvent( context.WaitForStart );
    (context.Buffer->*context.Function)();
    SetEvent( context.WaitForEnd );
  }
}

ZippedBuffer_AsyncHelper::~ZippedBuffer_AsyncHelper() {
  for( uint i = 0; i < Contexts.GetNum(); i++ ) {
    Contexts[i].Thread.Break();
    CloseHandle( Contexts[i].WaitForStart );
    CloseHandle( Contexts[i].WaitForEnd );
  }
}

ZippedBuffer_AsyncHelper& ZippedBuffer_AsyncHelper::GetInstance( const uint& threads_count ) {
  static ZippedBuffer_AsyncHelper helper( threads_count );
  return helper;
}