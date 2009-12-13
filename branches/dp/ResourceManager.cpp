#include "ResourceManager.h"
#include "Logs.h"

ResourceManager::Resources ResourceManager::_resources;
double ResourceManager::_expiry = 10.0f;
double ResourceManager::_lastUpdate = 0.0f;
double ResourceManager::_lastReport = 0.0f;

void ResourceManager::Register(Resource* resource)
{
    _resources.push_back(resource);
};

class ResourceExpired
{
public:
    ResourceExpired(double time): _time(time) { };
    bool operator()(Resource* resource)
    {
        return (resource->GetLastUsage() < _time);
    }

private:
    double _time;
};

void ResourceManager::Sweep(double currentTime)
{

    if(currentTime - _lastUpdate < _expiry)
        return;

    Resources::iterator begin = std::remove_if(_resources.begin(), _resources.end(), ResourceExpired(currentTime - _expiry));

    for(Resources::iterator iter = begin; iter != _resources.end(); iter++)
        (*iter)->Release();

    if(begin != _resources.end())
        WriteLog("Released resources: ", _resources.end() - begin);

    _resources.erase(begin, _resources.end());

    if(currentTime - _lastReport > 30.0f)
    {
        WriteLog("Resources count: ", _resources.size());
        _lastReport = currentTime;
    };

    _lastUpdate = currentTime;

};
 