#include <iostream>
#include <stdio.h>
#include "Bruinbase.h"
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

// For testing
int main() {
  // BTREENODE TESTING CODE //////////////////////////////////////////////////
	//BTLeafNode* leafNode = new BTLeafNode();
	//BTNonLeafNode* nonLeafNode = new BTNonLeafNode();

	// // BTLeafNode::insert
	// for (int i = 0; i < MAX_NUM_RECORD_KEYS + 1; i++) {
	// 	if (leafNode->insert(1, RecordId{0, 1})) {
	// 		cout << "RC_NODE_FULL" << endl;
	// 		break;
	// 	}
	// }

	// // BTLeafNode::locate
	// int eid = -1;
	// leafNode->insert(1, RecordId{0,0});
	// leafNode->insert(3, RecordId{0,0});
	// leafNode->insert(5, RecordId{0,0});
	// leafNode->insert(2, RecordId{0,0});

	// leafNode->locate(3, eid);	
	// cout << eid << endl;

	// leafNode->locate(4, eid);	
	// cout << eid << endl;

	// BTLeafNode::getNextNodePtr
	// cerr << "Next Node Ptr: " << leafNode->getNextNodePtr() << endl;

	// PageId pid = 10;
	// leafNode->setNextNodePtr(pid);
	// cerr << "Next Node Ptr: " << leafNode->getNextNodePtr() << endl;

	// pid = 0;
	// leafNode->setNextNodePtr(pid);
	// cerr << "Next Node Ptr: " << leafNode->getNextNodePtr() << endl;

	// BTLeafNode::readEntry
	// leafNode->insert(1, RecordId{1,10});
	// leafNode->insert(3, RecordId{3,30});
	// leafNode->insert(5, RecordId{5,50});
	// leafNode->insert(2, RecordId{2,20});

	// cerr << leafNode->getKeyCount() << endl;

	// int key;
	// RecordId rid;

	// leafNode->readEntry(0, key, rid); // key: 1
	// cerr << "key: " << key << endl;
	// cerr << "rid.pid: " << rid.pid;
	// cerr << " rid.sid: " << rid.sid << endl;
	// cerr << "entry: " << "0" << endl << endl;

	// leafNode->readEntry(1, key, rid); // key: 2
	// cerr << "key: " << key << endl;
	// cerr << "rid.pid: " << rid.pid;
	// cerr << " rid.sid: " << rid.sid << endl;
	// cerr << "entry: " << "1" << endl << endl;

	// leafNode->readEntry(3, key, rid); // key: 5
	// cerr << "key: " << key << endl;
	// cerr << "rid.pid: " << rid.pid;
	// cerr << " rid.sid: " << rid.sid << endl;
	// cerr << "entry: " << "3" << endl << endl;

	// // BTNonLeafNode::initializeRoot
	// nonLeafNode->initializeRoot(1,12,3);
	// nonLeafNode->print();

	// BTLeafNode::insertAndSplit
	// BTLeafNode leafNode1;
	// BTLeafNode leafNode2;

	// int siblingKey;

	// leafNode1.insert(1, RecordId{1,10});
	// leafNode1.insert(3, RecordId{3,30});
	// leafNode1.insert(5, RecordId{5,50});
	// leafNode1.insert(2, RecordId{2,20});

	// leafNode1.insertAndSplit(4, RecordId{4, 40}, leafNode2, siblingKey);

	// cerr << "Leaf Node 1" << endl;
	// leafNode1.print();

	// cerr << "Leaf Node 2" << endl;
	// leafNode2.print();

	// BTNonLeafNode::insert
	// nonLeafNode->insert(1, 10);
	// nonLeafNode->insert(3, 30);
	// nonLeafNode->insert(5, 50);
	// nonLeafNode->insert(2, 20);

	// BTLeafNode::insertAndSplit
	// BTNonLeafNode nonLeafNode1;
	// BTNonLeafNode nonLeafNode2;

	// int siblingKey;

	// nonLeafNode1.insert(1, 1);
	// nonLeafNode1.insert(2, 2);
	// nonLeafNode1.insert(3, 3);
	// nonLeafNode1.insert(4, 4);

	// nonLeafNode1.insertAndSplit(5, 5, nonLeafNode2, siblingKey);

	// cerr << "nonLeafNode1" << endl;
	// nonLeafNode1.print();

	// cerr << "nonLeafNode2" << endl;
	// nonLeafNode2.print();

	// PageId result;

	// nonLeafNode->locateChildPtr(0, result);

	// cerr << result << endl;

	// leafNode->print();
	// nonLeafNode->print();

  // BTREEINDEX TESTING CODE /////////////////////////////////////////////////
	// BTreeIndex* index = new BTreeIndex();

	// char fakeBuffer[PageFile::PAGE_SIZE];

	// BTreeIndex::open
	// PageFile* test1 = new PageFile("test1", 'w');
	// PageId testRootId = 1;
	// int testTreeHeight = 2;

	// memcpy(fakeBuffer, &testRootId, sizeof(PageId));
	// memcpy(fakeBuffer + sizeof(PageId), &testTreeHeight, sizeof(int));

	// if (test1->write(0, fakeBuffer))
	// 	cerr << "error" << endl;

	// index->open("test1", 'w');
	// // index->insert(2, RecordId{2, 20});
	// index->close();
	// index->print();

	// BTreeIndex::insertRec (2.)
	BTreeIndex test;

	test.open("testFile", 'w');


	for (int i = 1; i < 10000; i++)
	 	test.insert(i, RecordId{i, i});

	// BTNonLeafNode nonLeaf1;
	// nonLeaf1.read(44, test.pf);
	// nonLeaf1.print();

	// BTNonLeafNode nonLeaf2;
	// nonLeaf2.read(86, test.pf);
	// nonLeaf2.print();

	// BTLeafNode first;
	// first.read(84, test.pf);
	// first.print();

	// BTLeafNode second;
	// second.read(43, test.pf);
	// second.print();

	// BTLeafNode third;
	// third.read(69, test.pf);
	// third.print();

	// BTreeIndex::readForward
	// IndexCursor cursor;
	// cursor.eid = 42;
	// cursor.pid = 1;

	// RecordId rid;
	// int key;

	// test.readForward(cursor, key, rid);

	// cerr << "cursor.eid: " << cursor.eid;
	// cerr << " cursor.pid: " << cursor.pid << endl;
	// cerr << "key: " << key << endl;
	// cerr << "rid.pid: " << rid.pid << endl;
	// cerr << endl;

	// test.readForward(cursor, key, rid);

	// cerr << "cursor.eid: " << cursor.eid;
	// cerr << " cursor.pid: " << cursor.pid << endl;
	// cerr << "key: " << key << endl;
	// cerr << "rid.pid: " << rid.pid << endl;
	// cerr << endl;

	// BTreeIndex::locate
	// IndexCursor cursor;
	// int key = 95;

	// test.locate(key, cursor);

	// cerr << "cursor.eid: " << cursor.eid;
	// cerr << " cursor.pid: " << cursor.pid << endl;
	// cerr << "key: " << key << endl;
	// cerr << endl;

	// test.print();
	test.close();

	return RC_SUCCESS;
}
