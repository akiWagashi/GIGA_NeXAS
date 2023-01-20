#include "packFunc.h"

/**
 * @brief 根据目录导出封包中的所有文件
 * 
 * @param pack 封包stream
 * @param fileList 封包的文件目录
 * @param fileCount 封包的文件计数
 * @param compMethod 封包的加密方式
 * @param packPath 封包的绝对路径
 * @return 函數的执行结果
 */
bool fileExtract(FILE *pack, PackFile *fileList, DWORD fileCount, DWORD compMethod, const char *packPath)
{
    char extractPath[MAX_PATH] = {0};
    ::strncpy(extractPath, packPath, (int)::strrchr(packPath, '.') - (int)packPath);    //以封包名作为新路径名
    if (GetFileAttributesA(extractPath) == INVALID_FILE_ATTRIBUTES) //没有文件夹则创建
        CreateDirectoryA(extractPath, NULL);
    ::strcat(extractPath, "\\");
    ::printf("now loading……\n");

    for (DWORD i = 0; i < fileCount; i++)
    {
        char fileFullPath[MAX_PATH] = {0};  //封包文件的导出路径
        BYTE *compBuffer = new BYTE[fileList[i].compSize];  
        BYTE *uncompBuffer = new BYTE[fileList[i].fileSize];

        ::fseek(pack, fileList[i].offsetInPack, SEEK_SET);  
        ::fread(compBuffer, fileList[i].compSize, 1, pack);

        if (fileList[i].compSize == fileList[i].fileSize||(compMethod!=4&&compMethod!=7))   //判断文件是否被压缩
        {
            ::memcpy(uncompBuffer, compBuffer, fileList[i].compSize);   //未被压缩则直接复制源缓冲区
        }
        else
        {
            if (compMethod == 7)    //zstd
            {
                size_t retCode=ZSTD_decompress(uncompBuffer,fileList[i].fileSize,compBuffer,fileList[i].compSize);
                if(ZSTD_isError(retCode)){
                    ::printf("file %s hava error,error is %s\n",fileList[i].fileName,ZSTD_getErrorName(retCode));
                    delete uncompBuffer;
                    delete compBuffer;
                    continue;
                }
            }
            else if (compMethod == 4)   //zlib--deflate
            {
                uncompress((Bytef*)uncompBuffer,&fileList[i].fileSize,(Bytef*)compBuffer,fileList[i].compSize);
            }
        }

        ::strcpy(fileFullPath,extractPath);
        ::strcat(fileFullPath,fileList[i].fileName);
        FILE* extractFile=::fopen(fileFullPath,"wb");
        ::fwrite(uncompBuffer,fileList[i].fileSize,1,extractFile);
        ::fclose(extractFile);

        delete uncompBuffer;
        delete compBuffer;
    }
    return true;
}

/**
 * @brief 解包一个封包
 *
 * @param packPath 封包的绝对路径
 * @return 函数执行结果
 */
bool fileUnpack(char *packPath)
{

    BYTE magic[4]{0}; // 封包头缓冲区

    DWORD packFileCount = 0;   // 封包内文件计数
    DWORD packCompMethod = 0;  // 封包采用的压缩算法
    DWORD compedIndexSize = 0; // 封包被压缩后的文件目录的大小
    DWORD readByte = 0;        // 解码的已读字节

    FILE *pack = ::fopen(packPath, "rb");

    if (pack == nullptr)
    {
        ::printf("Sorry,pack open failed\n");
        return false;
    }

    ::fread(magic, 4, 1, pack);

    if (magic[0] != 0x50 || magic[1] != 0x41 || magic[2] != 0x43)
    {
        ::printf("Sorry,the packet format does not conform to the specification\n");
        ::fclose(pack);
        return false;
    }

    ::fread(&packFileCount, 4, 1, pack); // 读取封包文件数

    ::fread(&packCompMethod, 4, 1, pack); // 读取封包压缩算法

    ::printf("There are %d files in this pack\n", packFileCount);

    ::fseek(pack, -4, SEEK_END); // 获取(压缩后)封包文件目录大小

    ::fread(&compedIndexSize, 4, 1, pack);

    ::fseek(pack, -4 - compedIndexSize, SEEK_END);

    BYTE *compedIndexBuffer = new BYTE[compedIndexSize]; // 压缩目录的缓冲区

    ::fread(compedIndexBuffer, compedIndexSize, 1, pack);

    for (DWORD i = 0; i < compedIndexSize; i++)
    {
        compedIndexBuffer[i] = ~compedIndexBuffer[i]; // 取反解密压缩目录
    }

    DWORD uncompIndexSize = packFileCount * 0x4C;   //计算解码后需要的缓冲区大小

    BYTE *uncompIndexBuffer = new BYTE[uncompIndexSize];

    Huffman *huffman = new ::Huffman(compedIndexBuffer, compedIndexSize);

    huffman->decode(uncompIndexBuffer, uncompIndexSize, &readByte); // 封包目录解码

    delete huffman;

    fileExtract(pack,(PackFile*)uncompIndexBuffer,packFileCount,packCompMethod,packPath);

    delete[] uncompIndexBuffer;
    delete[] compedIndexBuffer;
    fclose(pack);
    return true;
}