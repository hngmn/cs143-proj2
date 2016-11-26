#include "BTreeNode.h"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

// Size of (key, id) pairs
const int RECORD_PAIR_SIZE = sizeof(int) + sizeof(RecordId);
const int PAGE_PAIR_SIZE = sizeof(int) + sizeof(PageId);

// Max number of keys
const int MAX_NUM_RECORD_KEYS = 85;
const int MAX_NUM_PAGE_KEYS = 40;
// const int MAX_NUM_PAGE_KEYS = (PageFile::PAGE_SIZE - sizeof(PageId)) / PAGE_PAIR_SIZE;
// const int MAX_NUM_RECORD_KEYS = (PageFile::PAGE_SIZE - sizeof(PageId)) / RECORD_PAIR_SIZE;

//////////////////////////////////////////////////////////////////////
/////////// BTLeafNode ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

BTLeafNode::BTLeafNode() {
	clearBuffer();
	numKeys = 0;
	pid_ = -1;
}

BTLeafNode::BTLeafNode(PageId pid) {
	clearBuffer();
	numKeys = 0;
	pid_ = pid;
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
		return rc;
	}

	numKeys = getKeyCount();

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
		return rc;
	}

	return RC_SUCCESS;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount(){
	int count = 0;
	char* traverse = buffer;

	while (count < MAX_NUM_RECORD_KEYS && *traverse) {
		count++;
		traverse += RECORD_PAIR_SIZE;
	}

	return count;
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
	char newBuffer[PageFile::PAGE_SIZE] = {0};
	memcpy(newBuffer, buffer, offset); // buffer[0] to buffer[offset]
	memcpy(newBuffer + offset, &key, sizeof(int)); // key
	memcpy(newBuffer + offset + sizeof(int), &rid, sizeof(RecordId)); // rid

	// buffer[offset] to buffer[PageFile::PAGE_SIZE]
	memcpy(newBuffer + offset + RECORD_PAIR_SIZE, buffer + offset, numKeys * RECORD_PAIR_SIZE - offset);
	memcpy(newBuffer + PageFile::PAGE_SIZE - sizeof(PageId), buffer + PageFile::PAGE_SIZE - sizeof(PageId), sizeof(PageId));

	// Reassign newBuffer to buffer and free memory
	memcpy(buffer, newBuffer, PageFile::PAGE_SIZE);

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

	// Get number of half keys
	// And calculate the offset of the midway point
	int numHalfKeys = (getKeyCount() + 1) / 2;

	// Get number of keys in each
	sibling.numKeys = getKeyCount() - numHalfKeys;
	numKeys = getKeyCount() - sibling.numKeys;

	int midOffset = numHalfKeys * RECORD_PAIR_SIZE;

	// Copy second half of node into sibling
	memcpy(sibling.buffer, buffer + midOffset, PageFile::PAGE_SIZE - midOffset);

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

	cerr << "numKeys: " << numKeys << endl;

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
	pid_ = -1;
}

