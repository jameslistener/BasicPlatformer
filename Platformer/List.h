#pragma once
#define NULL 0

typedef unsigned int uint;

template <class T> class Element
{
private:
	T * obj;

public:
	Element<T> * next;
	Element<T> * prev;

	Element();
	Element(T * o, Element<T> *n = NULL, Element<T> *p = NULL);
	~Element();

	T *getObj();
	void clear();
};

template <class T> class List
{
	Element <T> * head;
	Element <T> * tail;
	Element <T> * pointer;
	uint listsize;
public:
	List();
	~List();

	void clear();
	void deform();

	uint getSize();

	void push(T *o);
	void push(Element<T> *e);

	Element<T> * popElem();
	T * popObj();
	Element<T> * lookFirstElem();
	T * lookFirstObj();

	bool removeElem(uint uid);
	bool removeObj(uint uid);

	Element<T> * pinchElem(uint uid);
	T * pinchObj(uint uid);

	Element<T> * lookElem(uint uid);
	T * lookObj(uint uid);

	T * startLoopElem();
	T * nextStepElem();

	T * startLoopObj();
	T * nextStepObj();
};

template<class T>
T * Element<T>::getObj()
{
	return obj;
}

template<class T>
void Element<T>::clear()
{
	if (!obj) delete obj;
}

template<class T>
Element<T>::Element()
{
	obj = NULL;
	next = NULL;
	prev = NULL;
}

template<class T>
Element<T>::Element(T * o, Element<T>* n, Element<T>* p)
{
	obj = o;
	next = n;
	prev = p;
}

template<class T>
Element<T>::~Element()
{
}

template<class T>
List<T>::List()
{
	head = tail = pointer = NULL;
}

template<class T>
List<T>::~List()
{
}

template<class T>
void List<T>::clear()
{
	Element<T> *curr = head;
	Element<T> *currNext;
	while (curr)
	{
		currNext = head->next;
		curr->clear();
		delete curr;
		curr = currNext;
	}
	listsize = 0;
	head = NULL;
	tail = NULL;
}

template<class T>
void List<T>::deform()
{
	Element<T> *curr = head;
	Element<T> *currNext;
	while (curr)
	{
		currNext = head->next;
		delete curr;
		curr = currNext;
	}
	listsize = 0;
	head = NULL;
	tail = NULL;
}

template<class T>
uint List<T>::getSize()
{
	return listsize;
}

template<class T>
void List<T>::push(T * o)
{
	if (!head) head = tail = new Element<T>(o, NULL, NULL);
	else
	{
		tail->next = new Element<T>(o, NULL, tail);
		tail = tail->next;
	}
	listsize++;
}

template<class T>
void List<T>::push(Element<T>* e)
{
	if (!head)
	{
		head = tail = e;
		e->next = e->prev = NULL;
	}
	else
	{
		tail->next = e;
		e->next = NULL;
		e->prev = tail;
		tail = e;
	}
	listsize++;
}

template<class T>
Element<T>* List<T>::popElem()
{
	if (!head) return NULL;
	Element<T> * tmp = head;
	head = head->next;
	listsize--;
	if (!head) tail = NULL;
	return tmp;
}

template<class T>
T * List<T>::popObj()
{
	if (!head) return NULL;
	Element<T> * tmp = head;
	T* obj = tmp->getObj();
	head = head->next;
	listsize--;
	if (!head) tail = NULL;
	delete tmp;
	return obj;
}

template<class T>
Element<T>* List<T>::lookFirstElem()
{
	return head;
}

template<class T>
T * List<T>::lookFirstObj()
{
	if (!head) return NULL;
	return head->getObj();
}

template<class T>
bool List<T>::removeElem(uint uid)
{
	Element<T> *curr = head;
	Element<T> *currNext;
	Element<T> *currPrev;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	if (!curr) return false;
	currPrev = curr->prev;
	currNext = curr->next;
	delete curr;
	if (currPrev) currPrev->next = currNext;
	if (currNext) currNext->prev = currPrev;
	listsize--;
	return true;
}

template<class T>
bool List<T>::removeObj(uint uid)
{
	Element<T> *curr = head;
	Element<T> *currNext;
	Element<T> *currPrev;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	if (!curr) return false;
	currPrev = curr->prev;
	currNext = curr->next;
	curr->clear();
	delete curr;
	if (currPrev) currPrev->next = currNext;
	if (currNext) currNext->prev = currPrev;
	listsize--;
	return true;
}

template<class T>
Element<T> * List<T>::pinchElem(uint uid)
{
	Element<T> *curr = head;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	if (!curr) return NULL;
	if (curr->prev) curr->prev->next = curr->next;
	if (curr->next) curr->next->prev = curr->prev;
	listsize--;
	return curr;
}

template<class T>
T * List<T>::pinchObj(uint uid)
{
	Element<T> *curr = head;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	if (!curr) return NULL;
	if (curr->prev) curr->prev->next = curr->next;
	if (curr->next) curr->next->prev = curr->prev;
	listsize--;
	T * obj = curr->getObj();
	delete curr;
	return obj;
}

template<class T>
Element<T>* List<T>::lookElem(uint uid)
{
	Element<T> *curr = head;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	return curr;
}

template<class T>
T * List<T>::lookObj(uint uid)
{
	Element<T> *curr = head;
	while (curr->getObj()->UID() != uid && curr)
		curr = curr->next;
	if (!curr) return NULL;
	return curr->getObj();
}

template<class T>
T * List<T>::startLoopElem()
{
	return pointer = head;
}

template<class T>
T * List<T>::nextStepElem()
{
	if (!pointer) return NULL;
	return pointer = pointer->next;
}

template<class T>
T * List<T>::startLoopObj()
{
	if (pointer = head) return pointer->getObj();
	return NULL;
}

template<class T>
T * List<T>::nextStepObj()
{
	if (!pointer) return NULL;
	if (pointer = pointer->next) return pointer->getObj();
	return NULL;
}