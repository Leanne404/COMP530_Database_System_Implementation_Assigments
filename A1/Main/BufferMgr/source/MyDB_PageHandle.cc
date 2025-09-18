#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include "MyDB_PageHandle.h"
#include "MyDB_BufferManager.h"   // 需要裡面的 Page 定義

void* MyDB_PageHandleBase::getBytes() {
    return page->bytes;
}

void MyDB_PageHandleBase::wroteBytes() {
    page->dirty = true;   // 標記 page 被修改過
}

MyDB_PageHandleBase::~MyDB_PageHandleBase() {
    // 預設不用刪 page，因為 Page 的生命週期由 BufferManager 管
    // 這裡只要釋放 shared_ptr 即可
}

#endif
