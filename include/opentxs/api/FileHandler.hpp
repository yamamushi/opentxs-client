#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include <opentxs/api/WinsockWrapper.h>
#include <opentxs/api/ExportWrapper.h>

#include <opentxs/core/String.hpp>


class FileHandler
{
public:

    /*check if the file exist*/
    bool isFileExist(String fileName);
    /*remove file*/
    bool removeFile(String fileName);
};

#endif // FILEHANDLER_HPP
