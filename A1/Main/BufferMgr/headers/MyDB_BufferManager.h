
#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

struct Page {
	MyDB_TablePtr table;   // 指向 Table（如果是 anonymous 則為 nullptr）
	long pageNum;          // page 編號（anonymous 則為 -1）
	bool dirty;            // 是否被修改過
	bool pinned;           // 是否被鎖定
	void* bytes;           // 指向 buffer slot 的記憶體
	int slotIndex;         // 在 buffer 的位置
};

class MyDB_BufferManager {

public:

	// THESE METHODS MUST APPEAR AND THE PROTOTYPES CANNOT CHANGE!

	// gets the i^th page in the table whichTable... note that if the page
	// is currently being used (that is, the page is current buffered) a handle 
	// to that already-buffered page should be returned
	MyDB_PageHandle getPage (MyDB_TablePtr whichTable, long i);

	// gets a temporary page that will no longer exist (1) after the buffer manager
	// has been destroyed, or (2) there are no more references to it anywhere in the
	// program.  Typically such a temporary page will be used as buffer memory.
	// since it is just a temp page, it is not associated with any particular 
	// table
	MyDB_PageHandle getPage ();

	// gets the i^th page in the table whichTable... the only difference 
	// between this method and getPage (whicTable, i) is that the page will be 
	// pinned in RAM; it cannot be written out to the file
	MyDB_PageHandle getPinnedPage (MyDB_TablePtr whichTable, long i);

	// gets a temporary page, like getPage (), except that this one is pinned
	MyDB_PageHandle getPinnedPage ();

	// un-pins the specified page
	void unpin (MyDB_PageHandle unpinMe);

	// creates an LRU buffer manager... params are as follows:
	// 1) the size of each page is pageSize 
	// 2) the number of pages managed by the buffer manager is numPages;
	// 3) temporary pages are written to the file tempFile
	MyDB_BufferManager (size_t pageSize, size_t numPages, string tempFile);
	
	// when the buffer manager is destroyed, all of the dirty pages need to be
	// written back to disk, any necessary data needs to be written to the catalog,
	// and any temporary files need to be deleted
	~MyDB_BufferManager ();

	// FEEL FREE TO ADD ADDITIONAL PUBLIC METHODS 
	
    size_t getPageSize() const { return pageSize; }




private:

	// YOUR STUFF HERE
	size_t pageSize;
    size_t numPages;
    string tempFile;

    vector<Page*> pages;
    unordered_map<string, Page*> lookup;
    vector<bool> useBit;
    int clockHand;

    int findVictim(); 
	// 幫助函式：把非匿名 dirty 頁寫回對應 table 檔
	void writeBack(Page* p);
	// 幫助函式：清掉某個 slot 目前的頁（會負責 flush/lookup/free/delete）
	void evictSlot(int slot);
	// 幫助函式：組 key（tableName_pageNum）
	static inline string makeKey(const MyDB_TablePtr& t, long i) {
		return t->getName() + "_" + to_string(i);
	}


};

#endif


