#include "ZippedAfx.h"
#include <iostream>
using std::string;
using std::cout;
using std::endl;
using std::cin;

const bool DebugWriteFiles = false;

void TestCompress() {
  cout << "Compressed test . . ." << endl;
  char* fileName = "Textures.vdf";
  FILE* fileIn = fopen( fileName, "rb" );
  if( fileIn == Null ) {
    throw std::exception( "Can not open Input file." );
    return;
  }

  ZippedStreamWriter* writer = new ZippedStreamWriter( fopen( "zipped.vdf", "wb+" ) );

  const uint cacheSize = 8192;
  byte cache[cacheSize];
  uint timeStart = GetTickCount();
  while( true ) {
    uint readed = fread( cache, 1, cacheSize, fileIn );
    if( readed == 0 )
      break;

    if( DebugWriteFiles )
      writer->Write( cache, readed );
  }
  uint timeEnd = GetTickCount();
  cout << "Compressed in " << timeEnd - timeStart << "ms" << endl;

  writer->Close();
  fclose( fileIn );
}



void TestDecompress() {
  cout << "Decompressed test . . ." << endl;
  char* fileName = "unzipped.vdf";
  FILE* fileOut = fopen( fileName, "wb+" );
  if( fileOut == Null ) {
    throw std::exception( "Can not open Output file." );
    return;
  }

  ZippedStreamReader* reader = new ZippedStreamReader( fopen( "zipped.vdf", "rb" ) );

  const uint cacheSize = 8192;
  byte cache[cacheSize];
  uint timeStart = GetTickCount();
  while( true ) {
    uint readed = reader->Read( cache, cacheSize );
    if( readed == 0 )
      break;

    if( DebugWriteFiles )
      fwrite( cache, 1, readed, fileOut );
  }
  uint timeEnd = GetTickCount();
  cout << "Decompressed in " << timeEnd - timeStart << "ms" << endl;

  reader->Close();
  fclose( fileOut );
}




void TestCopy() {
  cout << "Copy test . . ." << endl;
  char* fileName1 = "Textures.vdf";
  FILE* fileIn = fopen( fileName1, "rb" );
  if( fileIn == Null ) {
    throw std::exception( "Can not open Input file." );
    return;
  }

  char* fileName2 = "copied.vdf";
  FILE* fileOut = fopen( fileName2, "wb+" );
  if( fileOut == Null ) {
    throw std::exception( "Can not open Output file." );
    return;
  }

  const uint cacheSize = 8192;
  byte cache[cacheSize];
  uint timeStart = GetTickCount();
  while( true ) {
    uint readed = fread( cache, 1, cacheSize, fileIn );
    if( readed == 0 )
      break;

    if( DebugWriteFiles )
      fwrite( cache, 1, readed, fileOut );
  }
  uint timeEnd = GetTickCount();
  cout << "Copy in " << timeEnd - timeStart << "ms" << endl;

  fclose( fileIn );
  fclose( fileOut );
}


void main() {
  try {
    while( true ) {
      for( uint i = 0; i < 5000; i++ ) {
        cout << "Iteration: " << i << endl;
        TestDecompress();
      }
      //ZippedBlockReaderCache::GetInstance()->ShowDebug();
      //TestCopy();
      system( "pause" );
    }
  }
  catch( const std::exception& e ) {
    cout << e.what() << endl;
  }

  system("pause");
}