#include "BTreeNode.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

// Error codes
const int ERROR_NODE_FULL = 1;

// Size of (key, id) pairs
const int RECORD_PAIR_SIZE = sizeof(int) + sizeof(RecordId);
const int PAGE_PAIR_SIZE = sizeof(int) + sizeof(PageId);

// Max number of keys
const int MAX_NUM_PAGE_KEYS = (PageFile::PAGE_SIZE - sizeof(PageId)) / PAGE_PAIR_SIZE;
const int MAX_NUM_RECORD_KEYS = (PageFile::PAGE_SIZE - sizeof(PageId)) / RECORD_PAIR_SIZE;

//////////////////////////////////////////////////////////////////////
/////////// BTLeafNode ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

BTLeafNode::BTLeafNode() {
	numKeys = 0;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf){
	
	int rc;

	// Read in page into memory buffer
	if (rc = pf.read(pid, buffer)) {
		cerr << "Error Code: " << rc << endl;
	}

	return 0;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf){
	
	int rc;

	if (rc = pf.write(pid, buffer)) {
		cerr << "Error Code: " << rc << endl;
	}

	return 0;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{ return 0; }

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid){

	int offset = 0;
	char* traverse = buffer;

	// numKeys is already at the maximum number of keys
	if (numKeys == MAX_NUM_RECORD_KEYS)
		return ERROR_NODE_FULL;

	// Traverse buffer and find the correct spot to put pair in
	// The correct spot will either be an empty spot (null)
	// OR when the key is less than the key at traverse
	while (*traverse) {
		offset += RECORD_PAIR_SIZE;
		traverse += RECORD_PAIR_SIZE;
	}

	// Create a new buffer with
	// buffer[0] to buffer[offset], (key, rid), buffer[offset] to buffer[PageFile::PAGE_SIZE]
	char* newBuffer = (char *) malloc(PageFile::PAGE_SIZE);
	memcpy(newBuffer, buffer, offset); // buffer[0] to buffer[offset]
	memcpy(newBuffer + offset, &key, sizeof(int)); // key
	memcpy(newBuffer + offset + sizeof(int), &rid, sizeof(RecordId)); // rid

	// buffer[offset] to buffer[PageFile::PAGE_SIZE]
	memcpy(newBuffer + offset + RECORD_PAIR_SIZE, buffer + offset, numKeys*RECORD_PAIR_SIZE-offset);
	memcpy(newBuffer + PageFile::PAGE_SIZE-sizeof(PageId), buffer + PageFile::PAGE_SIZE-sizeof(PageId), sizeof(PageId));

	// Reassign newBuffer to buffer and free memory
	memcpy(buffer, newBuffer, PageFile::PAGE_SIZE);
	free(newBuffer);

	// Success
	numKeys++;
	return 0;
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ return 0; }

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ return 0; }

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ return 0; }

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ return 0; }

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ return 0; }

void BTLeafNode::print() {
	
	char* traverse = buffer;

	while (*traverse) {
		int key;
		int recordId;
		
		memcpy(&key, traverse, sizeof(int));
		memcpy(&recordId, traverse + sizeof(int), sizeof(int));

		cerr << "Key: " << key;
		cerr << " Record Id: " << recordId;
		cerr << endl;

		traverse += RECORD_PAIR_SIZE;
	}
}

//////////////////////////////////////////////////////////////////////
/////////// BTNonLeafNode ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


BTNonLeafNode::BTNonLeafNode() {
	numKeys = 0;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf){
	
	int rc;

	// Read in page into memory buffer
	if (rc = pf.read(pid, buffer)) {
		cerr << "Error Code: " << rc << endl;
	}

	return 0;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ return 0; }

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ return 0; }


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ return 0; }

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ return 0; }

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ return 0; }

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ return 0; }

// For testing
int main() {
	BTLeafNode* leafNode = new BTLeafNode();

	for (int i = 0; i < MAX_NUM_RECORD_KEYS + 1; i++) {
		if (leafNode->insert(1, RecordId{0, 1})) {
			cout << "ERROR_NODE_FULL" << endl;
			break;
		}
	}

	leafNode->print();
}