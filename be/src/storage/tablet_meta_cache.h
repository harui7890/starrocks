// This file is licensed under the Elastic License 2.0. Copyright 2021-present, StarRocks Limited.

#pragma once


#include <memory>
#include <list>
#include <unordered_map>
#include <thread>
#include <mutex>

namespace starrocks {
class Tablet;
using TabletSharedPtr = std::shared_ptr<Tablet>;


class TabletMetaCache {
public:
    virtual TabletSharedPtr get(int64_t tablet_id) = 0;
    virtual bool put(const TabletSharedPtr& tabletptr) = 0;
    virtual bool remove(int64_t tablet_id) = 0;
    virtual ~TabletMetaCache() = default;
};

class LRUTabletMetaCache : public TabletMetaCache {
public:
    TabletSharedPtr get(int64_t tablet_id);
    bool put(const TabletSharedPtr& tabletptr);
    bool remove(int64_t tablet_id);
    LRUTabletMetaCache(int capacity);
    virtual ~LRUTabletMetaCache();

private:
    void prune_cache();
    std::thread _prune_thread;
    std::mutex _mutex;
    int _capacity;
    int _used;
    bool _is_stopped;
    std::list<TabletSharedPtr> _lru_list;
    std::unordered_map<int64_t,std::list<TabletSharedPtr>::iterator> _tabletmap;
};

} // namespace starrocks