BTNonLeafNode::BTNonLeafNode(PageId pid) {
	clearBuffer();
	numKeys = 0;
	pid_ = pid;
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
		return rc;
	}

	numKeys = getKeyCount();

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
		return rc;
	}

	return RC_SUCCESS;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount(){
	int count = 0;
	char* traverse = buffer + sizeof(PageId);

	while (count < MAX_NUM_PAGE_KEYS && *traverse) {
		count++;
		traverse += PAGE_PAIR_SIZE;
	}

	return count;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid){

	// Non leaf nodes have a lone pid at the beginning
	// i.e. pid, key, pid
	int offset = sizeof(PageId);
	char* traverse = buffer + offset;

	// numKeys is already at the maximum number of keys
	if (numKeys == MAX_NUM_PAGE_KEYS) {
		return RC_NODE_FULL;
	}

	// Traverse buffer and find the correct spot to put pair in
	// The correct spot will either be an empty spot (null)
	// OR when the key is less than the key at traverse
	while (*traverse) {
		int traverseKey;
		memcpy(&traverseKey, traverse, sizeof(int));

		if (key < traverseKey)
			break;

		offset += PAGE_PAIR_SIZE;
		traverse += PAGE_PAIR_SIZE;
	}

	// Create a new buffer with
	// buffer[0] to buffer[offset], (key, rid), buffer[offset] to buffer[PageFile::PAGE_SIZE]
	char newBuffer[PageFile::PAGE_SIZE] = {0};
	memcpy(newBuffer, buffer, offset); // buffer[0] to buffer[offset]
	memcpy(newBuffer + offset, &key, sizeof(int)); // key
	memcpy(newBuffer + offset + sizeof(int), &pid, sizeof(PageId)); // pid

	// buffer[offset] to buffer[PageFile::PAGE_SIZE]
	memcpy(newBuffer + offset + PAGE_PAIR_SIZE, buffer + offset, numKeys * PAGE_PAIR_SIZE);
	memcpy(newBuffer + PageFile::PAGE_SIZE - sizeof(PageId), buffer + PageFile::PAGE_SIZE - sizeof(PageId), sizeof(PageId));

	// Reassign newBuffer to buffer and free memory
	memcpy(buffer, newBuffer, PageFile::PAGE_SIZE);

	// Success
	numKeys++;
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
{
  // impl notes: -sean
  // 
  // I went with calculating left-cut (lc) and right-cut (rc) as below:
  //     lc = sizeof(p) + (nkeys/2 * RECORD_PAIR_SIZE)
  //     rc = lc + sizeof(k);
  // Where p is PageId and k is key
  // This results in cuts as shown below:
  // 
  // case 1: k-odd, p-even
  // +-------------------------------------------+
  // | p | k | p | k | p | k | p | k | p | k | p |
  // +-------------------------------------------+
  //                     |   |
  //                    lc   rc
  //  <----left node---->     <----right node--->
  // 
  // case 2: k-even, p-odd
  // +---------------------------------------------------+
  // | p | k | p | k | p | k | p | k | p | k | p | k | p |
  // +---------------------------------------------------+
  //                             |   |
  //                            lc   rc
  //  <--------left node-------->     <----right node--->
  // 

	// Validate parameters
	if (sibling.getKeyCount())
		return RC_SIBLING_NOT_EMPTY;
	else if (key < 0)
		return RC_INVALID_KEY;
	else if (pid < 0)
		return RC_INVALID_PID;

	// Parameters are valid

	// Get number of keys to store in original
	int numHalfKeys = (getKeyCount() + 1) / 2;

  // leftcut and right cut are the left and right side of the key that will be 'deleted'
  int leftcut = sizeof(PageId) + (numHalfKeys * RECORD_PAIR_SIZE);
  int rightcut = leftcut + sizeof(int);

	// Copy second half of node into sibling
	memcpy(sibling.buffer, buffer + rightcut, PageFile::PAGE_SIZE - rightcut);

	// Erase second half of original
	memset(buffer + leftcut, 0, PageFile::PAGE_SIZE - leftcut);

	// Choose which buffer to insert new key into
	int siblingFirstKey;
	int thisNodeFirstKey;

  // get the first key of each node; they are one PageId offset from start of buffer
	memcpy(&siblingFirstKey, sibling.buffer + sizeof(PageId), sizeof(int));
	memcpy(&thisNodeFirstKey, buffer + sizeof(PageId), sizeof(int));

	if (key < siblingFirstKey) // Belongs in lower half
		insert(key, pid);
	else // Belongs in upper half
		sibling.insert(key, pid);

	// output sibling's first key in midKey
	memcpy(&midKey, sibling.buffer + sizeof(PageId), sizeof(int));

	// Success
	return RC_SUCCESS;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
	char *p = buffer + sizeof(PageId); // traversal pointer
	int key; // traversal key

	while (*p != 0) {
  		memcpy(&key, p, sizeof(int));

		if (key > searchKey) {
			// Return pid on left side of key
			memcpy(&pid, p-sizeof(PageId), sizeof(PageId));
			return RC_SUCCESS;
		}

		p += PAGE_PAIR_SIZE;
	}

	// Reached the end of the node, return the last pid
	memcpy(&pid, p-sizeof(PageId), sizeof(PageId));
	return RC_SUCCESS;
}

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
	PageId initialPageId;

	memcpy(&initialPageId, traverse, sizeof(PageId));

	cerr << "Initial Page Id: " <<  initialPageId << endl;

	traverse += sizeof(PageId);

	while (*traverse) {
		int key;
		PageId pageId;
		
		memcpy(&key, traverse, sizeof(int));
		memcpy(&pageId, traverse + sizeof(int), sizeof(PageId));

		cerr << "Key: " << key << endl;
		cerr << "Page Id: " << pageId << endl;

		traverse += PAGE_PAIR_SIZE;
	}

	cerr << endl;
}

