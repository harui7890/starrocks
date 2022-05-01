// This file is licensed under the Elastic License 2.0. Copyright 2021-present, StarRocks Limited.

#include "storage/tablet_meta_cache.h"
#include <thread>
#include "tablet.h"
#include "util/thread.h"

namespace starrocks {
TabletSharedPtr LRUTabletMetaCache::get(int64_t tablet_id) {
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _tabletmap.find(tablet_id);

    if (it != _tabletmap.end()) {
        _lru_list.splice(_lru_list.cend(),_lru_list,it->second);
        return *(it->second);
    }
    return nullptr;
}

bool LRUTabletMetaCache::put(const TabletSharedPtr& meta) {
    std::lock_guard<std::mutex> lk(_mutex);
    auto old = _tabletmap.find(meta->tablet_id());
    if(old !=_tabletmap.end()){
        _lru_list.splice(_lru_list.cend(),_lru_list,old->second);
        return false;
    }
    auto it = _lru_list.insert(_lru_list.cend(),meta);
    auto [new_it, inserted] = _tabletmap.emplace(meta->tablet_id(),it);
    if (inserted) {
        _lru_list.splice(_lru_list.cend(),_lru_list,it);
        _used++;
    }else{
        _lru_list.erase(it);
    }
    return inserted;
}

bool LRUTabletMetaCache::remove(int64_t tablet_id) {
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _tabletmap.find(tablet_id);
    if (it != _tabletmap.end()) {
        _lru_list.erase(it->second);
        _tabletmap.erase(tablet_id);
        _used--;
    }
    return true;
}

void LRUTabletMetaCache::prune_cache() {
    while (!_is_stopped) {
        std::this_thread::sleep_for(std::chrono::seconds(config::tablet_meta_cache_prune_interval));
        if (_used <= _capacity) {
            continue;
        }
        int try_to_free = _used - _capacity;
        int cnt = 0;
        {
            std::lock_guard<std::mutex> lk(_mutex);
            for (auto it = _lru_list.cbegin(); it!=_lru_list.cend(); ) {
                const TabletSharedPtr  & tablet = *it;
                if (tablet.use_count() == 1 && tablet->can_be_evicted()) {
                    LOG(WARNING) << "evict  tablet=" << tablet->tablet_id();
                    _tabletmap.erase(tablet->tablet_id());
                    _lru_list.erase(it++);
                    _used--;
                    cnt++;
                }else{
                    ++it;
                }
                if (cnt >= try_to_free) {
                    break;
                }
            }
        }
    }
}

LRUTabletMetaCache::LRUTabletMetaCache(int capacity) {
    _capacity = capacity;
    _used = 0;
    _is_stopped = false;
    _prune_thread = std::thread([this] { prune_cache(); });
    Thread::set_thread_name(_prune_thread, "tablet_cache_prune");
}

LRUTabletMetaCache::~LRUTabletMetaCache() {
    _is_stopped = true;
    _prune_thread.join();
}
} // namespace starrocks
