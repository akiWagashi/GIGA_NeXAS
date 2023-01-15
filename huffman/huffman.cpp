#include "huffman.h"

/**
 * @brief 二进制字符串转十进制
 * 
 * @param str 要转化的二进制字符串
 * @return DWORD 转化结果
 */
DWORD binTodec(const char* const str){
    DWORD result = 0;
    DWORD len=::strlen(str);
    for(DWORD i=0;i<len;i++){
        result=result*2+str[i]-0x30;
    }
    return result;
}

/**
 * @brief 二进制字符串转十进制
 * 
 * @param str 要转化的二进制字符串
 * @param strLen 读取长度
 * @return DWORD 转化结果
 */
DWORD binTodec(const char* const str,DWORD strLen){
    DWORD result = 0;
    for(DWORD i=0;i<strLen;i++){
        result=result*2+str[i]-0x30;
    }
    return result;
}

Huffman::Huffman()
{
}

Huffman::Huffman(BYTE *buffer, DWORD bufferSize) : buffer(buffer), bufferSize(bufferSize)
{
    HuffmanNode *root = new HuffmanNode; // 生成一个根节点
    this->root = root;
}

Huffman::~Huffman()
{
    if (this->root != nullptr)
        this->destroyHuffmanNode(this->root);
}

/**
 * @brief 向buffer中写入指定位bit
 *
 * @param buffer 目标缓冲区
 * @param setbits 要设置的bit数
 * @param val 要设置的值
 * @return 函数执行结果
 */
bool Huffman::setBits(std::vector<BYTE> *buffer, DWORD setbit, DWORD val)
{
    DWORD writeBit=0;

    while (setbit > 0)
    {
        if (!this->bitCount)
        {
            if(this->byteCount){
            buffer->push_back(this->val);
            }else{
                this->byteCount++;
            }
            this->bitCount = 8;
            this->val = 0;
        }
        writeBit= this->bitCount<setbit ? this->bitCount : setbit;
        if(setbit>this->bitCount){
            this->val|=(val>>(setbit-this->bitCount));
            val&=(1<<(setbit-this->bitCount))-1;
        }else{
            this->val|=(val<<(this->bitCount-setbit));
        }

        setbit-=writeBit;
        this->bitCount-=writeBit;
    }

    return true;
}

/**
 * @brief 按照前缀码字符串将指定位bit写入buffer
 * 
 * @param buffer 目标缓冲区
 * @param setbits 要设置的bit数
 * @param path 要写入的前缀码字符串
 * @return 函数执行结果
 */
bool Huffman::setBits(std::vector<BYTE>* buffer,DWORD setbit,const char* const path){
    DWORD loop=setbit/32;
    for(int i=0;i<=loop;i++){
        if(i<loop){
            this->setBits(buffer,32,binTodec(path+i*32,32));
        }else{
            this->setBits(buffer,setbit-i*32,binTodec(path+i*32));
        }
    }
    return true;
}

/**
 * @brief 从buffer中读取指定位数的bit
 *
 * @param getBit  要读取的bit数
 * @param bitVal  用于存放读取结果的指针
 * @return 函数执行结果
 */
bool Huffman::getBits(DWORD getBit, _Out_ DWORD *bitVal)
{
    DWORD readBit = 0; // 每次需要读取的bit数

    while (getBit > 0) // 判断是否已经读取需要的bit数
    {
        if (!this->bitCount) // 判断当前byte是否还有bit可读
        {
            if (this->byteCount >= this->bufferSize) // 当前stream缓冲区是否已被读取完毕
            {
                return false;
            }
            this->val = this->buffer[this->byteCount++]; // 读取当前指向byte的value，并令index+1;
            this->bitCount = 8;                          // 可读bit数置为8;
        }
        readBit = getBit > this->bitCount ? this->bitCount : getBit;// 判断当前byte的可用bit是否满足要读取的bit数

        *bitVal <<= readBit;                                // 分byte时移位
        *bitVal |= this->val >> (this->bitCount - readBit); // 用位移的方式读取bit值
        this->val &= (1 << (this->bitCount - readBit)) - 1; // 将已读的bit抹0，防止对后续的读取结果产生影响

        this->bitCount -= readBit; // 数据更新
        getBit -= readBit;
    }

    return true;
}

