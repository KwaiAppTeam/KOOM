//
// Created by lirui on 2020/11/3.
//

#include "item.h"

namespace koom {

ResItemData::ResItemData() = default;

ResItemData::ResItemData(const ResItemData &data) {
    this->lib_index = data.lib_index;
    this->allocate_time = data.allocate_time;
    this->id = data.id;
    this->java_call_stack.assign(data.java_call_stack);
    memcpy(this->pc, data.pc, sizeof(this->pc));
}

void ResItemData::Update(int64_t allocateTime, const std::string &javaCallStack, int resId,
                         uintptr_t (&_pc)[koom::Constant::max_call_stack_depth]) {
    this->allocate_time = allocateTime;
    this->java_call_stack.assign(javaCallStack);
    this->id = resId;
    memcpy(this->pc, _pc, sizeof(_pc));
}

void ResItemData::Clear() {
    this->lib_index = 0;
    this->id = 0;
    this->allocate_time = 0;
    this->java_call_stack.clear();//don't shrink for performance
    memset(pc, 0, sizeof(pc));
}
}