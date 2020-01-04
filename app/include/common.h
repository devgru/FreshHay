#pragma once

#include <switch.h>
#include <setjmp.h>

#define ROOT_PATH "/"
#define START_PATH ROOT_PATH
#define MAX_FILES 2048
#define FILES_PER_PAGE 8
#define wait(msec) svcSleepThread(10000000 * (s64)msec)

extern jmp_buf exitJmp;

extern FsFileSystem *fs;
extern FsFileSystem devices[4];
extern u64 total_storage, used_storage;

extern char cwd[FS_MAX_PATH];
