#include "packFunc.h"

/**
 * @brief 遍历文件夹，并记录遍历出的文件信息
 *
 * @param dirPath 需要读取的目标文件夹
 * @param fileIndex 文件目录，用于记录文件信息的vector
 * @param dirPrefix 遍历子文件夹时，该文件夹的相对前缀
 * @return 函数的执行结果
 */
bool dirTraversal(char *dirPath, std::vector<PackFile *> *fileIndex, char const *dirPrefix)
{
    char *findDir = new char[MAX_PATH];
    WIN32_FIND_DATAA *fileData = new WIN32_FIND_DATAA();
    HANDLE findFileHandle = INVALID_HANDLE_VALUE;
    PackFile *packFileMem = nullptr;

    ::strcpy(findDir, dirPath);
    ::strcat(findDir, "\\*");

    findFileHandle = ::FindFirstFileA(findDir, fileData);

    if (findFileHandle == INVALID_HANDLE_VALUE) return false;

    do
    {   //遍历文件夹文件
        if (::strcmp(fileData->cFileName, ".") == 0 || ::strcmp(fileData->cFileName, "..") == 0)    //排出.与..
        {
            continue;
        }
        else if (fileData->nFileSizeLow <= 0)
        {
            ::strcpy(::strrchr(findDir, '\\') + 1, fileData->cFileName);
            if (!::dirTraversal(findDir, fileIndex, fileData->cFileName))
            {
                return false;
            }
        }
        else
        {
            packFileMem = new PackFile();
            if (dirPrefix != nullptr)
            {
                ::strcat(packFileMem->fileName, dirPrefix);
                ::strcat(packFileMem->fileName, "\\");
            }
            ::strcat(packFileMem->fileName, fileData->cFileName);
            packFileMem->fileSize = fileData->nFileSizeLow;
            fileIndex->push_back(packFileMem);
        }
    } while (::FindNextFileA(findFileHandle, fileData));

    delete fileData;
    delete findDir;

    return true;
}

/**
 * @brief 释放fileIndex
 *
 * @param fileIndex 文件目录，用于记录文件信息的vector
 */
void clearfileIndex(std::vector<PackFile *> *fileIndex)
{
    for (auto iterator = fileIndex->begin(); iterator != fileIndex->end(); ++iterator)
    {
        delete *iterator;
    }

    fileIndex->clear();
    delete fileIndex;
}

/**
 * @brief 将一个目录中的所有文件封包，封包名是目录名
 *
 * @param dirPath 需要封包的文件夹路径
 * @param compMethod 封包要采取的压缩方式，4:zlib--deflate,7:zstd(default method),anther:no compression
 * @return 函數的执行結果
 */
