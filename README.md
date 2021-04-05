
# Compressed streams
Compressed streams use the zip algorithm (based on zlib). Zipped streams are built on a segment data structure, this is the difference between zipped streams and regular zip compression. Writing a buffer is accompanied by the creation of blocks of a certain size, which will be written to disk in a compressed form. When reading, the blocks will be read and unpacked back into buffers of a certain size.

The segmented structure allows you to quickly access any part of the file and selectively extract data from it. When trying to read a (uncompressed) range of data from a (compressed) file, the zipped stream (based on the size of the segments) reads the nearest segments from the (compressed) file and then decompresses them into memory.

By default, zipped stream segments take up 2 MB of uncompressed memory. When a segment is compressed, its physical size will be reduced and written to disk. Several uncompressed segments (cache) can be simultaneously in memory for quick access to previously used segments in large files. The maximum number of segments in memory is determined by the maximum allowable volume of unpacked data, but no more than 1024 segments. By default, the maximum allowed size is 20 MB, which corresponds to 10 unpacked segments (20 / 2).

You can change the maximum allowable size (cache) of uncompressed data as follows:
```cpp
// Changing cache size up to 50 MB
ZippedBlockReaderCache::GetInstance()->SetMemoryLimit( 1024 * 1024 * 50 );
```

# Compressed data structure
```
The data structure of the compressed file looks like this:
[FILE HEADER]         A header of the compressed file
Length       4 bytes  Length of uncompressed data
BlockSize    4 bytes  Segments size
BlocksCount  4 bytes  Number of the segments

  [BLOCK HEADER]             Segment header
  LengthSource      4 bytes  Length of uncompressed segment data
  LengthCompressed  4 bytes  Compressed segment data length
  BlockSize         4 bytes  Segment size
  Bytes             N bytes  Compressed segment data, where N equals LengthCompressed

  [BLOCK HEADER]             Segment header
  LengthSource      4 bytes  Length of uncompressed segment data
  LengthCompressed  4 bytes  Compressed segment data length
  BlockSize         4 bytes  Segment size
  Bytes             N bytes  Compressed segment data, where N equals LengthCompressed

  [BLOCK HEADER]             Segment header
  LengthSource      4 bytes  Length of uncompressed segment data
  LengthCompressed  4 bytes  Compressed segment data length
  BlockSize         4 bytes  Segment size
  Bytes             N bytes  Compressed segment data, where N equals LengthCompressed
  
  ...
```

# Writing data to disk
The write position in the file can be set before the zipped stream starts writing. After the start of writing, an attempt to change the position of the reading will throw an exception. This is due to the fact that a zipped stream immediately divides the data being written into blocks and compresses them as it fills. The compressed blocks are sent to the base stream cache, and all intermediate buffers are removed from memory. This solution allows you not to get stuck on the consumed amount of memory in x32-bit applications.

The final amount of memory written to the zipped stream is determined after calling the Flush procedure (also called when the stream is destroyed). The Flush command finishes the writing of the remaining blocks and appends the file header to the base stream. After Flush called, no further writing to the zipped stream will be possible.

The stream is closed by the Close function. At this time, the current zipped stream instance is deleted and Flush is called. Optionally, the base stream is closed if it is no longer needed.



## The compression the file
```cpp
void TestCompress() {
  // Streams for input and output files
  FILE* fileIn  = fopen( "Speech1.vdf", "rb" );
  FILE* fileOut = fopen( "zipped.vdf", "wb+" );
  ZIPASSERT( fileIn  != Null, "Can not open fileIn." );
  ZIPASSERT( fileOut != Null, "Can not open fileOut." );

  // Zipped stream based on stream fileOut. In the moment
  // initialization, you can specify the position of the compressed data
  // in the future file, and also specify the segment size.
  ZippedStreamWriter* zippedWriter = new ZippedStreamWriter( fileOut );
  
  // Intermediate data read from
  // input file and written to output.
  const size_t cacheSize = 8192;
  byte cache[cacheSize];

  while( true ) {
    size_t readed = fread( cache, 1, cacheSize, fileIn );
    if( readed == 0 )
      break;

    // Write data to the zipped stream. Data compression
    // and resetting the base stream cache is done
    // automatically as new segments fill.
    zippedWriter->Write( cache, readed );
  }

  // Close and delete the current zipped stream. Before deleting
  // zipped stream appends all remaining data to the base stream.
  // Optionally close the base stream (default true).
  zippedWriter->Close();

  fclose( fileIn );
}
```

