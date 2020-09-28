#ifndef __TS_STORAGE_FFAT__
#define __TS_STORAGE_FFAT__

//
// Low level FFat functions
// - only to be included from storage.cpp
//

#include "FS.h"
#include "FFat.h"

namespace StorageFFat {

  bool begin()
  {
    return FFat.begin();
  }

  uint32_t totalBytes()
  {
    return FFat.totalBytes();
  }

  uint32_t freeBytes()
  {
    return FFat.freeBytes();
  }

  void listDir(const char* dirname, uint8_t levels)
  {
    printf("Listing directory: %s\r\n", dirname);

    File root = FFat.open(dirname);
    if(!root)
    {
        log_e("- failed to open directory");
        return;
    }
    
    if(!root.isDirectory())
    {
        log_e(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file)
    {
        if(file.isDirectory())
        {
            printf("  DIR : %s\r\n", file.name());
            if(levels)
            {
                listDir(file.name(), levels -1);
            }
        }
        else
        {
            printf("  FILE: %s\tSIZE: %n\r\n", file.name(), file.size());
        }
        
        file = root.openNextFile();
    }

    file.close();
}

File openDir(const char * path)
{
  File file = FFat.open(path);
  if(!file || !file.isDirectory())
  {
    log_e("- failed to open dir for reading / not a dir");
  }
    
  return file;
}

File openRead(const char * path)
{
  File file = FFat.open(path);
  if(!file || file.isDirectory())
  {
    log_e("- failed to open file for reading");
  }
    
  return file;
}

File openWrite(const char * path)
{
  File file = FFat.open(path, FILE_WRITE);
  if(!file)
  {
    log_e("- failed to open file for writing");
  }

  return file;
}

File openAppend(const char * path)
{
  File file = FFat.open(path, FILE_APPEND);
  if(!file)
  {
    log_e("- failed to open file for appending");
  }

  return file;
}

bool renameFile(const char * path1, const char * path2)
{
  if (!FFat.rename(path1, path2))
  {
    log_e("- file rename failed");
    return false;
  }

  return true;
}

bool deleteFile(const char * path)
{
  if(!FFat.remove(path))
  {
    log_e("- delete failed");
    return false;
  }

  return true;
}

bool testFileIO(const char * path)
{
  log_w("Testing file I/O with %s", path);

  static uint8_t buf[512];
  size_t len = 0;
  File file = FFat.open(path, FILE_WRITE);
  if(!file)
  {
    log_e("- failed to open file for writing");
    return false;
  }

  size_t i;
  log_w("- writing" );
  uint32_t start = millis();
  for(i=0; i<2048; i++)
  {
    if ((i & 0x001F) == 0x001F)
    {
      printf(".");
    }
        
    file.write(buf, 512);
  }
    
  log_w("");
  uint32_t end = millis() - start;
  log_w(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
  file.close();

  file = FFat.open(path);
  start = millis();
  end = start;
  i = 0;
  if(file && !file.isDirectory())
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    log_w("- reading" );
     
    while(len)
    {
      size_t toRead = len;
      if(toRead > 512)
      {
        toRead = 512;
      }
       
      file.read(buf, toRead);
      if ((i++ & 0x001F) == 0x001F)
      {
        printf(".");
      }
      
      len -= toRead;
    }
      
    log_w("");
    end = millis() - start;
      
    log_w("- %u bytes read in %u ms\r\n", flen, end);
    file.close();

    // delete test file
    return deleteFile(path);
  }
  else
  {
    log_e("- failed to open file for reading");
    return false;
  }

  
}

};

#endif
