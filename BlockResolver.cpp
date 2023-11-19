#include <iostream>
#include "BlockResolver.h"
#include "TCPHandler.h"
#include <condition_variable>

// ----------------------------

size_t get_block_number(const std::string & hash)
{
    //size_t randomNumber = std::rand() % 1000 + 1;
    //return randomNumber
}

size_t get_block_size(const std::string & hash)
{
    //srand(time(0)); 
    //size_t randomNumber = std::rand() % 1024 + 16;
    //return randomNumber;
}

int get_block_data(size_t block_num, char * buffer, size_t buffer_size)
{
    //std::memcpy(buffer, &block_num, sizeof(size_t));
    //for (size_t i = sizeof(size_t); i < buffer_size; i++)
    //{
    //    buffer[i] = std::rand() % 256; 
    //}

    //return 1; 
}

// Данная функция вызывается TCP Handler'ом после успешной отправки блока
// - Декрементирует счетчик получателей данного блока в контейнере отправляемых блоков
// - Если счетчик равен 0, то удаляет блок из контейнера и освобождает память
void SendingData::DeleteConnection(const uint id) 
{
    std::lock_guard<std::mutex> lock(_mtx); 

    auto it = _blocksInUse.find(id);
    if (it != _blocksInUse.end()) 
    {
        ActiveUsersOfBuffer & activeBuffer = it -> second;
        activeBuffer.counter--;

        if (counter == 0) 
        {
            _totalSizeInUse -= activeBuffer.size; 
            delete[] activeBuffer.buffer; 
            _blocksInUse.erase(it); 

            if ( _totalSizeInUse < MAX_TOTAL_BLOCK_SIZE)
            {
                _cv.notify_all();
            }
        }
    }
}

// Ищет в контейнере требуемый блок и инкрементирует счетчик получателей, если блок найден
bool SendingData::GetBlockById(const uint id, char * retBuf) 
{
    std::lock_guard<std::mutex> lock(_mtx); 

    auto it = _blocksInUse.find(id);
    if (it != _blocksInUse.end()) 
    {
        ActiveUsersOfBuffer & activeBuffer = it -> second;
        
        retBuf = activeBuffer.buffer;
        activeBuffer.counter++; 

        return true;
    } else 
    {
        retBuf = nullptr; 
        return false;
    }
}

//
// Вызывается после загрузки блока с помощью get_block_data(...) 
// (Ведь блок не был найдет в контейнере и требуется загрузка из блочного устройства)
// Добавляет блок в контейнер отправляемых блоков 
void SendingData::AddConnectionToBlock(const uint id, char * readyBuf, size_t size) 
{
    std::unique_lock<std::mutex> lock(_mtx); 

    while (_totalSizeInUse >= MAX_TOTAL_BLOCK_SIZE && !_deleteNotified)
    {
        _cv.wait(lock);
    }

    if (_deleteNotified)
    {
        return;
    }

    ActiveUsersOfBuffer activeBuffer;
    activeBuffer.buffer = readyBuf;
    activeBuffer.size   = size;
     _totalSizeInUse    += size;

    activeBuffer.counter++;
    _blocksInUse[id] = activeBuffer; 
}

void SendingData::NotifyDelete()
{
    std::lock_guard<std::mutex> lock(_mtx);
    _deleteNotified = true;
    _cv.notify_all();
}

const size_t SendingData::GetSize()
{
    return _totalSizeInUse;
}

void BlockResolver::AddQuerry(std::shared_ptr<Connection> & conn, std::string & hash)
{
    conn_hash pair(conn, hash);
    hashClientQueue.Push(pair);
}

void BlockResolver::DeleteFromSending(const uint & id)
{
    _readyBlocks.DeleteConnection(id);
}


// Сначала пытается найти блок в контейнере на отправку, если не находит - загружает, добавляет в контейнер и отправляет
void BlockResolver::Resolve()
{
    while(true)
    {
        conn_hash currConnHash = hashClientQueue.PopFront();
        std::shared_ptr<Connection> conn = currConnHash.first;

        size_t number = get_block_number(currConnHash.second);
        size_t size = get_block_size(currConnHash.second);

        BlockMsg msg(size, number);
        msg.encodeHeader();

        bool result = _readyBlocks.GetBlockById(number, msg.getData());

        if(result)
        {
            conn -> Write(msg);
        }
        else
        {
            int res = get_block_data(number, msg.getBody(), size);
            _readyBlocks.AddConnectionToBlock(number, msg.getData(), size);

            conn -> Write(msg);
        }
    }
}

void BlockResolver::BasicResolve()
{
    while(true)
    {
        conn_hash currConnHash = hashClientQueue.PopFront();
        std::shared_ptr<Connection> conn = currConnHash.first;

        size_t number = get_block_number(currConnHash.second);
        size_t size = get_block_size(currConnHash.second);

        BlockMsg msg;
        
        msg.setBodyLength(size);
        int res = get_block_data(number, msg.getBody(), size);       
        msg.encodeHeader();

        conn -> Write(msg);
    }
}

void BlockResolver::Dump()
{
    hashClientQueue.Dump();
}
