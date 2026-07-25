
#pragma once
#include <memory>
#include "domain/model/Frame.h"

#include <variant>
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
class IFrameCache 
{
public:
    explicit IFrameCache();
    ~IFrameCache()=default;

public:
    using FramePtr = std::variant<std::shared_ptr<Frame>, vtkSmartPointer<vtkImageData>,nullptr_t>;
    virtual void put(const std::string& uid, int index, const IFrameCache::FramePtr& frame) = 0;
    virtual IFrameCache::FramePtr get(const std::string& uid, int index) = 0;
};
