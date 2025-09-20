#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include <cstdio>    // fopen/fseek/fread/fwrite/fclose
#include <cstring>   // memset
#include <string>

using namespace std;

void MyDB_BufferManager::writeBack(Page* p) {
    if (!p || !p->dirty) return;
    if (p->table == nullptr) {
        // 匿名頁：本作業不要求回讀/回寫，可略過（或寫 tempFile，自行擴充）
        p->dirty = false;
        return;
    }
    const string& path = p->table->getStorageLoc();

    // 先嘗試 rb+，沒有檔就 wb+ 建新檔
    FILE* f = fopen(path.c_str(), "rb+");
    if (!f) f = fopen(path.c_str(), "wb+");
    if (!f) {
        // 開檔失敗：避免掛掉，標乾淨以免重複嘗試（也可選擇保留 dirty 看你策略）
        p->dirty = false;
        return;
    }

    // 定位到 page offset
    long offset = static_cast<long>(p->pageNum) * static_cast<long>(pageSize);
    fseek(f, offset, SEEK_SET);

    // 若 bytes 還沒載入，代表有人 writeBytes() 前一定 getBytes() 過；保守起見做個 0 填
    if (!p->bytes) {
        // lazy-load 尚未觸發，但我們仍要把「當前記憶體」寫回；保守起見寫一頁零
        void* tmp = malloc(pageSize);
        memset(tmp, 0, pageSize);
        fwrite(tmp, 1, pageSize, f);
        free(tmp);
    } else {
        fwrite(p->bytes, 1, pageSize, f);
    }

    fclose(f);
    p->dirty = false;
}

void MyDB_BufferManager::evictSlot(int slot) {
    Page* victim = pages[slot];
    if (!victim) return;

    // 如果是 non-anon，先把 lookup 乾淨
    if (victim->table != nullptr) {
        auto oldKey = makeKey(victim->table, victim->pageNum);
        auto it = lookup.find(oldKey);
        if (it != lookup.end() && it->second == victim) {
            lookup.erase(it);
        }
    }

    // 請先 flush 再釋放記憶體
    if (victim->dirty) {
        writeBack(victim);
    }

    if (victim->bytes) {
        free(victim->bytes);
        victim->bytes = nullptr;
    }
    delete victim;
    pages[slot] = nullptr;
}

int MyDB_BufferManager::findVictim() {
    while (true) {
        // 空槽：直接用
        if (!pages[clockHand]) {
            int victimSlot = clockHand;
            clockHand = (clockHand + 1) % numPages;
            return victimSlot;
        }

        // 被 pin 住：跳過
        if (pages[clockHand]->pinned) {
            clockHand = (clockHand + 1) % numPages;
            continue;
        }

        // 第二次機會
        if (useBit[clockHand]) {
            useBit[clockHand] = false;
            clockHand = (clockHand + 1) % numPages;
            continue;
        }

        // 真的要淘汰
        int victimSlot = clockHand;
        clockHand = (clockHand + 1) % numPages;
        return victimSlot;
    }
}

MyDB_PageHandle MyDB_BufferManager::getPage(MyDB_TablePtr whichTable, long i) {
    string key = makeKey(whichTable, i);

    // 命中：回 handle，並給第二次機會
    auto it = lookup.find(key);
    if (it != lookup.end()) {
        Page* existing = it->second;

        // 標記 useBit
        int slot = -1;
        for (int j = 0; j < (int)numPages; ++j) {
            if (pages[j] == existing) { slot = j; break; }
        }
        if (slot != -1) useBit[slot] = true;

        return make_shared<MyDB_PageHandleBase>(existing, this);
    }

    // 未命中：找 victim，先把原本的清掉
    int slot = findVictim();
    evictSlot(slot);

    // 放入新頁（lazy-load：bytes 先 nullptr，交給 handle.getBytes() 負責）
    Page* newPage = new Page();
    newPage->table  = whichTable;
    newPage->pageNum = i;
    newPage->dirty  = false;
    newPage->pinned = false;
    newPage->bytes  = nullptr;

    pages[slot] = newPage;
    lookup[key]  = newPage;
    useBit[slot] = true;

    return make_shared<MyDB_PageHandleBase>(newPage, this);
}

MyDB_PageHandle MyDB_BufferManager::getPage() {
    int slot = findVictim();
    evictSlot(slot);

    Page* newPage = new Page();
    newPage->table  = nullptr;   // anonymous
    newPage->pageNum = -1;
    newPage->dirty  = false;
    newPage->pinned = false;
    newPage->bytes  = nullptr;   // lazy-load: 全 0 由 handle.getBytes() 處理

    pages[slot]   = newPage;
    useBit[slot]  = true;

    return make_shared<MyDB_PageHandleBase>(newPage, this);
}

MyDB_PageHandle MyDB_BufferManager::getPinnedPage(MyDB_TablePtr t, long i) {
    auto handle = getPage(t, i);
    handle->setPinned(true);
    return handle;
}

MyDB_PageHandle MyDB_BufferManager::getPinnedPage() {
    auto handle = getPage();
    handle->setPinned(true);
    return handle;
}

void MyDB_BufferManager::unpin(MyDB_PageHandle unpinMe) {
    if (unpinMe) unpinMe->setPinned(false);
}

MyDB_BufferManager::MyDB_BufferManager(size_t pageSizeIn, size_t numPagesIn, string tempFileIn) {
    pageSize = pageSizeIn;
    numPages = numPagesIn;
    tempFile = tempFileIn;

    pages.resize(numPages, nullptr);
    useBit.assign(numPages, false);
    clockHand = 0;
}

MyDB_BufferManager::~MyDB_BufferManager() {
    // flush + free
    for (auto page : pages) {
        if (!page) continue;

        if (page->dirty) {
            writeBack(page);
        }
        if (page->bytes) {
            free(page->bytes);
            page->bytes = nullptr;
        }
        delete page;
    }

    // tempFile 若有，刪掉即可（匿名頁沒寫回也無妨）
    if (!tempFile.empty()) {
        remove(tempFile.c_str());
    }
}

#endif