## Compressing a data range
```cpp
void FileFromTo( FILE* fileIn,  const int& positionIn,
                 FILE* fileOut, const int& positionOut,
                 const size_t& length ) {
  // Create a zipped stream at a given position within the output
  // file, as well as resizing the segments to 1 MB.
  ZippedStreamWriter* zippedWriter = new ZippedStreamWriter( fileOut, positionOut );
  zippedWriter->SetBlockSize( 1024 * 1024 * 1 );

  fseek( fileIn, positionIn, SEEK_SET );

  // Intermediate data read from
  // input file and written to output one.
  const size_t cacheSize = 8192;
  byte cache[cacheSize];
  size_t readLeft = length;

  while( readLeft > 0 ) {
    size_t toRead = min( readLeft, cacheSize );
    size_t readed = fread( cache, 1, toRead, fileIn );
    if( readed == 0 )
      break;

    // Write data to a zipped stream. Data compression
    // and resetting the base stream cache is done
    // automatically as new segments fill.
    zippedWriter->Write( cache, readed );

    readLeft -= readed;
  }

  // close and destroy the zipped stream
  // without closing the main thread.
  zippedWriter->Close( false );
}
```

# Reading data from disk
Accessing a zipped stream has no difference from accessing usual streams. The file can either be read fully or in partically. In order to read a specific part of a compressed file, the program does not need to decompress it completely. To do this, the zipped stream calculates the closest compressed segments relative to the given index of the uncompressed file. The zipped stream will unpack only the nearest segments in the range, which have needed data.

The unpacked segments will stay in memory in order of using them. The least used segments will be unloaded from memory if the total amount of decompressed data begins to exceed the specified limit (by default, the decompressed data cache is 20 MB). Holding the data allows the zipped stream to perform the minimum number of decompression operations.

## Complete file unpacking
```cpp
void TestDecompress() {
  // Streams for input and output files
  FILE* fileIn  = fopen( "zipped.vdf", "rb" );
  FILE* fileOut = fopen( "unzipped.vdf", "wb+" );
  ZIPASSERT( fileIn  != Null, "Can not open fileIn." );
  ZIPASSERT( fileOut != Null, "Can not open fileOut." );
  
  // Zipped stream based on stream fileIn. AT
   // the moment of initialization, you can specify the position
   // start of compressed data in file 'fileIn'.
  ZippedStreamReader* zippedReader = new ZippedStreamReader( fileIn );

  // Intermediate data read from
  // input file and written to output one.
  const size_t cacheSize = 8192;
  byte cache[cacheSize];

  while( true ) {
    // Sequentially read data from the zipped stream.
     // The zipped stream will automatically unpack the required
     // segments of the compressed file and will pass them to cache.
    size_t readed = zippedReader->Read( cache, cacheSize );
    if( readed == 0 )
      break;

    fwrite( cache, 1, readed, fileOut );
  }

  // Close and delete the current zipped stream. Before deleting
  // the zipped stream will unload all the cached segments.
  // Optionally close the base stream (default true).
  zippedReader->Close();

  fclose( fileOut );
}
```

## Retrieving a data range
```cpp
size_t ReadCompressedData( FILE* fileIn, byte* buffer, const long& position, const size_t& length ) {
  // Calculate the starting point in the stream,
  // which contains the compressed data.
  long baseStreamPosition = ftell( fileIn );

  // Create a zipped stream at the start point of the compressed
  // data. During initialization, the zipped stream
  // will create a list of compressed file segments.
  ZippedStreamReader* zippedReader = new ZippedStreamReader( fileIn, baseStreamPosition );

  // The function sets the virtual position corresponding to
  // real position inside the unpacked file.
  zippedReader->Seek( position );

  // Read data from a zipped stream. Zipped stream
  // automatically unpack the required segments
  // compressed file and copied to buffer.
  size_t readed = zippedReader->Read( buffer, length );

  // close and destroy the zipped stream
  // without closing the main thread.	
  zippedReader->Close( false );

  // Return the reference value defining the real
  // the number of bytes read from the zipped stream.
  return readed;
}