/**
 * @brief huffmanTree转bitstream
 * 
 * @param[in] buffer
 * @param[in] node 
 * @return 函数执行结果
 */
bool Huffman::huffmanTreeToBitStream(std::vector<BYTE>* buffer,HuffmanNode* node){
    if(node->leftNode==nullptr&&node->rightNode==nullptr){
        this->setBits(buffer,1,(DWORD)0);
        this->setBits(buffer,8,node->symbol);
    }else{
        this->setBits(buffer,1,1);
        this->huffmanTreeToBitStream(buffer,node->leftNode);
        this->huffmanTreeToBitStream(buffer,node->rightNode);
    }

    return true;
}


/**
 * @brief bitStream转huffmanTree
 * @param node huffmanNode指针，用于做父节点或做叶节点获值
 *
 * @return 函数执行结果
 */
bool Huffman::bitStreamToHuffmanTree(HuffmanNode *node)
{

    DWORD bitVal = 0;

    if (!this->getBits(1, &bitVal))
        false;

    if (bitVal)
    { // 根据bit值判断节点是否为叶节点

        HuffmanNode *leftNode = new HuffmanNode;
        this->bitStreamToHuffmanTree(leftNode);
        node->leftNode = leftNode;

        HuffmanNode *rightNode = new HuffmanNode;
        this->bitStreamToHuffmanTree(rightNode);
        node->rightNode = rightNode;
    }
    else
    {                     // 读取到0时，表明该节点是一个叶节点，读取8bit将其作为节点值
        bitVal ^= bitVal; // bitval清0

        if (!this->getBits(8, &bitVal))
            return false;

        node->symbol = (BYTE)bitVal;
    }

    return true;
}

/**
 * @brief 释放一个huffmanNode
 *
 * @param node huffmanNode指针
 * @return 函数执行结果
 */
bool Huffman::destroyHuffmanNode(HuffmanNode *node)
{
    if (node == nullptr)
        return false;
    if(node->path!=nullptr) delete node->path;
    
    if (node->leftNode != nullptr)
        this->destroyHuffmanNode(node->leftNode);

    if (node->rightNode != nullptr)
        this->destroyHuffmanNode(node->rightNode);

    delete node;
    return true;
}

/**
 * @brief 将bitStram形式的huffmanTree解码为fileIndex
 *
 * @param decodeBuffer 存放解码结果的缓冲区
 * @param decodeBufferSize 缓冲区大小
 * @param readByte 实际解码出的字节数
 * @return 函数执行结果
 */
bool Huffman::decode(BYTE *decodeBuffer, DWORD decodeBufferSize, _Out_ DWORD *readByte)
{
    if (!this->bitStreamToHuffmanTree(this->root))
    {
        ::printf("Huffman tree creation failed\n");
        return false;
    }

    HuffmanNode *node = this->root;

    DWORD bitVal = 0;

    while (this->getBits(1, &bitVal))
    {

        if (*readByte >= decodeBufferSize)
            break;

        if (!bitVal)
        {
            node = node->leftNode;
        }
        else
        {
            node = node->rightNode;
        }

        bitVal ^= bitVal;

        if (node->leftNode == nullptr && node->rightNode == nullptr)
        {
            decodeBuffer[(*readByte)++] = node->symbol;
            node = this->root;
        }
    }

    return true;
}

/**
 * @brief 构造huffmantree节点的编码
 *
 * @param[in] node 父节点
 * @param[in] parentPath 父节点的编码
 * @param[in] branch 本节点要追加的编码
 * @return 函数执行结果
 */
