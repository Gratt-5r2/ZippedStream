#include "ZippedAfx.h"
#include <iostream>
using std::string;
using std::cout;
using std::endl;
using std::cin;



void TestCompress() {
  cout << "Compressed test . . ." << endl;
  char* fileName = "Sounds_Addon.vdf";
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

    uint writed = writer->Write( cache, readed );
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

  ZippedStreamReader* reader = new ZippedStreamReader( fopen( "Sounds_Addon_zipped.vdf" /* "zipped.vdf"*/, "rb" ) );

  const uint cacheSize = 8192;
  byte cache[cacheSize];
  uint timeStart = GetTickCount();
  while( true ) {
    uint readed = reader->Read( cache, cacheSize );
    if( readed == 0 )
      break;

    fwrite( cache, 1, readed, fileOut );
  }
  uint timeEnd = GetTickCount();
  cout << "Decompressed in " << timeEnd - timeStart << "ms" << endl;

  reader->Close();
  fclose( fileOut );
}



void main() {
  try {
    //TestCompress();
    TestDecompress();
  }
  catch( const std::exception& e ) {
    cout << e.what() << endl;
  }

  system("pause");
}