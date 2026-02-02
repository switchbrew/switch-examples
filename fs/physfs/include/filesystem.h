#pragma once

#include <physfs.h>

#include <string>
#include <vector>

namespace FileSystem
{
    static constexpr auto MAX_STAMP = 0x20000000000000LL;

    enum FileMode
    {
        FileMode_Open,
        FileMode_Read,
        FileMode_Write,
        FileMode_Closed
    };

    enum FileType
    {
        FileType_File,
        FileType_Directory,
        FileType_SymLink,
        FileType_Other
    };

    struct File
    {
        PHYSFS_file* handle;
        FileMode mode;

        File()
        {
            this->handle = nullptr;
            this->mode   = FileMode_Closed;
        }

        int64_t GetSize()
        {
            if (this->handle == nullptr)
                return 0;

            return (int64_t)PHYSFS_fileLength(this->handle);
        }
    };

    struct Info
    {
        int64_t size;
        int64_t mod_time;
        FileType type;
    };

    void Initialize();

    /*
    ** mounts a specific directory for physfs to search in
    ** this is typically a main directory
    */
    bool SetSource(const char* source);

    /*
    ** mounts a specific directory as a "save" directory
    ** if appended, it will be added to the search path
    */
    bool SetIdentity(const char* name, bool append);

    static std::string savePath;

    /* gets the last physfs error */
    const char* GetPhysfsError();

    /* strips any duplicate slashes */
    std::string Normalize(const std::string& input);

    /* gets the user directory from physfs */
    std::string GetUserDirectory();

    /* gets the save directory */
    std::string GetSaveDirectory();

    /* sets up the writing directory for physfs */
    bool SetupWriteDirectory();

    /* gets a list of files in a directory */
    void GetDirectoryItems(const char* directory, std::vector<std::string>& items);

    /* gets the size, mod_time, and type of a file */
    bool GetInfo(const char* filename, Info& info);

    /* creates a new directory */
    bool CreateDirectory(const char* name);

    bool CloseFile(File& file);

    /* creates a new file */
    bool OpenFile(File& file, const char* name, FileMode mode);

    /* writes to a file */
    bool WriteFile(File& file, const void* data, int64_t size);

    /* reads a file's content */
    int64_t ReadFile(File& file, void* destination, int64_t size);
}
