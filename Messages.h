#pragma once
#include <iostream>

// Message
// В первые 4 байта кодируется длина последующего блока
class BlockMsg
{
public:

    enum { header_length = 4 };
    enum { max_body_length = 1024 };

    BlockMsg()
        : body_length_(0), data_(new char[header_length + max_body_length])
    {
    }

    BlockMsg(size_t bodyLength, size_t id) 
        : _id(id), 
          body_length_(bodyLength), 
          data_(new char[header_length + bodyLength])
    { 
        if (body_length_ > max_body_length)
            body_length_ = max_body_length;

        body_length_ = bodyLength;
    }

    const char * getData() const
    {
        return data_;
    }

    char * getData()
    {
        return data_;
    }

    std::size_t getLength() const
    {
        return header_length + body_length_;
    }

    const char * getBody() const
    {
        return data_ + header_length;
    }

    char * getBody()
    {
        return data_ + header_length;
    }

    std::size_t getBodyLength() const
    {
        return body_length_;
    }

    void setBodyLength(std::size_t new_length)
    {
        body_length_ = new_length;
        if (body_length_ > max_body_length)
            body_length_ = max_body_length;
    }

    std::size_t getId() const
    {
        return _id;
    }

    void setId(const std::size_t id)
    {
        _id = id;
    }
    
    bool decodeHeader()
    {
        char header[header_length + 1] = "";
        std::strncat(header, data_, header_length);
        body_length_ = std::atoi(header);
        if (body_length_ > max_body_length)
        {
            body_length_ = 0;
            return false;
        }

        return true;
    }

    void encodeHeader()
    {
        char header[header_length + 1] = "";
        std::sprintf(header, "%4d", static_cast<int>(body_length_));
        std::memcpy(data_, header, header_length);
    }

private:
    char * data_;
    size_t _id;
    size_t body_length_;
};

//
// Сначала на сервер отправляется сообщение, в котором содержится число хэшей, затем отправляются N хэшей 128 бит
class HashMsg
{
public:
    enum { header_length = 12 };
    enum { max_body_length = 16 };

    HashMsg()
        : _length(max_body_length)
    {
        std::cout << " constructor " << _length << std::endl;
    }

    const char * getData() const
    {
        return _data;
    }

    char * getData()
    {
        return _data;
    }

    char * getBody()
    {
        return _data;
    }

    std::size_t getLength() const
    {
        return _length;
    }

    void encodeHeader(uint numMessages)
    {
        std::memcpy(_data, headerMagicBytes, header_length);
        std::memcpy(_data + header_length, &numMessages, sizeof(uint));
        std::cout << _data << std::endl;
    }

    std::size_t decodeHeader() const
    {
        uint numMessages;
        std::memcpy(&numMessages, _data + header_length, sizeof(uint));
        return numMessages;
    }

    bool isHeader() const
    {
        if (_length < header_length)
        {
            return false;
        }

        return std::memcmp(_data, headerMagicBytes, header_length) == 0;
    }

private:
    const char headerMagicBytes[header_length] = { 

        0x74, 0x74, 0x74,  0x74,
        0x74, 0x74, 0x74,  0x74, 
        0x74, 0x74, 0x74,  0x74,  
        //data (num of hashes)
        };

    char _data[max_body_length];
    std::size_t _length;
};
