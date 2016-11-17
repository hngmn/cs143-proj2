/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

// Return codes
const int RC_SUCCESS = 0;
const int RC_PF_CLOSE_ERROR = -10;
const int RC_PF_OPEN_ERROR = -11;
const int RC_PF_READ_ERROR = -12;
const int RC_PF_WRITE_ERROR = -13;
const int RC_INSERT_ERROR = -14;
// RC_NO_SUCH_RECORD already defined

const int RC_ADD_NEW_CHILD = 1;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex() {
    rootPid = -1;
    treeHeight = 0;
    clearBuffer();
}

RC BTreeIndex::clearBuffer() {
	memset(buffer, 0, PageFile::PAGE_SIZE);
	return RC_SUCCESS;
}

void BTreeIndex::print() {

	// Print BTreeIndex information
	cerr << "endPid: " << pf.endPid() << endl;
	cerr << "rootPid: " << rootPid << endl;
	cerr << "treeHeight: " << treeHeight << endl;
	cerr << endl;

	// // Print root node information
	BTNonLeafNode root(rootPid);
	root.read(rootPid, pf);
	root.print();
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode) {
    
    // Open the index file
	if (pf.open(indexname, mode))
		return RC_PF_OPEN_ERROR;

	// If endPid == 0, there are no disk pages currently stored,
	// so just return, otherwise, check the disk page with pid = 0 for
	// the rootPid and treeHeight
	//
	// NOTE: We'll reserve the page with pid = 0 for storing rootPid
	// and treeHeight. We'll create nodes starting with pid = 1 
	if (pf.endPid() != 0) {

		// Read pid = 0 into buffer
		if (pf.read(0, buffer))
			return RC_PF_READ_ERROR;

		// Get the rootId and treeHeight
		PageId tempRootId;
		int tempTreeHeight;

		memcpy(&tempRootId, buffer, sizeof(PageId));
		memcpy(&tempTreeHeight, buffer + sizeof(PageId), sizeof(int));

		// Store found values only if they are valid
		// We know that tempRootId must be > 0 because we reserved pid = 0
		if (tempRootId != 0 && tempTreeHeight >= 0) {
			rootPid = tempRootId;
			treeHeight = tempTreeHeight;
		}
	}

    return RC_SUCCESS;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close() {

	// Store rootPid and treeHeight into buffer
	memcpy(buffer, &rootPid, sizeof(PageId));
	memcpy(buffer + sizeof(PageId), &treeHeight, sizeof(int));

	// Write the buffer to pid = 0
	if (pf.write(0, buffer))
		return RC_PF_WRITE_ERROR;

	// Close the page file
	if (pf.close())
		return RC_PF_CLOSE_ERROR;

    return RC_SUCCESS;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid) {

	// TODO: There are four different cases upon inserting:
	// (COMPLETE) 1. New Root
	// (COMPLETE) 2. No Overflow
	// (COMPLETE) 3. Leaf Overflow
	// (INCOMPLETE) 4. Non-Leaf Overflow

	// 1. New Root
	if (treeHeight == 0) {

		// Create a new leaf node
		int nextPid = pf.endPid() == 0 ? 1 : pf.endPid();
		BTLeafNode leafNode;
		leafNode.insert(key, rid);

		// Create root node at next pid that is not pid = 0
		rootPid = nextPid;

		treeHeight++;

		// Write tree to the pid in pf
		return leafNode.write(rootPid, pf);
	}

	// We will insert the key and rid recursively for cases 2, 3, and 4.
	//
	// NOTE: The function signature will keep changing as we figure out what we need
	// to traverse recursively for each case.
	//
	// 2. We will start at currTreeHeight = 1 and reach currTreeHeight = treeHeight
	// so that we can insert at the leaf level.
	//
	// 3.
	//
	// 4.
	// 

	int newChildKey = -1;
	PageId newChildPid = -1;

	return insertRec(key, rid, 1, rootPid, newChildKey, newChildPid);
}

RC BTreeIndex::insertRec(int key, const RecordId& rid, int currTreeHeight, PageId currPid, int& newChildKey, PageId& newChildPid) {

	int error;

	// We've reach the leaf node level, so insert into the leaf node
	if (currTreeHeight == treeHeight) {

		// Read in leaf node
		BTLeafNode leafNode;
		leafNode.read(currPid, pf);

		// 2. No Overflow, parent node does not need to be split,
		// so just return success
		if (leafNode.insert(key, rid) == RC_SUCCESS) {
			leafNode.write(currPid, pf);
			return RC_SUCCESS;
		}

		// 3. Leaf Overflow
		int newLeafNodePid = pf.endPid();
		BTLeafNode newLeafNode;
		int newLeafNodeKey;

		if (error = leafNode.insertAndSplit(key, rid, newLeafNode, newLeafNodeKey)) {
			cerr << "Could not insert and split leaf node, error code: " << error << endl;
			return error;
		}

		// Set next node pointers
		newLeafNode.setNextNodePtr(leafNode.getNextNodePtr());
		leafNode.setNextNodePtr(newLeafNodePid);

		// Write leafNode and newLeafNode out to disk
		leafNode.write(currPid, pf);
		newLeafNode.write(newLeafNodePid, pf);

		// Not at root, so tell parent node to insert newLeafNode information
		if (currTreeHeight != 1) {
			newChildKey = newLeafNodeKey;
			newChildPid = newLeafNodePid;
			return RC_ADD_NEW_CHILD;
		}

		// At root, need to make a new root
		else {
			// Initialize a new root
			int newRootPid = pf.endPid();
			BTNonLeafNode newRoot;

			newRoot.initializeRoot(currPid, newLeafNodeKey, newLeafNodePid);
			newRoot.write(newRootPid, pf);

			// Update BTreeIndex
			rootPid = newRootPid;
			treeHeight++;

			// We are done
			return RC_SUCCESS;
		}


	// We are at a non leaf node level
	} else {

		// Read in current node
		BTNonLeafNode currNode;
		currNode.read(currPid, pf);

		// Locate the child pointer
		PageId childPid = -1;	

		if (currNode.locateChildPtr(key, childPid)) {
			cerr << "Could not locate childPid with key: " << key << endl;
			return RC_INSERT_ERROR;
		};

		// Recursively traverse deeper into the tree.
		// Keep note if the child split, and add the new pid to this node
		RC addNewChild;
		addNewChild = insertRec(key, rid, currTreeHeight + 1, childPid, newChildKey, newChildPid);

		// Child did not split, so do not need to add a new child
		if (!addNewChild)
			return RC_SUCCESS;
		
		// Child split, so we need to add a new child to this node
		else if (currNode.insert(newChildKey, newChildPid) == RC_SUCCESS) {
			currNode.write(currPid, pf);
			return RC_SUCCESS;
		}

		// Child split, but currNode is full, so split this node
		else {
			BTNonLeafNode newNode;
			PageId newNodePid = pf.endPid();
			int newNodeKey;

			if (error = newNode.insertAndSplit(newChildKey, newChildPid, newNode, newNodeKey)) {
				cerr << "Could not insert and split non leaf node, error code: " << error << endl;
				return error;
			}

			// Write currNode and newNode out to disk
			currNode.write(currPid, pf);
			newNode.write(newNodePid, pf);

			// Not at root, so tell parent node to insert newLeafNode information
			if (currTreeHeight != 1) {
				newChildKey = newNodeKey;
				newChildPid = newNodePid;
				return RC_ADD_NEW_CHILD;
			}

			// At root, need to make a new root
			else {
				// Initialize a new root
				int newRootPid = pf.endPid();
				BTNonLeafNode newRoot;

				cerr << "newRootPid: " << newRootPid << endl;

				newRoot.initializeRoot(currPid, newNodeKey, newNodePid);
				newRoot.write(newRootPid, pf);

				// Update BTreeIndex
				rootPid = newRootPid;
				treeHeight++;

				// We are done
				return RC_SUCCESS;
			}

			return RC_SUCCESS;
		}
	}

	return RC_SUCCESS;
}

/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor) {

	// Tree is empty
	if (treeHeight <= 0) {
		cerr << "Tree is empty." << endl;
		return RC_NO_SUCH_RECORD;
	}

	// Traverse down the tree
    return locateRec(1, rootPid, searchKey, cursor);
}

