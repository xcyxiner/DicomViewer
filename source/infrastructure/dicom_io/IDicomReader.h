
#pragma once
#include <string>
#include <memory>
#include "domain/model/Series.h"
#include "domain/model/Frame.h"
#include "infrastructure/cache/IFrameCache.h"
class IDicomReader 
{
public:
    explicit IDicomReader();
    virtual ~IDicomReader()=default;
    virtual void open(const std::string& filePath) = 0;
    virtual Series readSeries(const std::string& path) = 0;
	virtual std::unique_ptr<Frame> readFrameInfo( int index) = 0;
	virtual IFrameCache::FramePtr readFrame(int index) = 0;
    virtual void close() = 0; 
};
