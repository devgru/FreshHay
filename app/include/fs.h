#pragma once

#include <switch.h>

bool FS_FileExists(FsFileSystem *fs, const char *path);
//bool FS_DirExists(FsFileSystem *fs, const char *path);
//Result FS_MakeDir(FsFileSystem *fs, const char *path);
//Result FS_CreateFile(FsFileSystem *fs, const char *path, size_t size, int flags);
//Result FS_OpenDirectory(FsFileSystem *fs, const char *path, int flags, FsDir *dir);
//Result FS_GetDirEntryCount(FsDir *dir, u64 *count);
//Result FS_ReadDir(FsDir *dir, u64 inval, size_t *total_entries, size_t max_entries, FsDirectoryEntry *entry);
Result FS_GetFileSize(FsFileSystem *fs, const char *path, s64 *size);
//Result FS_RemoveFile(FsFileSystem *fs, const char *path);
//Result FS_RemoveDir(FsFileSystem *fs, const char *path);
//Result FS_RemoveDirRecursive(FsFileSystem *fs, const char *path);
//Result FS_RenameFile(FsFileSystem *fs, const char *old_filename, const char *new_filename);
//Result FS_RenameDir(FsFileSystem *fs, const char *old_dirname, const char *new_dirname);
Result FS_ReadFile(FsFileSystem *fs, const char *path, size_t size, void *buf);
//Result FS_WriteFile(FsFileSystem *fs, const char *path, const void *buf, bool flush);
//Result FS_SetArchiveBit(FsFileSystem *fs, const char *path);
//Result FS_OpenBisFileSystem(FsFileSystem *fs, u32 partition_ID, const char *string);
//const char *FS_GetFileExt(const char *filename);
//char *FS_GetFilePermission(const char *filename);
