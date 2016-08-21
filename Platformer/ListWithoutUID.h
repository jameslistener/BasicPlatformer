#pragma once

#pragma once
#define NULL 0

template <class T> class ElementWoUID
{
private:
	T * obj;

public:
	ElementWoUID<T> * next;
	ElementWoUID<T> * prev;

	ElementWoUID();
	ElementWoUID(T * o, ElementWoUID<T> *n = NULL, ElementWoUID<T> *p = NULL);
	~ElementWoUID();

	T *getObj();
	void clear();
};

template <class T> class ListWithoutUID
{
	ElementWoUID <T> * head;
	ElementWoUID <T> * tail;
	ElementWoUID <T> * pointer;
	int listsize;
public:
	ListWithoutUID();
	~ListWithoutUID();

	void clear();
	void deform();

	int getSize();

	void push(T *o);
	void push(ElementWoUID<T> *e);

	ElementWoUID<T> * popElem();
	T * popObj();
	ElementWoUID<T> * lookFirstElem();
	T * lookFirstObj();

	T * startLoopElem();
	T * nextStepElem();

	T * startLoopObj();
	T * nextStepObj();
};

template<class T>
T * ElementWoUID<T>::getObj()
{
	return obj;
}

template<class T>
void ElementWoUID<T>::clear()
{
	if (!obj) delete obj;
}

template<class T>
ElementWoUID<T>::ElementWoUID()
{
	obj = NULL;
	next = NULL;
	prev = NULL;
}

template<class T>
ElementWoUID<T>::ElementWoUID(T * o, ElementWoUID<T>* n, ElementWoUID<T>* p)
{
	obj = o;
	next = n;
	prev = p;
}

template<class T>
ElementWoUID<T>::~ElementWoUID()
{
}

template<class T>
ListWithoutUID<T>::ListWithoutUID()
{
	head = tail = pointer = NULL;
}

template<class T>
ListWithoutUID<T>::~ListWithoutUID()
{
}

template<class T>
void ListWithoutUID<T>::clear()
{
	ElementWoUID<T> *curr = head;
	ElementWoUID<T> *currNext;
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
void ListWithoutUID<T>::deform()
{
	ElementWoUID<T> *curr = head;
	ElementWoUID<T> *currNext;
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
int ListWithoutUID<T>::getSize()
{
	return listsize;
}

template<class T>
void ListWithoutUID<T>::push(T * o)
{
	if (!head) head = tail = new ElementWoUID<T>(o, NULL, NULL);
	else
	{
		tail->next = new ElementWoUID<T>(o, NULL, tail);
		tail = tail->next;
	}
	listsize++;
}

template<class T>
void ListWithoutUID<T>::push(ElementWoUID<T>* e)
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
ElementWoUID<T>* ListWithoutUID<T>::popElem()
{
	if (!head) return NULL;
	ElementWoUID<T> * tmp = head;
	head = head->next;
	listsize--;
	if (!head) tail = NULL;
	return tmp;
}

template<class T>
T * ListWithoutUID<T>::popObj()
{
	if (!head) return NULL;
	ElementWoUID<T> * tmp = head;
	T* obj = tmp->getObj();
	head = head->next;
	listsize--;
	if (!head) tail = NULL;
	delete tmp;
	return obj;
}

template<class T>
ElementWoUID<T>* ListWithoutUID<T>::lookFirstElem()
{
	return head;
}

template<class T>
T * ListWithoutUID<T>::lookFirstObj()
{
	if (!head) return NULL;
	return head->getObj();
}

template<class T>
T * ListWithoutUID<T>::startLoopElem()
{
	return pointer = head;
}

template<class T>
T * ListWithoutUID<T>::nextStepElem()
{
	if (!pointer) return NULL;
	return pointer = pointer->next;
}

template<class T>
T * ListWithoutUID<T>::startLoopObj()
{
	if (pointer = head) return pointer->getObj();
	return NULL;
}

template<class T>
T * ListWithoutUID<T>::nextStepObj()
{
	if (!pointer) return NULL;
	if (pointer = pointer->next) return pointer->getObj();
	return NULL;
}