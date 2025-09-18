
#ifndef CATALOG_UNIT_H
#define CATALOG_UNIT_H

#include "MyDB_BufferManager.h"
#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"
#include "QUnit.h"
#include <iostream>
#include <unistd.h>
#include <vector>

using namespace std;

// these functions write a bunch of characters to a string... used to produce data
void writeNums(char *bytes, size_t len, int i)
{
	for (size_t j = 0; j < len - 1; j++)
	{
		bytes[j] = '0' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeLetters(char *bytes, size_t len, int i)
{
	for (size_t j = 0; j < len - 1; j++)
	{
		bytes[j] = 'i' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeSymbols(char *bytes, size_t len, int i)
{
	for (size_t j = 0; j < len - 1; j++)
	{
		bytes[j] = '!' + (i % 10);
	}
	bytes[len - 1] = 0;
}

int main()
{

	QUnit::UnitTest qunit(cerr, QUnit::verbose);

	// UNIT TEST 1: A BIG ONE!!
	{

		// create a buffer manager
		MyDB_BufferManager myMgr(64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared<MyDB_Table>("tempTable", "foobar");

		// allocate a pinned page
		cout << "allocating pinned page\n";
		MyDB_PageHandle pinnedPage = myMgr.getPinnedPage(table1, 0);
		char *bytes = (char *)pinnedPage->getBytes();
		writeNums(bytes, 64, 0);
		pinnedPage->wroteBytes();

		// create a bunch of pinned pages and remember them
		vector<MyDB_PageHandle> myHandles;
		for (int i = 1; i < 10; i++)
		{
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage(table1, i);
			char *bytes = (char *)temp->getBytes();
			writeNums(bytes, 64, i);
			temp->wroteBytes();
			myHandles.push_back(temp);
		}

		// now forget the pages we created
		vector<MyDB_PageHandle> temp;
		myHandles = temp;

		// now remember 8 more pages
		for (int i = 0; i < 8; i++)
		{
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage(table1, i);
			char *bytes = (char *)temp->getBytes();

			// write numbers at the 0th position
			if (i == 0)
				writeNums(bytes, 64, i);
			else
				writeSymbols(bytes, 64, i);
			temp->wroteBytes();
			myHandles.push_back(temp);
		}

		// now correctly write nums at the 0th position
		cout << "allocating unpinned page\n";
		MyDB_PageHandle anotherDude = myMgr.getPage(table1, 0);
		bytes = (char *)anotherDude->getBytes();
		writeSymbols(bytes, 64, 0);
		anotherDude->wroteBytes();

		// now do 90 more pages, for which we forget the handle immediately
		for (int i = 10; i < 100; i++)
		{
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage(table1, i);
			char *bytes = (char *)temp->getBytes();
			writeNums(bytes, 64, i);
			temp->wroteBytes();
		}

		// now forget all of the pinned pages we were remembering
		vector<MyDB_PageHandle> temp2;
		myHandles = temp2;

		// now get a pair of pages and write them
		for (int i = 0; i < 100; i++)
		{
			cout << "allocating pinned page\n";
			MyDB_PageHandle oneHandle = myMgr.getPinnedPage();
			char *bytes = (char *)oneHandle->getBytes();
			writeNums(bytes, 64, i);
			oneHandle->wroteBytes();
			cout << "allocating pinned page\n";
			MyDB_PageHandle twoHandle = myMgr.getPinnedPage();
			writeNums(bytes, 64, i);
			twoHandle->wroteBytes();
		}

		// make a second table
		MyDB_TablePtr table2 = make_shared<MyDB_Table>("tempTable2", "barfoo");
		for (int i = 0; i < 100; i++)
		{
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage(table2, i);
			char *bytes = (char *)temp->getBytes();
			writeLetters(bytes, 64, i);
			temp->wroteBytes();
		}
	}

	// UNIT TEST 2
	{
		MyDB_BufferManager myMgr(64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared<MyDB_Table>("tempTable", "foobar");

		// look up all of the pages, and make sure they have the correct numbers
		for (int i = 0; i < 100; i++)
		{
			MyDB_PageHandle temp = myMgr.getPage(table1, i);
			char answer[64];
			if (i < 8)
				writeSymbols(answer, 64, i);
			else
				writeNums(answer, 64, i);
			char *bytes = (char *)temp->getBytes();
			QUNIT_IS_EQUAL(string(answer), string(bytes));
		}
	}
	// UNIT TEST 3: unpin 與 pinned 不被淘汰（基本）
	{
		MyDB_BufferManager myMgr(64, 16, "tempDSFSD3");
		MyDB_TablePtr table = make_shared<MyDB_Table>("T3", "fileT3");

		// 先 pin 住一頁，寫入 'A'
		auto keep = myMgr.getPinnedPage(table, 0);
		char *kb = (char *)keep->getBytes();
		for (int j = 0; j < 63; ++j)
			kb[j] = 'A';
		kb[63] = 0;
		keep->wroteBytes();

		// 再塞一堆其他頁（部分 unpinned），確保不會把 pinned 的 0 號頁踢掉
		for (int i = 1; i < 30; ++i)
		{
			auto h = myMgr.getPage(table, i); // unpinned
			char *b = (char *)h->getBytes();
			// 寫入 'B'+(i%10)
			for (int j = 0; j < 63; ++j)
				b[j] = 'B' + (i % 10);
			b[63] = 0;
			h->wroteBytes();
		}

		// 再次拿 pinned 的 0 號頁，檢查內容還在（沒有被淘汰）
		auto again = myMgr.getPage(table, 0);
		char *ab = (char *)again->getBytes();
		QUNIT_IS_EQUAL(std::string(kb), std::string(ab));

		// 測試手動 unpin：解除 pin 後，大量配置應可把它淘汰（不掛住）
		myMgr.unpin(keep);
		// 釋放 keep 的 handle，本身 destructor 也不該爆
	}

	// UNIT TEST 4: 多頁 dirty non-anon 在淘汰/析構時會寫回；重開可讀回
	{
		const char *pathTP = "persist_tp_multi";
		const char *pathTP2 = "persist_tp2_pressure";

		// 想驗證的一批頁號（任意挑幾個分散的）
		vector<int> targets = {2, 7, 13, 29, 41};

		// 第一次：寫 TP 的多個頁變髒，改用 TP2 製造壓力迫使淘汰
		{
			MyDB_BufferManager mgr(64, /*numPages*/ 6, "tmp_eviction_multi_a");
			MyDB_TablePtr tp = make_shared<MyDB_Table>("TP", pathTP);
			MyDB_TablePtr tp2 = make_shared<MyDB_Table>("TP2", pathTP2);

			// 對每個 target page 寫入不同可辨識圖樣：'A'+(i%26)
			for (int pno : targets)
			{
				auto h = mgr.getPage(tp, pno); // 不一定要 pinned，作業對非 pinned 也要能寫回
				char *b = (char *)h->getBytes();
				char ch = 'A' + (pno % 26);
				for (int j = 0; j < 63; ++j)
					b[j] = ch;
				b[63] = 0;
				h->wroteBytes();
			}

			// 用另一張表瘋狂塞頁面，迫使 TP 的頁被淘汰（CLOCK/LRU 都會被打爆）
			for (int i = 0; i < 100; ++i)
			{
				auto x = mgr.getPage(tp2, i);
				char *xb = (char *)x->getBytes();
				char ch2 = 'a' + (i % 26);
				for (int j = 0; j < 63; ++j)
					xb[j] = ch2;
				xb[63] = 0;
				x->wroteBytes();
			}
			// 離開 scope 會呼叫 ~MyDB_BufferManager()，再把殘留的 dirty 也 flush
		}

		// 第二次：新建 manager，從磁碟讀回 TP 的那些頁，應與先前寫入一致
		{
			MyDB_BufferManager mgr2(64, /*numPages*/ 6, "tmp_eviction_multi_b");
			MyDB_TablePtr tp = make_shared<MyDB_Table>("TP", pathTP);

			for (int pno : targets)
			{
				auto h = mgr2.getPage(tp, pno);
				char *b = (char *)h->getBytes();

				char expect[64];
				char ch = 'A' + (pno % 26);
				for (int j = 0; j < 63; ++j)
					expect[j] = ch;
				expect[63] = 0;

				QUNIT_IS_EQUAL(std::string(expect), std::string(b));
			}
		}
	}

	// UNIT TEST 5: 匿名頁（temp）基本行為（初始 0、可 pinned）
	{
		MyDB_BufferManager myMgr(64, 8, "tempDSFSD5");

		// 建兩個匿名 pinned 頁，檢查初始為 0 清空
		auto a = myMgr.getPinnedPage(); // anonymous + pinned
		auto b = myMgr.getPinnedPage();

		char *ab = (char *)a->getBytes();
		char *bb = (char *)b->getBytes();

		// 預期一開始全 0（字串等於空字串）
		QUNIT_IS_EQUAL(std::string(""), std::string(ab));
		QUNIT_IS_EQUAL(std::string(""), std::string(bb));

		// 對 a 寫入 'Z'，b 不動
		for (int j = 0; j < 63; ++j)
			ab[j] = 'Z';
		ab[63] = 0;
		a->wroteBytes();

		// 解除 a 的 pin，確保不會卡住
		myMgr.unpin(a);
	}

	// UNIT TEST 6: 不同 table 同頁號互不干擾
	{
		MyDB_BufferManager myMgr(64, 8, "tempDSFSD6");
		MyDB_TablePtr tA = make_shared<MyDB_Table>("TA", "fileTA");
		MyDB_TablePtr tB = make_shared<MyDB_Table>("TB", "fileTB");

		// 在兩個 table 都寫 page 2，但內容不同
		auto a2 = myMgr.getPage(tA, 2);
		auto b2 = myMgr.getPage(tB, 2);

		char *a2b = (char *)a2->getBytes();
		char *b2b = (char *)b2->getBytes();

		for (int j = 0; j < 63; ++j)
			a2b[j] = 'A';
		a2b[63] = 0;
		a2->wroteBytes();

		for (int j = 0; j < 63; ++j)
			b2b[j] = 'B';
		b2b[63] = 0;
		b2->wroteBytes();

		// 再取一次，看內容各自獨立
		auto a2r = myMgr.getPage(tA, 2);
		auto b2r = myMgr.getPage(tB, 2);
		QUNIT_IS_EQUAL(std::string("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"),
					   std::string((char *)a2r->getBytes()));
		QUNIT_IS_EQUAL(std::string("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"),
					   std::string((char *)b2r->getBytes()));
	}
}

#endif
