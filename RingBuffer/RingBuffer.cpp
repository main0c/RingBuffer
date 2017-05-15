//
//  OutputRingBuffer.cpp
//  RingBuffer
//
//  Created by Сергей Сейтов on 14.05.17.
//  Copyright © 2017 V-Channel. All rights reserved.
//

#include "RingBuffer.h"
#include "AudioUtils.h"

RingBuffer::RingBuffer() : _count(POOL_SIZE+1), _bufferSize(FRAME_SIZE*sizeof(int16_t)), _start(0), _end(0), _stopped(false)
{
    _data = (int16_t*)calloc(_count, _bufferSize);
}

RingBuffer::~RingBuffer()
{
    free(_data);
}

void RingBuffer::read(void (^data)(int16_t*))
{
    std::unique_lock<std::mutex> lock(_mutex);
    _empty.wait(lock, [this]() { return (!isEmpty() || _stopped);});
    if (_stopped) return;
    
    data(_data + _start*_bufferSize);
    
    _start = (_start + 1) % _count;
    _overflow.notify_one();
}

void RingBuffer::write(void (^data)(int16_t*))
{
    std::unique_lock<std::mutex> lock(_mutex);
    _overflow.wait(lock, [this]() { return (!isFull() || _stopped);});
    if (_stopped) return;
    
    data(_data + _end*_bufferSize);
    
    _end = (_end + 1) % _count;
    _empty.notify_one();
}

void RingBuffer::read(void* audioFrame)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _empty.wait(lock, [this]() { return (!isEmpty() || _stopped);});
    if (_stopped) return;
    
    memcpy(audioFrame, _data + _start*_bufferSize, _bufferSize);
    
    _start = (_start + 1) % _count;
    _overflow.notify_one();
}

void RingBuffer::write(void* audioFrame)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _overflow.wait(lock, [this]() { return (!isFull() || _stopped);});
    if (_stopped) return;
    
    memcpy(_data + _end*_bufferSize, audioFrame, _bufferSize);
    
    _end = (_end + 1) % _count;
    _empty.notify_one();
}

void RingBuffer::flush()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _start = _end = 0;
}

void RingBuffer::stop()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _stopped = true;
    _overflow.notify_one();
    _empty.notify_one();
}
