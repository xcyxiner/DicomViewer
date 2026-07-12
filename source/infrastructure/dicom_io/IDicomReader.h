
#pragma once
#include <string>
#include <memory>
#include "domain/model/Series.h"
#include "domain/model/Frame.h"

class IDicomReader 
{
public:
    explicit IDicomReader();
    ~IDicomReader()=default;
	virtual void open(const std::string& path) = 0;
	virtual Series readSeries() = 0;
	virtual std::shared_ptr<Frame> readFrame(int index) = 0;    
};
