
#ifndef STACK_H
#define STACK_H

// this is the node class used to build up the LIFO stack
template <class Data>
class Node {

private:

	Data holdMe;
	Node *next;
	
public:

	/*****************************************/
	/** WHATEVER CODE YOU NEED TO ADD HERE!! */
	/*****************************************/
	Node(const Data& d, Node<Data>* n) : holdMe(d), next(n) {}
    Node(Data&& d, Node<Data>* n) : holdMe(std::move(d)), next(n) {}

    const Data& value() const { return holdMe; }
    Data& value() { return holdMe; }
    Node<Data>* nextPtr() const { return next; }
    void setNext(Node<Data>* n) { next = n; }

};

// a simple LIFO stack
template <class Data> 
class Stack {

	Node <Data> *head;

public:

	// destroys the stack
	~Stack () { /* your code here */ 
		while (head != nullptr) {
			Node<Data>* old = head;   
			head = head -> nextPtr();       
			delete old;                    
    	}
	}

	// creates an empty stack
	Stack () { /* your code here */ 
		head = nullptr;
	}

	// adds pushMe to the top of the stack
	void push (Data x) { /* your code here */ 
	    Node<Data>* newNode = new Node<Data>(x, head);
    	head = newNode;
	}

	// return true if there are not any items in the stack
	bool isEmpty () { /* replace with your code */ 
		return head == nullptr; 
	}

	// pops the item on the top of the stack off, returning it...
	// if the stack is empty, the behavior is undefined
	Data pop () { /* replace with your code */ 

		Node<Data>* old = head;          
		head = head->nextPtr();          
		Data ret = std::move(old->value()); 
		delete old;                     
		return ret; 
	}
};

#endif
