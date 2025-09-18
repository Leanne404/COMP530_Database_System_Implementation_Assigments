#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include "MyDB_PageHandle.h"
#include "MyDB_BufferManager.h"
#include <cstdlib>   // for malloc, free
#include <cstring>   // for memset
#include <cstdio>    // for FILE, fopen, fread, fseek, fclose
#include <iostream>

void* MyDB_PageHandleBase::getBytes() {

    if (!page->bytes) {
        // 配置記憶體給這個 page
        page->bytes = malloc(mgr->getPageSize());


        if (page->table != nullptr) {
            // 從檔案讀資料
            FILE* f = fopen(page->table->getStorageLoc().c_str(), "rb");
            if (f) {
                fseek(f, page->pageNum * mgr->getPageSize(), SEEK_SET);
                fread(page->bytes, 1, mgr->getPageSize(), f);
                fclose(f);
            } else {
                // 新檔案或讀失敗 -> 填零
                memset(page->bytes, 0, mgr->getPageSize());
            }
        } else {
            // anonymous page
            memset(page->bytes, 0, mgr->getPageSize());
        }
    }
    return page->bytes;
}

void MyDB_PageHandleBase::wroteBytes() {
    page->dirty = true;
}

void MyDB_PageHandleBase::setPinned(bool val) {
    if (page) page->pinned = val;
}

bool MyDB_PageHandleBase::isPinned() const {
    return page && page->pinned;
}

MyDB_PageHandleBase::~MyDB_PageHandleBase() {
    // 如果這個 page pinned，要解 pin
    if (page) {
        page->pinned = false;   // 直接改 flag
    }
}

#endif
