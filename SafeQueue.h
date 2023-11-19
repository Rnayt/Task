#pragma once

#include <iostream>
#include <queue>

template <typename T>
class SafeQueue
{
public:
    void Push(T element)
    {
	std::unique_lock<std::mutex> lock(_mtx);
	_queue.push(element);
	_cond.notify_one();
    }
         
    T PopFront()
    {
     	std::unique_lock<std::mutex> lock(_mtx);
			
	_cond.wait(lock, [this]() 
	{ 
	    return !_queue.empty(); 
	}); 

	T element = _queue.front();
	_queue.pop();

	return element;
    }
        
    void Pop()
    {
        std::unique_lock<std::mutex> lock(_mtx);
	_cond.wait(lock, [this]() 
	{ 
	    return !_queue.empty(); 
	}); 
        _queue.pop();
    }

    T Front()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        T element = _queue.front();
        return element;
    }

    int Empty()
    {
        return _queue.empty();
    }

    void Dump()
    {
	std::unique_lock<std::mutex> lock(_mtx);
	std::cout << " -- queue -- \n";
        std::queue<T> copy = _queue; 

        while (!copy.empty())
        {
            std::cout << copy.front().second << std::endl;
            copy.pop();
        }
	std::cout << " -- \n";
    }

private:
 	std::queue<T> _queue;
	std::mutex _mtx;

	std::condition_variable _cond;
};
