#include <switch.h>
#include <stdio.h>
#include <minizip/unzip.h>
#include <string.h>
#include <dirent.h>

#include "unzip.h"

#define WRITEBUFFERSIZE 500000 // 500KB
#define MAXFILENAME     500

int unzip(const char *zipFilePath) {
  unzFile zipFile = unzOpen(zipFilePath);
  unz_global_info globalInfo;
  unzGetGlobalInfo(zipFile, &globalInfo);

  if (globalInfo.number_entry < 0) {
    printf("\nNo archive to unzip\n");
    unzClose(zipFile);
    return 0;
  }

  printf("\nUnzipping. There are %ld files in archive.\n\n", globalInfo.number_entry);
  printf("1                  20                  40                  60                 80");

  for (int i = 0; i < globalInfo.number_entry; i++) {
    printf("|");
    consoleUpdate(NULL);

    char innerFileName[MAXFILENAME];
    unz_file_info fileInfo;

    unzOpenCurrentFile(zipFile);
    unzGetCurrentFileInfo(zipFile, &fileInfo, innerFileName, sizeof(innerFileName), NULL, 0, NULL, 0);

    // check if the string ends with a /, if so, then its a directory.
    if ((innerFileName[strlen(innerFileName) - 1]) == '/') {
      // check if directory exists
      DIR *dir = opendir(innerFileName);
      if (dir) {
        closedir(dir);
      } else {
        mkdir(innerFileName, 0777);
      }
    } else {
      u8 *buffer = malloc(WRITEBUFFERSIZE);
      FILE *outfile = fopen(innerFileName, "wb");

      for (int j = unzReadCurrentFile(zipFile, buffer, WRITEBUFFERSIZE);
           j > 0; j = unzReadCurrentFile(zipFile, buffer, WRITEBUFFERSIZE)) {
        fwrite(buffer, 1, j, outfile);
      }

      fclose(outfile);
      free(buffer);
    }

    unzCloseCurrentFile(zipFile);
    unzGoToNextFile(zipFile);
  }

  unzClose(zipFile);
//  remove(zipFilePath);
  return 0;
}
