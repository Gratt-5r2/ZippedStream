# ZippedStream
Fast compressed stream

The library allows to create Segmented compressed files and read them quickly from any source-part of the file.
Compressed file divided into (compressed) blocks. Each block defines some decompressed volume.
For example file of 15BM can be divided on 8 blocks of 2MB. At next this blocks will be compressed and saved to file.

# Compressed file structure:
```
  [ZIPPED FILE HEADER]
  Length      4 Bytes
  BlockSize   4 Bytes
  BlocksCount 4 Bytes

      [ZIPPED DATA]
      LengthSource     4 Bytes
      LengthCompressed 4 Bytes
      BlockSize        4 Bytes
      ZippedPtr        [LengthCompressed] Bytes

      [ZIPPED DATA]
      LengthSource     4
      LengthCompressed 4
      BlockSize        4
      ZippedPtr        [LengthCompressed] Bytes

      [ZIPPED DATA]
      LengthSource     4
      LengthCompressed 4
      BlockSize        4
      ZippedPtr        [LengthCompressed] Bytes
      
      ...
```

# Compress to file
```cpp
  // Source file
  FILE* fileIn = fopen( "source.bin", "rb" );
  if( fileIn == Null ) {
    throw std::exception( "Can not open Input file." );
    return;
  }
  
  // Compressed file
  FILE* fileOut = fopen( "compressed.bin", "wb+" );
  ZippedStreamWriter* writer = new ZippedStreamWriter( fileOut, 0 /* optional: position in fileOUT stream */ );
  writer->SetBlockSize( 1024 * 1024 * 5 ); // 5MB for some blocks. Default is 2MB
  
  // Read some data from fileIN and write to fileOUT.
  // Stream will automatically compress as blocks fill.
  const uint cacheSize = 8192;
  byte cache[cacheSize];
  while (true) {
    uint readed = fread( cache, 1, cacheSize, fileIn );
    if( readed == 0 )
      break;
      
    uint writed = writer->Write( cache, readed );
  }

  fclose( fileIn );
  writer->Close( true /* optional: close base stream */ ); // delete reader and close a base stream
```

# Read compressed file
```cpp
  FILE* fileOut = fopen( "uncompressed.bin", "wb+" );
  if( fileOut == Null ) {
    throw std::exception( "Can not open Output file." );
    return;
  }
  
  FILE* fileIn = fopen( "compressed.bin", "rb" );
  ZippedStreamReader* reader = new ZippedStreamReader( fileIn );
  
  const uint cacheSize = 8192;
  byte cache[cacheSize];
  while (true) {
    uint readed = reader->Read( cache, cacheSize );
    if( readed == 0 )
      break;

    fwrite( cache, 1, readed, fileOut );
  }
  
  reader->Close(); // delete reader and close a base stream
  fclose( fileOut );
```
# Seeking in compressed file
Stream automatically decompresses required file segments and keeps in memory while it is being used.
Data access looks like as to normal file. 
```cpp
  ...
  ZippedStreamReader* reader = new ZippedStreamReader( fileIn, positionInFile );
  reader->Seek( offset, SEEK_CUR );
  ulong readed = reader->Read( buffer, length );
  reader->Close( false ); // delete reader but don't close a base stream
  ...
```
The system has a memory limit for keeping decompressed blocks. 20MB is default volume.
For change limit see the following code. 
```cpp
  ZippedBlockStack::GetInstance()->SetMemoryLimit( 1024 * 1024 * 50 ); // 50MB. Default - 20MB.
```
