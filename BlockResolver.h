#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <condition_variable>
#include "Messages.h"
#include "SafeQueue.h"

#define MAX_TOTAL_BLOCK_SIZE 100 * 1024 * 1024 

class Connection;

struct ActiveUsersOfBuffer
{
    ActiveUsersOfBuffer()
        : counter(0) 
    {  
    }

    char * buffer;
    size_t   size;

    uint  counter;
    std::vector<std::shared_ptr<Connection>> connections;
};

typedef std::pair<std::shared_ptr<Connection>, std::string> conn_hash;
typedef std::map<uint, ActiveUsersOfBuffer> BlocksInUSE;

class SendingData
{
public:
    bool GetBlockById        (const uint id, char * retBuf);
    void AddConnectionToBlock(const uint id, char * readyBuf, size_t size);
    void DeleteConnection    (const uint id);

    const size_t GetSize();

private:
    void NotifyDelete();

    std::mutex _mtx;
    std::condition_variable _cv;
    bool _deleteNotified = false;

    BlocksInUSE _blocksInUse;
    size_t _totalSizeInUse = 0; 
};

class BlockResolver
{
public:
    void AddQuerry(std::shared_ptr<Connection> & conn, std::string & hash);
    void DeleteFromSending(const uint & id);

    void Resolve();
    void BasicResolve();

    void Dump();
        
private:
    SendingData _readyBlocks;
    SafeQueue<conn_hash> hashClientQueue;
};

