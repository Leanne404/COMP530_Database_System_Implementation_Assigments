#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include <string>

using namespace std;

MyDB_PageHandle MyDB_BufferManager::getPage(MyDB_TablePtr whichTable, long i) {
    string key = whichTable->getName() + "_" + to_string(i);

	if (lookup.count(key)) {
		// 已經有了，直接取出
		Page* existing = lookup[key];

		// 找到它在 pages[] 裡的 index
		int slot = -1;
		for (int j = 0; j < numPages; j++) {
			if (pages[j] == existing) {
				slot = j;
				break;
			}
		}

		// 標記 useBit，表示最近有被用到
		if (slot != -1) {
			useBit[slot] = true;
		}

		// 回傳一個 handle 包裝這個 Page
		return make_shared<MyDB_PageHandleBase>(existing, this);
	}



    // 沒有 → 找一個 victim
    int slot = findVictim();

    Page* victim = pages[slot];
    if (victim && victim->dirty) {
        // flush victim
    }
    // 載入新的 page
    Page* newPage = new Page();
    newPage->table = whichTable;
    newPage->pageNum = i;
    newPage->dirty = false;
    newPage->pinned = false;
    newPage->bytes = malloc(pageSize);

    // 從檔案讀 (seek 到 i*pageSize)
    // fread(newPage->bytes, ...);

    pages[slot] = newPage;
    lookup[key] = newPage;
    useBit[slot] = true;

    return make_shared<MyDB_PageHandleBase>(newPage, this);
}


MyDB_PageHandle MyDB_BufferManager::getPage() {
    int slot = findVictim();

    Page* victim = pages[slot];
    if (victim && victim->dirty) {
        // flush victim 到 tempFile
    }

    Page* newPage = new Page();
    newPage->table = nullptr;   // anonymous
    newPage->pageNum = -1;
    newPage->dirty = false;
    newPage->pinned = false;
    newPage->bytes = malloc(pageSize);

    pages[slot] = newPage;
    useBit[slot] = true;

    return make_shared<MyDB_PageHandleBase>(newPage, this);
}


MyDB_PageHandle MyDB_BufferManager::getPinnedPage(MyDB_TablePtr t, long i) {
    auto handle = getPage(t, i);
    // 標記 pinned
    // 找到對應 Page，pinned = true
    return handle;
}

MyDB_PageHandle MyDB_BufferManager::getPinnedPage() {
    auto handle = getPage();
    // 標記 pinned
    return handle;
}

void MyDB_BufferManager::unpin(MyDB_PageHandle unpinMe) {
    // 找到 Page，把 pinned = false
}
int MyDB_BufferManager::findVictim() {
	// Clock 演算法
	while (true) {
		if (!pages[clockHand] || !pages[clockHand]->pinned) {
			if (useBit[clockHand]) {
				useBit[clockHand] = false;
			} else {
				int victimSlot = clockHand;
				clockHand = (clockHand + 1) % numPages;
				return victimSlot;
			}
		}
		clockHand = (clockHand + 1) % numPages;
	}
}

MyDB_BufferManager::MyDB_BufferManager(size_t pageSizeIn, size_t numPagesIn, string tempFileIn) {
    pageSize = pageSizeIn;
    numPages = numPagesIn;
    tempFile = tempFileIn;

    // 初始化 pool slots
    pages.resize(numPages, nullptr);

    // 初始化 clock
    clockHand = 0;
    useBit.assign(numPages, false);
}

MyDB_BufferManager::~MyDB_BufferManager() {
    // 把所有 dirty 的 page 寫回去
    for (auto page : pages) {
        if (page && page->dirty) {
            // 如果是 non-anonymous：寫回 table file
            // 如果是 anonymous：寫回 tempFile
        }
        delete page; // 回收 metadata
    }
    // 刪掉 tempFile
    remove(tempFile.c_str());
}

	
#endif