bool fileTopack(char *dirPath, DWORD compMethod)
{
    DWORD errorCode = 0;                      // 错误码
    char createFileName[MAX_PATH] = {0};      // 要创建的封包名
    BYTE magic[4] = {0x50, 0x41, 0x43, 0x75}; // pac封包的文件头，最后一位随意
    DWORD packFileCount = 0;                  // 封包文件计数
    char basePath[MAX_PATH] = {0};  //遍历文件夹时，目标文件夹及其子文件夹的基路径
    char *writeFileName = nullptr;  //要写入封包的文件的全路径
    char *extension = nullptr;  //要写入封包的文件的拓展名
    DWORD compSize = 0; //要写入封包的文件的压缩后size
    std::vector<BYTE>* bitIndexBuffer = new std::vector<BYTE>();    //编码后huffmanTree+目录的缓冲区
    DWORD bitIndexBufferSize=0; //编码后缓冲区的大小
    BYTE byteVal=0;
    std::vector<PackFile *> *fileIndex = new std::vector<PackFile *>(); //文件目录
    FILE *packStream=nullptr;   //生成封包的stream
    Huffman *huffman=nullptr;   

    if (!::dirTraversal(dirPath, fileIndex, nullptr)) // 遍历文件夹,记录文件夹中的所有文件
    {
        errorCode = ::GetLastError();
        ::printf("sorry,findFileError,errorCode is %d", errorCode);
        return false;
    }

    ::printf("now loading……\n");

    ::strcpy(createFileName, dirPath);
    ::strcat(createFileName, ".pac"); // 以目标文件夹为名，生成一个封包
    packFileCount = fileIndex->size();

    packStream = ::fopen(createFileName, "wb+"); // 创建封包stream

    ::fwrite(magic, 4, 1, packStream);          // 写入文件头
    ::fwrite(&packFileCount, 4, 1, packStream); // 写入封包文件计数
    ::fwrite(&compMethod, 4, 1, packStream);    // 写入封包的压缩方式

    ::strcpy(basePath, dirPath);
    ::strcat(basePath, "\\"); // 构建base路径
    writeFileName = ::strrchr(basePath, '\\') + 1;
    for (auto iterator = fileIndex->begin(); iterator != fileIndex->end(); ++iterator)
    {
        bool extenFlag = true;      // 当前文件是否需要压缩
        FILE *curFile = nullptr;    // 当前文件的stream
        BYTE *fileBuffer = nullptr; // 文件原数据缓冲区
        BYTE *compBuffer = nullptr; // 文件压缩数据缓冲区

        extension = ::strrchr((*iterator)->fileName, '.') + 1;                                                                                                                 // 获取文件后缀名
        extenFlag = (::strcmp(extension, "ogg") == 0 || ::strcmp(extension, "png") == 0 || ::strcmp(extension, "wav") == 0 || ::strcmp(extension, "fnt") == 0) ? false : true; // 判断文件的后缀名，如果文件属于已压缩文件，该文件不需要压缩

        ::strcpy(writeFileName, (*iterator)->fileName); // base路径+相对文件名=当前目标文件的全路径
        curFile = ::fopen(basePath, "rb+");
        (*iterator)->offsetInPack = ::ftell(packStream); // 记录文件在封包的offset
        fileBuffer = new BYTE[(*iterator)->fileSize];
        ::fread(fileBuffer, (*iterator)->fileSize, 1, curFile); // 读取文件原数据
        if (compMethod == 4 && extenFlag)                       // 压缩文件
        {
            compSize = ::compressBound((*iterator)->fileSize);
            compBuffer = new BYTE[compSize];
            ::compress(compBuffer, &compSize, fileBuffer, (*iterator)->fileSize);
            (*iterator)->compSize = compSize; // 记录文件压缩后的size
        }
        else if (compMethod == 7 && extenFlag)
        {
            compSize = ::ZSTD_compressBound((*iterator)->fileSize);
            compBuffer = new BYTE[compSize];
            ::ZSTD_compress(compBuffer, compSize, fileBuffer, (*iterator)->fileSize, ::ZSTD_defaultCLevel());
            (*iterator)->compSize = compSize; // 记录文件压缩后的size
        }
        else
        {
            (*iterator)->compSize = (*iterator)->fileSize;
            compBuffer = new BYTE[(*iterator)->compSize]; // 记录文件压缩后的size
            memcpy(compBuffer, fileBuffer, (*iterator)->fileSize);
        }
        ::fwrite(compBuffer, (*iterator)->compSize, 1, packStream); // 将压缩后的数据写入封包

        delete compBuffer;
        delete fileBuffer;
        ::fclose(curFile);
    }

    huffman = new ::Huffman();
    huffman->encode(fileIndex,bitIndexBuffer);

    for(auto iter=bitIndexBuffer->begin();iter!=bitIndexBuffer->end();++iter){
        byteVal=~(*iter);
        ::fwrite(&byteVal,1,1,packStream);
    }
    bitIndexBufferSize=bitIndexBuffer->size();
    ::fwrite(&bitIndexBufferSize,4,1,packStream);

    delete huffman;
    delete bitIndexBuffer;
    ::fclose(packStream);
    ::clearfileIndex(fileIndex);
    return true;
}