#pragma once

struct ZSTREAMAPI ZippedBuffer_AsyncHelper;
struct ZSTREAMAPI AsyncContext;



class ZSTREAMAPI ZippedBufferProto {
  friend struct ZippedBuffer;
  ZippedBuffer* Parent;
  byte* Buffer;
  ulong Length;

public:
  void SetBuffer( byte* buffer, const ulong& length );
  byte* GetBuffer();
  ulong GetLength();
  ulong Write( byte* buffer, const ulong& length );
  void Clear();
  ~ZippedBufferProto();
};



struct ZSTREAMAPI ZippedBuffer {
  ulong LengthMax; // Maximum length of the buffer
  ZippedBufferProto Source;
  ZippedBufferProto Compressed;
  AsyncContext* AsyncContext;
  Common::ThreadLocker DecompressContextMutex;

  ZippedBuffer();
  ZippedBuffer( const ulong& length );
  void Compress();
  void Decompress( bool async );
  void Clear();
  bool IsCompressed();
  bool IsDecompressed();
  void WaitForDecompress();
  bool DecompressIsActive();
  ~ZippedBuffer();

protected:
  void DecompressAsync();
};



struct ZSTREAMAPI AsyncContext {
  uint Index;
  Common::Thread Thread;
  Common::ThreadLocker Mutex;
  HANDLE WaitForStart;
  HANDLE WaitForEnd;
  ZippedBuffer* Buffer;
  bool_t UseOneBuffer;
  void(__thiscall ZippedBuffer::* Function)();
  void SetLowPriority();
  void SetHighPriority();
};



struct ZSTREAMAPI ZippedBuffer_AsyncHelper {
  Common::Array<AsyncContext> Contexts;
  uint Iterator;

  ZippedBuffer_AsyncHelper( const uint& threads_count );
  AsyncContext& GetNextThread();
  AsyncContext& Start( ZippedBuffer* owner, void(__thiscall ZippedBuffer::* func)() );
  ~ZippedBuffer_AsyncHelper();
  static void AsyncProcedure( AsyncContext& context );
  static ZippedBuffer_AsyncHelper& GetInstance( const uint& threads_count = 8 );
};