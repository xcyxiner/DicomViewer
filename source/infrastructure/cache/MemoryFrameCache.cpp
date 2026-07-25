
#include "MemoryFrameCache.h"

MemoryFrameCache::MemoryFrameCache()
{
    cache=std::unordered_map<std::string, std::unordered_map<int, IFrameCache::FramePtr>>();
}

void MemoryFrameCache::put(const std::string& uid,
                           int index,
                           const IFrameCache::FramePtr& frame)
{
    if (cache.find(uid) == cache.end())
    {
        cache[uid] = std::unordered_map<int, IFrameCache::FramePtr>();
    }
    cache[uid][index] = frame;
}

IFrameCache::FramePtr MemoryFrameCache::get(const std::string& uid, int index)
{
    if (cache.find(uid) != cache.end())
    {
        const auto& frameMap = cache[uid];
        if (frameMap.find(index) != frameMap.end())
        {
            return frameMap.at(index);
        }
    }
    return IFrameCache::FramePtr();
}
