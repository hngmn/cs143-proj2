--------------------------
Project 2A

Johnathan Estacio (johnathan.estacio@gmail.com)
Sean Kim (sean.hk127@gmail.com)

--------------------------
TODO

Check if all return error codes are correct

BTNonLeafNode::insertAndSplit
- Doesn't correctly split the node
	- The first half splits correctly, however the second half (sibling)
	ends up having the first pid in the buffer being pid = 0, and then an extraneous key,
	and then the correct output
