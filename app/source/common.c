#include "common.h"

jmp_buf exitJmp;

char cwd[FS_MAX_PATH];

FsFileSystem *fs;
FsFileSystem devices[4];
u64 total_storage = 0, used_storage = 0;
