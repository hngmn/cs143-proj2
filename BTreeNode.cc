#include "BTreeNode.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

// Return codes
const int RC_SUCCESS = 0;
const int RC_INVALID_KEY = -11;
const int RC_INVALID_RECORD = -12;
const int RC_SIBLING_NOT_EMPTY = -13;
// RC_INVALID_PID is already defined
// RC_NODE_FULL is already defined
// RC_NO_SUCH_RECORD is already defined

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
	clearBuffer();
	numKeys = 0;
}

RC BTLeafNode::clearBuffer() {
	memset(buffer, 0, PageFile::PAGE_SIZE);
	return RC_SUCCESS;
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

	return RC_SUCCESS;
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

	return RC_SUCCESS;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount(){
	return numKeys;
}

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
		return RC_NODE_FULL;

	// Traverse buffer and find the correct spot to put pair in
	// The correct spot will either be an empty spot (null)
	// OR when the key is less than the key at traverse
	while (*traverse) {
		int traverseKey;
		memcpy(&traverseKey, traverse, sizeof(int));

		if (key < traverseKey)
			break;

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
	return RC_SUCCESS;
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
                              BTLeafNode& sibling, int& siblingKey) {

	// Validate parameters
	if (sibling.getKeyCount())
		return RC_SIBLING_NOT_EMPTY;
	else if (key < 0)
		return RC_INVALID_KEY;
	else if (rid.pid < 0 || rid.sid < 0)
		return RC_INVALID_RECORD;

	// Parameters are valid

	// Get number of keys to store in each
	// And calculate the offset of the midway point
	int numHalfKeys = (getKeyCount() + 1) / 2;
	sibling.numKeys = getKeyCount() - numHalfKeys;
	numKeys = getKeyCount() - sibling.getKeyCount();

	int midOffset = numHalfKeys * RECORD_PAIR_SIZE;

	// Copy second half of node into sibling
	// Set next node pointer to this node's next node pointer
	memcpy(sibling.buffer, buffer + midOffset, PageFile::PAGE_SIZE - midOffset);
	sibling.setNextNodePtr(getNextNodePtr());

	// Erase second half of the node
	// Will set key of this later, because it could be the newly inserted key
	memset(buffer + midOffset, 0, midOffset);
	

	// Choose which buffer to insert new key into
	int siblingFirstKey;
	int thisNodeFirstKey;

	memcpy(&siblingFirstKey, sibling.buffer, sizeof(int));
	memcpy(&thisNodeFirstKey, buffer, sizeof(int));

	if (key < siblingFirstKey) // Belongs in lower half
		insert(key, rid);
	else // Belongs in upper half
		sibling.insert(key, rid);

	// Set the next node pointer for this node
	// TODO: is the next node pointer (a PageId) supposed to be 
	// siblings first record id OR siblings first key?
	RecordId siblingFirstRecordId;

	memcpy(&siblingFirstRecordId, sibling.buffer + sizeof(int), sizeof(RecordId));
	setNextNodePtr(siblingFirstRecordId.pid);

	// Store first sibling key in siblingKey
	memcpy(&siblingKey, sibling.buffer, sizeof(int));

	// Success
	return RC_SUCCESS;
}

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
RC BTLeafNode::locate(int searchKey, int& eid){

	int offset = 0;
	char* traverse = buffer;

	// Traverse buffer and find searchKey
	// Stop when searchKey is found OR searchKey > 
	while (*traverse) {
		int traverseKey;
		memcpy(&traverseKey, traverse, sizeof(int));

		// Set eid to current index
		eid = offset / RECORD_PAIR_SIZE;

		// Found, return Success
		if (searchKey == traverseKey)
			return RC_SUCCESS;

		// traverseKey is greater than searchKey, thus there is no record
		if (searchKey < traverseKey)
			return RC_NO_SUCH_RECORD;

		offset += RECORD_PAIR_SIZE;
		traverse += RECORD_PAIR_SIZE;
	}

	// Reached end, searchKey is greater than all keys
	return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid){

	int offset = eid * RECORD_PAIR_SIZE;
	int traverseKey;

	// Validate key
	memcpy(&traverseKey, buffer + offset, sizeof(int));

	if (!traverseKey)
		return RC_INVALID_KEY;

	// key is valid
	memcpy(&key, buffer + offset, sizeof(int));
	memcpy(&rid, buffer + offset + sizeof(int), sizeof(RecordId));

	return RC_SUCCESS;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr(){

	PageId pid;
	memcpy(&pid, buffer + PageFile::PAGE_SIZE - sizeof(PageId), sizeof(PageId));

	return pid;
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid){

	// Invalid pid
	if (pid < 0)
		return RC_INVALID_PID;

	// pid is valid
	memcpy(buffer + PageFile::PAGE_SIZE - sizeof(PageId), &pid, sizeof(PageId));
	return RC_SUCCESS;
}

void BTLeafNode::print() {
	
	char* traverse = buffer;

	while (*traverse) {
		int key;
		RecordId recordId;
		
		memcpy(&key, traverse, sizeof(int));
		memcpy(&recordId, traverse + sizeof(int), sizeof(RecordId));

		cerr << "Key: " << key;
		cerr << " RecordId.pid: " << recordId.pid;
		cerr << " RecordId.sid: " << recordId.sid;
		cerr << endl;

		traverse += RECORD_PAIR_SIZE;
	}

	cerr << "Next Node Ptr: " << getNextNodePtr() << endl;
	cerr << endl;
}

//////////////////////////////////////////////////////////////////////
/////////// BTNonLeafNode ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


BTNonLeafNode::BTNonLeafNode() {
	clearBuffer();
	numKeys = 0;
}

RC BTNonLeafNode::clearBuffer() {
	memset(buffer, 0, PageFile::PAGE_SIZE);
	return RC_SUCCESS;
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

	return RC_SUCCESS;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf) {
	
	int rc;

	if (rc = pf.write(pid, buffer)) {
		cerr << "Error Code: " << rc << endl;
	}

	return RC_SUCCESS;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount(){
	return numKeys;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid){

	// NOTE: non leaf nodes have a lone pid at the beginning
	// i.e. pid,key,pid,key,pid

	return RC_SUCCESS;
}

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
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2) {
	clearBuffer();

	// Validate inputs
	if (pid1 < 0 || pid2 < 0)
		return RC_INVALID_PID;

	if (key < 0)
		return RC_INVALID_KEY;

	// key, pid1, and pi2 are valid
	memcpy(buffer, &pid1, sizeof(PageId));
	memcpy(buffer + sizeof(PageId), &key, sizeof(int));
	memcpy(buffer + sizeof(PageId) + sizeof(int), &pid2, sizeof(PageId));

	return RC_SUCCESS;
}

void BTNonLeafNode::print() {

	char* traverse = buffer;

	// Currently prints only keys, not pageids
	// so start at first key
	traverse += sizeof(PageId);

	while (*traverse) {
		int key;
		// int pageId;
		
		memcpy(&key, traverse, sizeof(int));
		// memcpy(&pageId, traverse + sizeof(int), sizeof(PageId));

		cerr << "Key: " << key;
		// cerr << " Page Id: " << pageId;
		cerr << endl;

		traverse += PAGE_PAIR_SIZE;
	}
}

// For testing
int main() {
	BTLeafNode* leafNode = new BTLeafNode();
	BTNonLeafNode* nonLeafNode = new BTNonLeafNode();

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
	BTLeafNode* leafNode1 = new BTLeafNode();
	BTLeafNode leafNode2;

	int siblingKey;

	leafNode1->insert(1, RecordId{1,10});
	leafNode1->insert(3, RecordId{3,30});
	leafNode1->insert(5, RecordId{5,50});
	leafNode1->insert(2, RecordId{2,20});

	leafNode1->insertAndSplit(4, RecordId{4, 40}, leafNode2, siblingKey);

	cerr << "Leaf Node 1" << endl;
	leafNode1->print();

	cerr << "Leaf Node 2" << endl;
	leafNode2.print();

	// leafNode->print();
	// nonLeafNode->print();
}