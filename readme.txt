--------------------------
Project 2A

Johnathan Estacio (johnathan.estacio@gmail.com)
Sean Kim (sean.hk127@gmail.com)

--------------------------
TODO

Check if all return error codes are correct

BTLeafNode::insertAndSplit
- BUG: has a duplicate tuple
- setNextNodePtr(PageId) for this node (not sibling)
	- what do we send as PageId? Is it the first RecordId.pid in sibling, or is it the first key? 

BTNonLeafNode::insertAndSplit
- what if the key being inserted is a median key?
	- we need to disregard it
