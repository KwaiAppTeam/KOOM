//
// Created by lirui on 2020/11/3.
//

#ifndef APM_ITEM_H
#define APM_ITEM_H

#include <string>
#include "common/constant.h"

namespace koom {

class ResItemData {
public:
    int         lib_index{};
    int         id{};//线程id、文件句柄id等
    int64_t     allocate_time{};
    std::string java_call_stack;
    uintptr_t   pc[koom::Constant::max_call_stack_depth]{};

    ResItemData();

    ResItemData(const ResItemData& data);

    void Update(int64_t allocateTime, const std::string& javaCallStack, int resId,
                uintptr_t (&pc)[koom::Constant::max_call_stack_depth]);

    virtual void Clear();
};
}

#endif //APM_ITEM_H
