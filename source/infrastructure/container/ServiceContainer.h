
#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>

class ServiceContainer
{
public:
  ServiceContainer();
  ~ServiceContainer();

  template <typename Interface>
  void registerInstance(std::shared_ptr<Interface> instance);

  template <typename Interface>
  std::shared_ptr<Interface> resolve() const;

private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;
};
