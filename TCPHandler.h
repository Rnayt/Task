#pragma once
#include <ctime>
#include <iostream>
#include <string>
#include <cstdio>
#include <queue>
#include <deque>
#include <memory>
#include <set>
#include <utility>
#include <thread>
#include <functional>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include "BlockResolver.h"
class BlockResolver;

using boost::asio::ip::tcp;

class Connection
    :public std::enable_shared_from_this<Connection>
{
public:

    Connection(tcp::socket socket, boost::asio::io_context & io_context): 
        _socket(std::move(socket)), _strand(io_context)
    {  }

    using callback_nh     = std::function<void(std::shared_ptr<Connection>, std::string)>; 
    using callback_fw     = std::function<void(const uint id)>;

    void Register(callback_nh Callback_NewHash)
    {
        _Callback_NewHash = Callback_NewHash;
    }

    void Register(callback_fw Callback_FinishWrite)
    {
        _Callback_FinishWrite = Callback_FinishWrite;
    }

    void Start()
    {
        std::cout << " Start of service\n";
        ReadHeader();
        ReadBody(_hashesForResponse);
    }

    void Write(const BlockMsg & message)
    {
        _strand.post(
                boost::bind(
                    &Connection::WriteAdd,
                    this,
                    message
                    )
                );
    }

private:
    
    void WriteAdd(const BlockMsg & message)
    {
        _blockDeque.push_back(message);
        if (_blockDeque.size() > 1 ) 
        {
            return;
        }
        this -> Write();
    }

    void Write()
    {
        const BlockMsg & message = _blockDeque[0];
            
        boost::asio::async_write(
                _socket,
                boost::asio::buffer(message.getData(), message.getLength()),
                _strand.wrap(
                    boost::bind(
                        &Connection::writeHandler,
                        this,
                        message, 
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        )
                    )
                );
    }

    void writeHandler(const BlockMsg & message, const boost::system::error_code & error, const size_t transferred)
    {
        _blockDeque.pop_front();

        if ( error ) {
            std::cerr << "could not write: " << boost::system::system_error(error).what() << std::endl;
            return;
        }
            
        uint id = message.getId();
        std::shared_ptr<Connection> sharedPtr = shared_from_this();
        _Callback_FinishWrite(id);

        _hashesForResponse--;
        if (_hashesForResponse == 0)
        {
            _socket.close();
            return;
        }

        if ( !_blockDeque.empty() ) 
        {
            this -> Write();
        }
    }
   
    void ReadHeader()
    {
        auto self(shared_from_this());

        boost::asio::async_read(_socket,
        boost::asio::buffer(readMsg.getBody(), readMsg.getLength()),
        [this, self](boost::system::error_code ec, std::size_t transferred)
        {
            if(!ec)
            {
                _hashesForResponse = readMsg.decodeHeader();
                ReadBody(_hashesForResponse);
                if (readMsg.isHeader())
                {
                    _hashesForResponse = readMsg.decodeHeader();
                    ReadBody(_hashesForResponse);
                }
                else
                {
                    _socket.close();
                    std::cerr << "Error: Non-header message received" << std::endl;
                }
            }
            else
            {
                //
                _socket.close();
                std::cerr << " Error while reading data " << std::endl;
            }
        });
    }

    void ReadBody(int numOfHashes)
    {
        auto self(shared_from_this());

        if(numOfHashes > 0)
        {
            boost::asio::async_read(_socket,
            boost::asio::buffer(readMsg.getData(), readMsg.getLength()),
            [this, self, numOfHashes](boost::system::error_code ec, std::size_t transferred)
            {
                if (!ec)
                {
                    std::string msgStr(readMsg.getBody(), readMsg.getLength());
                    _Callback_NewHash(shared_from_this(), msgStr);          

                    ReadBody(numOfHashes - 1);
                }
            
                else
                {
                    //
                    _socket.close();
                    std::cerr << " Error while reading data " << ec.message() << std::endl;
                }
            });
        }
    }

    std::deque<BlockMsg> _blockDeque;

    HashMsg readMsg;
    uint    _hashesForResponse;

    callback_fw _Callback_FinishWrite;
    callback_nh _Callback_NewHash;

    tcp::socket _socket;
    boost::asio::io_context::strand _strand;
};

class Server
{
public:

    Server(const tcp::endpoint & endpoint, boost::asio::io_context & io_context, BlockResolver & resolver)
        : _resolver(resolver), _acceptor(io_context, endpoint), _io_context(io_context)
    {
        doAccept();
    }
    ~Server()
    { }

private:

    void doAccept()
    {
        _acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                std::shared_ptr<Connection> newConn = std::make_shared<Connection>(std::move(socket), _io_context);

                newConn -> Register([&, newConn](std::shared_ptr<Connection> conn, std::string hash) 
                {
                    _resolver.AddQuerry(conn, hash); 
                });

                newConn -> Register([&, newConn](const uint id) 
                {
                    _resolver.DeleteFromSending(id);
                });

                newConn -> Start();
            }

            doAccept();
        });
    }

    BlockResolver & _resolver;

    tcp::acceptor _acceptor;
    boost::asio::io_context & _io_context;
};
