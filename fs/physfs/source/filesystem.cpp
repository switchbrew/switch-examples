#include "filesystem.h"

#include <switch.h>

#include <cstring>

const char* FileSystem::GetPhysfsError()
{
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

std::string FileSystem::Normalize(const std::string& input)
{
    std::string out;
    bool seenSep = false, isSep = false;

    for (size_t i = 0; i < input.size(); ++i)
    {
        isSep = (input[i] == '/');

        if (!isSep || !seenSep)
            out += input[i];

        seenSep = isSep;
    }

    return out;
}

void FileSystem::Initialize()
{
    FileSystem::savePath = "";
}

bool FileSystem::SetSource(const char* source)
{
    if (!PHYSFS_isInit())
        return false;

    std::string searchPath = source;
    if (!PHYSFS_mount(searchPath.c_str(), NULL, 1))
        return false;

    return true;
}

bool FileSystem::SetIdentity(const char* name, bool append)
{
    if (!PHYSFS_isInit())
        return false;

    std::string old = FileSystem::savePath;

    FileSystem::savePath = FileSystem::Normalize(FileSystem::GetUserDirectory() + "/save/" + name);
    printf("Save Path set to %s\n", savePath.c_str());

    if (!old.empty())
        PHYSFS_unmount(old.c_str());

    int success = PHYSFS_mount(savePath.c_str(), NULL, append);
    printf("Save Path mounted %d\n", success);

    PHYSFS_setWriteDir(nullptr);

    return true;
}

std::string FileSystem::GetSaveDirectory()
{
    return FileSystem::Normalize(FileSystem::GetUserDirectory() + "/save");
}

bool FileSystem::SetupWriteDirectory()
{
    if (!PHYSFS_isInit())
        return false;

    if (FileSystem::savePath.empty())
        return false;

    std::string tmpWritePath     = FileSystem::savePath;
    std::string tmpDirectoryPath = FileSystem::savePath;

    if (FileSystem::savePath.find(FileSystem::GetUserDirectory()) == 0)
    {
        tmpWritePath     = FileSystem::GetUserDirectory();
        tmpDirectoryPath = savePath.substr(FileSystem::GetUserDirectory().length());

        /* strip leading '/' characters from the path we want to create */
        size_t startPosition = tmpDirectoryPath.find_first_not_of('/');

        if (startPosition != std::string::npos)
            tmpDirectoryPath = tmpDirectoryPath.substr(startPosition);
    }

    if (!PHYSFS_setWriteDir(tmpWritePath.c_str()))
    {
        printf("Failed to set write dir to %s\n", tmpWritePath.c_str());
        return false;
    }

    if (!FileSystem::CreateDirectory(tmpDirectoryPath.c_str()))
    {
        printf("Failed to create dir %s\n", tmpDirectoryPath.c_str());
        /* clear the write directory in case of error */
        PHYSFS_setWriteDir(nullptr);
        return false;
    }

    if (!PHYSFS_setWriteDir(savePath.c_str()))
    {
        printf("Failed to set write dir to %s\n", savePath.c_str());
        return false;
    }

    if (!PHYSFS_mount(savePath.c_str(), nullptr, 0))
    {
        printf("Failed to mount write dir (%s)\n", FileSystem::GetPhysfsError());
        /* clear the write directory in case of error */
        PHYSFS_setWriteDir(nullptr);
        return false;
    }

    return true;
}

std::string FileSystem::GetUserDirectory()
{
    return FileSystem::Normalize(PHYSFS_getUserDir());
}

bool FileSystem::GetInfo(const char* filename, FileSystem::Info& info)
{
    if (!PHYSFS_isInit())
        return false;

    PHYSFS_Stat stat = {};

    if (!PHYSFS_stat(filename, &stat))
        return false;

    info.mod_time = std::min<int64_t>(stat.modtime, FileSystem::MAX_STAMP);
    info.size     = std::min<int64_t>(stat.filesize, FileSystem::MAX_STAMP);

    if (stat.filetype == PHYSFS_FILETYPE_REGULAR)
        info.type = FileSystem::FileType_File;
    else if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
        info.type = FileSystem::FileType_Directory;
    else if (stat.filetype == PHYSFS_FILETYPE_SYMLINK)
        info.type = FileSystem::FileType_SymLink;
    else
        info.type = FileSystem::FileType_Other;

    return true;
}

void FileSystem::GetDirectoryItems(const char* path, std::vector<std::string>& items)
{
    if (!PHYSFS_isInit())
        return;

    char** results = PHYSFS_enumerateFiles(path);

    if (results == nullptr)
        return;

    for (char** item = results; *item != 0; item++)
        items.push_back(*item);

    PHYSFS_freeList(results);
}

bool FileSystem::OpenFile(File& file, const char* name, FileMode mode)
{
    if (mode == FileMode_Closed)
        return false;

    if (!PHYSFS_isInit())
        return false;

    if (file.handle)
        FileSystem::CloseFile(file);

    if (mode == FileMode_Read && !PHYSFS_exists(name))
    {
        printf("Could not open file %s, does not exist.\n", name);
        return false;
    }

    if ((mode == FileMode_Write) &&
        (PHYSFS_getWriteDir() == nullptr && FileSystem::SetupWriteDirectory()))
    {
        printf("Could not set write directory.\n");
        return false;
    }

    PHYSFS_getLastErrorCode();

    switch (mode)
    {
        case FileMode_Read:
            file.handle = PHYSFS_openRead(name);
            break;
        case FileMode_Write:
            file.handle = PHYSFS_openWrite(name);
            break;
        default:
            break;
    }

    if (!file.handle)
    {
        const char* error = FileSystem::GetPhysfsError();

        if (error == nullptr)
            error = "unknown error";

        printf("Could not open file %s (%s)\n", name, error);

        return false;
    }

    file.mode = mode;

    return true;
}

bool FileSystem::CloseFile(File& file)
{
    if (file.handle == nullptr || !PHYSFS_close(file.handle))
        return false;

    file.handle = nullptr;

    return true;
}

bool FileSystem::CreateDirectory(const char* name)
{
    if (!PHYSFS_isInit())
        return false;

    if (PHYSFS_getWriteDir() == nullptr && !FileSystem::SetupWriteDirectory())
        return false;

    if (!PHYSFS_mkdir(name))
        return false;

    return true;
}

int64_t FileSystem::ReadFile(File& file, void* destination, int64_t size)
{
    if (!file.handle || file.mode != FileMode_Read)
    {
        printf("File is not opened for reading.\n");
        return 0;
    }

    if (size > file.GetSize())
        size = file.GetSize();
    else if (size < 0)
    {
        printf("Invalid read size %lld\n", size);
        return 0;
    }

    return PHYSFS_readBytes(file.handle, destination, (PHYSFS_uint64)size);
}

bool FileSystem::WriteFile(File& file, const void* data, int64_t size)
{
    if (!file.handle || file.mode != FileMode_Write)
    {
        printf("File is not opened for writing.\n");
        return false;
    }

    int64_t written = PHYSFS_writeBytes(file.handle, data, (PHYSFS_uint64)size);

    if (written != size)
        return false;

    return true;
}
