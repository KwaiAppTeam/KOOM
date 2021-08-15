#include "thread_item.h"

namespace koom {

ThreadItem::ThreadItem() = default;

ThreadItem::ThreadItem(const ThreadItem &threadItem) : ResItemData(threadItem) {
    this->thread_detached = threadItem.thread_detached;
    this->thread_internal_id = threadItem.thread_internal_id;
    this->startTime = threadItem.startTime;
    this->exitTime = threadItem.exitTime;
    this->thread_reported = threadItem.thread_reported;
    this->name.assign(threadItem.name);
    this->collect_mode.assign(threadItem.collect_mode);
}

void ThreadItem::Clear() {
    ResItemData::Clear();
    this->thread_internal_id = 0;
    this->startTime = 0LL;
    this->thread_detached = false;
    this->exitTime = 0LL;
    this->thread_reported = false;
    this->name.clear();
    this->collect_mode.clear();
}
}