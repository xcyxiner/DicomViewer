
#include <stdexcept>

#include "ServiceContainer.h"

#include "infrastructure/cache/IFrameCache.h"
#include "infrastructure/dicom_io/IDicomReader.h"
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/task/ITaskQueue.h"

ServiceContainer::ServiceContainer() = default;
ServiceContainer::~ServiceContainer() = default;

template<typename Interface>
void ServiceContainer::registerInstance(std::shared_ptr<Interface> instance)
{
  m_services[typeid(Interface)] = std::move(instance);
}

template<typename Interface>
std::shared_ptr<Interface> ServiceContainer::resolve() const
{
  auto it = m_services.find(typeid(Interface));
  if (it == m_services.end()) {
    throw std::runtime_error("Service not registered");
  }
  return std::static_pointer_cast<Interface>(it->second);
}

// 显式实例化 — 项目中使用的接口类型
template void ServiceContainer::registerInstance<IImageRenderer>(
    std::shared_ptr<IImageRenderer>);
template void ServiceContainer::registerInstance<IDicomReader>(
    std::shared_ptr<IDicomReader>);
template void ServiceContainer::registerInstance<ITaskQueue>(
    std::shared_ptr<ITaskQueue>);
template void ServiceContainer::registerInstance<IFrameCache>(
    std::shared_ptr<IFrameCache>);

template std::shared_ptr<IImageRenderer>
ServiceContainer::resolve<IImageRenderer>() const;
template std::shared_ptr<IDicomReader> ServiceContainer::resolve<IDicomReader>()
    const;
template std::shared_ptr<ITaskQueue> ServiceContainer::resolve<ITaskQueue>()
    const;
template std::shared_ptr<IFrameCache> ServiceContainer::resolve<IFrameCache>()
    const;
