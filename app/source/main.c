#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <switch.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include <jansson.h>

#include "common.h"
#include "fs.h"
#include "unzip.h"
#include "mega.h"

/*
u64 GetSdTotalSpace() {
    s64 sz = 0;
    fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sdmc"), "/", &sz);
    return (u64) sz;
}

u64 GetSdFreeSpace() {
    s64 sz = 0;
    fsFsGetFreeSpace(fsdevGetDeviceFileSystem("sdmc"), "/", &sz);
    return (u64) sz;
}
*/

char *concat(const char *s1, const char *s2) {
  const size_t len1 = strlen(s1);
  const size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1); // +1 for the null-terminator
  // in real code you would check for errors in malloc here
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator
  return result;
}

bool startDownloadingArchive = false;
u32 currentVersion;

u8 targetMajorVersion;
u8 targetMinorVersion;
u8 targetPatchVersion;
u8 currentMajorVersion;
u8 currentMinorVersion;
u8 currentPatchVersion;
const char *targetVersion;

const char *versionPattern = "%d.%d.%d";
const char *TARGET_CONFIGURATION = "/switch/FreshHay/target.json";

char *fileId;
char *fileKey;

Result readTargetFile() {
  Result ret = 0;

  if (!FS_FileExists(fs, TARGET_CONFIGURATION)) {
    return 0;
  }
  s64 size;
  FS_GetFileSize(fs, TARGET_CONFIGURATION, &size);
  char *buffer = (char *) malloc(size + 1);

  if (R_FAILED(ret = FS_ReadFile(fs, TARGET_CONFIGURATION, size, buffer))) {
    free(buffer);
    return ret;
  }
  buffer[size] = '\0';

  json_error_t error;
  json_t *root = json_loads(buffer, 0, &error);

  targetVersion = json_string_value(json_object_get(root, "version"));
  const char *archiveLink = json_string_value(json_object_get(root, "url"));

  sscanf(targetVersion, versionPattern, &targetMajorVersion, &targetMinorVersion, &targetPatchVersion);

  char *newFileId = (char *) malloc(9);
  strncpy(newFileId, archiveLink + 18, 8);
  newFileId[8] = '\0';
  char *newFileKey = (char *) malloc(44);
  strncpy(newFileKey, archiveLink + 27, 43);
  newFileKey[43] = '\0';
  fileId = newFileId;
  fileKey = newFileKey;

  free(buffer);
  printf("Available firmware version: %d.%d.%d\n\n", targetMajorVersion, targetMinorVersion, targetPatchVersion);
  return ret;
}

int main(int argc, char *argv[]) {
  json_auto_t *value = NULL;

  socketInitializeDefault();
  curl_global_init(CURL_GLOBAL_DEFAULT);

  Result ret = 0;

  devices[0] = *fsdevGetDeviceFileSystem("sdmc");
  fs = &devices[0];
  consoleInit(NULL);

  printf("Fresh Hay v0.1.0\n\n");
  currentVersion = hosversionGet();
  currentMajorVersion = HOSVER_MAJOR(currentVersion);
  currentMinorVersion = HOSVER_MINOR(currentVersion);
  currentPatchVersion = HOSVER_MICRO(currentVersion);
  printf("Current firmware version: %d.%d.%d\n\n", currentMajorVersion, currentMinorVersion, currentPatchVersion);

  if (R_FAILED(ret = readTargetFile())) {
    goto exit;
  }
  if (targetMajorVersion == 0) {
    ret = 0;

    printf("Fresh Hay needs settings at %s\n", TARGET_CONFIGURATION);
    printf("Keys are:\n");
    printf("- version = version number as string e.g. \"9.1.0\"\n");
    printf("- target = mega.nz url to download archive with specified version\n");
    printf("\nPress + to exit.\n");

    while (appletMainLoop()) {
      hidScanInput();
      u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

      if (kDown & KEY_PLUS) {
        break;
      }
      consoleUpdate(NULL);
    }
    goto exit;
  }

  bool noNeedToUpdate = targetMajorVersion == currentMajorVersion &&
                        targetMinorVersion == currentMinorVersion &&
                        targetPatchVersion == currentPatchVersion;

  if (noNeedToUpdate) {
    printf("Your firmware is fresh enough, no need to download anything. Press + to exit.\n");
    while (appletMainLoop()) {
      hidScanInput();
      u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

      if (kDown & KEY_PLUS) {
        break;
      }
      consoleUpdate(NULL);
    }
    goto exit;
  }
  u32 ip = gethostid();

  if (ip == INADDR_LOOPBACK) {
    printf("Waiting for internet connection. Are you in airplane mode?\n");
  } else {
    printf("Press A to start. Press + to exit.\n\n");
  }

  while (appletMainLoop()) {
    hidScanInput();
    u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

    if (ip == INADDR_LOOPBACK && (gethostid() != INADDR_LOOPBACK)) {
      ip = gethostid();
      printf("Internet connection restored, proceeding.\n\n");
      printf("Press A to start. Press + to exit.\n\n");
    }

    if (ip != INADDR_LOOPBACK && (kDown & KEY_A) && !startDownloadingArchive) {
      startDownloadingArchive = true;
      char *path = concat("/switch/FreshHay/", targetVersion);
      char *archiveLocation = concat(path, ".zip");
      char *firmwareDirectory = concat(path, "/");
      long responseCode = downloadArchive(fileId, fileKey, archiveLocation);

      if (responseCode >= 400 || !FS_FileExists(fs, archiveLocation)) {
        printf("Error occurred while downloading archive, retry later. Press + to exit \n");
      } else {
        if (!FS_FileExists(fs, firmwareDirectory)) {
          mkdir(firmwareDirectory, 0777);
        }
        chdir(firmwareDirectory);
        unzip(archiveLocation);
        printf("\n\nSuccess!");
        printf("\nPoint ChoiDujourNX to %s, we have some food for them.", firmwareDirectory);
        printf("\n\nAfter updating firmware consider removing");
        printf("\n- %s", firmwareDirectory);
        printf("\n- %s", archiveLocation);
        printf("\n\nAll done, press + to exit. Bye!");
      }

    }
    if (kDown & KEY_PLUS) {
      break;
    }
    consoleUpdate(NULL);
  }

  exit:
  consoleExit(NULL);
  appletSetAutoSleepDisabled(false);
  socketExit();
  return ret;
}
