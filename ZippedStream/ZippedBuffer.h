#pragma once

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

  ZippedBuffer();
  ZippedBuffer( const ulong& length );
  void Compress();
  void Decompress();
  void Clear();
  ~ZippedBuffer();
};