RC BTreeIndex::locateRec(int currTreeHeight, PageId currPid, int searchKey, IndexCursor& cursor) {
	
	// We are at the leaf
	if (currTreeHeight == treeHeight) {
		cursor.pid = currPid;

		// Read in leaf
		BTLeafNode leafNode;
		leafNode.read(currPid, pf);

		// searchKey is not in leafNode
		if (leafNode.locate(searchKey, cursor.eid)) {
			cerr << "Could not locate searchKey: " << searchKey;
			cerr << " in leafNode with pid: " << currPid << endl;
			cerr << endl;
			return RC_NO_SUCH_RECORD;
		}

		return RC_SUCCESS;
	}

	// We are at a non leaf node
	BTNonLeafNode currNode;
	currNode.read(currPid, pf);

	// Find child node to follow
	PageId childPid;
	currNode.locateChildPtr(searchKey, childPid);

	// Follow child node
	return locateRec(currTreeHeight + 1, childPid, searchKey, cursor);
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid) {
    
    RC error;
    BTLeafNode leafNode;

    // Read in the leaf node
    if (error = leafNode.read(cursor.pid, pf)) {
    	cerr << "Could not read from cursor.pid: " << cursor.pid << endl;
    	return error;
    }

    // Get (key, rid) from eid
   	if (error = leafNode.readEntry(cursor.eid, key, rid)) {
   		cerr << "Could not read entry cursor.eid: " << cursor.eid << endl;
   		return error;
   	}

   	// Get next entry

   	// If cursor is at the end of the node, go to next node pointer
   	if (cursor.eid + 1 >= leafNode.getKeyCount()) {
   		cursor.eid = 0;
   		cursor.pid = leafNode.getNextNodePtr();

   	// Else just go to the next entry
   	} else {
   		cursor.eid++;
   	}

    return RC_SUCCESS;
}
/*
int main() {

	BTreeIndex* index = new BTreeIndex();

	char fakeBuffer[PageFile::PAGE_SIZE];

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


	// for (int i = 1; i < 10000; i++)
	// 	test.insert(i, RecordId{i, i});

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
*/