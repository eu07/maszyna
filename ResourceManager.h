#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H 1

#include <vector>
#include <algorithm>

#pragma hdrstop

class Resource
{

public:
    virtual void Release() = 0;
    double GetLastUsage() const { return _lastUsage; }

protected:
    void SetLastUsage(double lastUsage) { _lastUsage = lastUsage; }

private:
    double _lastUsage;

};

class ResourceManager
{

public:
    static void Register(Resource* resource);
    static void Unregister(Resource* resource);

    static void Sweep(double currentTime);
    static void SetExpiry(double expiry) { _expiry = expiry; }

private:
    typedef std::vector<Resource*> Resources;

    static double _expiry;
    static double _lastUpdate;
    static double _lastReport;
    
    static Resources _resources;

};

#endif
