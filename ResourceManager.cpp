#include "ResourceManager.h"
#include "Logs.h"

#include <sstream>

ResourceManager::Resources ResourceManager::_resources;
double ResourceManager::_expiry = 5.0f;
double ResourceManager::_lastUpdate = 0.0f;
double ResourceManager::_lastReport = 0.0f;

void ResourceManager::Register(Resource *resource) { _resources.push_back(resource); };

void ResourceManager::Unregister(Resource *resource)
{
    Resources::iterator iter = std::find(_resources.begin(), _resources.end(), resource);

    if (iter != _resources.end())
        _resources.erase(iter);

    resource->Release();
};

class ResourceExpired
{
  public:
    ResourceExpired(double time) : _time(time){};
    bool operator()(Resource *resource) { return (resource->GetLastUsage() < _time); }

  private:
    double _time;
};

void ResourceManager::Sweep(double currentTime)
{

    if (currentTime - _lastUpdate < _expiry)
        return;

    Resources::iterator begin = std::remove_if(_resources.begin(), _resources.end(),
                                               ResourceExpired(currentTime - _expiry));

#ifdef RESOURCE_REPORTING
    if (begin != _resources.end())
        WriteLog("Releasing resources");
#endif

    for (Resources::iterator iter = begin; iter != _resources.end(); iter++)
        (*iter)->Release();

#ifdef RESOURCE_REPORTING
    if (begin != _resources.end())
    {
        std::ostringstream msg;
        msg << "Released " << (_resources.end() - begin) << " resources";
        WriteLog(msg.str().c_str());
    };
#endif

    _resources.erase(begin, _resources.end());

#ifdef RESOURCE_REPORTING
    if (currentTime - _lastReport > 30.0f)
    {
        std::ostringstream msg;
        msg << "Resources count: " << _resources.size();
        WriteLog(msg.str().c_str());
        _lastReport = currentTime;
    };
#endif

    _lastUpdate = currentTime;
};