bool createHuffmanNodePath(HuffmanNode *node, const char *const parentPath, const char branch)
{

    if (branch != NULL)
    {
        int parPathLen = parentPath == nullptr ? 0 : strlen(parentPath);
        node->path = new char[parPathLen + 2]();    //多1byte用作\0
        if (parentPath != nullptr) strcpy(node->path, parentPath);  //防止根节点
        (node->path)[parPathLen] = branch;
    }
    if (node->leftNode != nullptr && node->rightNode != nullptr)
    {
        ::createHuffmanNodePath(node->leftNode, node->path, '0');
        ::createHuffmanNodePath(node->rightNode, node->path, '1');
    }

    return true;
}

/**
 * @brief 将fileIndex编码为bitStream的huffmanTree
 *
 * @param[in] fileIndex 文件目录，用于记录文件信息的vector
 * @param[out] encodeBuffer 存储编码后bitStream的地址
 * @return 函数执行结果
 */
bool Huffman::encode(std::vector<PackFile *> *fileIndex, _Out_ std::vector<BYTE> *encodeBuffer)
{
    DWORD *nodeTable = new DWORD[256]();
    HuffmanNode *node = nullptr;
    std::vector<HuffmanNode *> *nodeList = new std::vector<HuffmanNode *>();
    std::vector<HuffmanNode *>::iterator nodeIter;

    for (auto fileIter = fileIndex->begin(); fileIter != fileIndex->end(); ++fileIter)
    {
        BYTE *basePointer = (BYTE *)*fileIter;
        for (int offset = 0; offset < 0x4C; offset++)
            nodeTable[*(basePointer + offset)]++;
    }

    for (int index = 0; index < 256; ++index)
    {
        if (nodeTable[index] != 0)
        {
            HuffmanNode *node = new HuffmanNode();
            node->symbol = index;
            node->weight = nodeTable[index];
            nodeList->push_back(node);
        }
    }

    if(nodeList->size()>2) std::sort(nodeList->begin(), nodeList->end(), [](HuffmanNode *sun, HuffmanNode *moon) -> bool{ return sun->weight > moon->weight; });

    std::deque<HuffmanNode*> *nodeDeque=new std::deque<HuffmanNode*>(nodeList->begin(),nodeList->end());

    while (nodeDeque->size()>1)
    {  
        node=new HuffmanNode();
        HuffmanNode* regNode1=nodeDeque->back();
        nodeDeque->pop_back();
        HuffmanNode* regNode2=nodeDeque->back();
        nodeDeque->pop_back();
        node->leftNode=regNode1;
        node->rightNode=regNode2;
        node->weight=regNode1->weight+regNode2->weight;
        if(nodeDeque->size()>1){
            if(node->weight>(*nodeDeque)[nodeDeque->size()-1]->weight+(*nodeDeque)[nodeDeque->size()-2]->weight){
                nodeDeque->insert(nodeDeque->end()-3,node);
                continue;
            }
        }
        nodeDeque->push_back(node);
    }
    
    this->root = node;

    ::createHuffmanNodePath(this->root, this->root->path, NULL);

    this->huffmanTreeToBitStream(encodeBuffer,this->root);

    std::map<BYTE,HuffmanNode*>* nodeMap=new std::map<BYTE,HuffmanNode*>();

    for(nodeIter=nodeList->begin();nodeIter!=nodeList->end();++nodeIter) nodeMap->insert(std::pair<BYTE,HuffmanNode*>((*nodeIter)->symbol,*nodeIter));

    for (auto fileIter = fileIndex->begin(); fileIter != fileIndex->end(); ++fileIter)
    {
        BYTE *basePointer = (BYTE*)*fileIter;
        for (int offset = 0; offset < 0x4C; ++offset)
        {
            node=(*nodeMap)[basePointer[offset]];
            this->setBits(encodeBuffer,strlen(node->path),node->path);
        }
    }

    encodeBuffer->push_back(this->val);

    delete nodeMap;
    delete nodeDeque;
    delete nodeList;
    delete nodeTable;

    return true;
}