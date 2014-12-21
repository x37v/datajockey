#ifndef DOUBLE_LINKED_LIST_H
#define DOUBLE_LINKED_LIST_H

#include <functional>

template <typename T>
class DoubleLinkedListNode;

template <typename T>
class DoubleLinkedList {
  public:
    DoubleLinkedList();
    ~DoubleLinkedList();

    //DoubleLinkedList takes ownership of node
    void push_back(DoubleLinkedListNode<T> * node);
    void push_front(DoubleLinkedListNode<T> * node);

    void insert(unsigned int index, DoubleLinkedListNode<T> * node);

    //returns true when found and removed, ownership ditched
    bool remove(DoubleLinkedListNode<T> * node);

    void each(std::function<void(T data)> func);
  private:
    DoubleLinkedListNode<T> * mHead = nullptr;
};

template <typename T>
class DoubleLinkedListNode {
  public:
    DoubleLinkedListNode() {}
    DoubleLinkedListNode(T v) { mData = v; }
    T data() const { return mData; }
    void data(T v) { mData = v; }
  private:
    T mData;
    friend class DoubleLinkedList<T>;
    DoubleLinkedListNode<T> * next = nullptr;
    DoubleLinkedListNode<T> * prev = nullptr;
};

template <typename T>
DoubleLinkedList<T>::DoubleLinkedList() {
}

template <typename T>
DoubleLinkedList<T>::~DoubleLinkedList() {
  DoubleLinkedListNode<T> * cur = mHead;
  while (cur) {
    mHead = cur;
    cur = cur->next;
    delete mHead;
  }
}


template <typename T>
void DoubleLinkedList<T>::push_back(DoubleLinkedListNode<T> * node) {
  node->next = nullptr;
  node->prev = nullptr;
  if (!mHead) {
    mHead = node;
    return;
  }

  //find the last element and append node
  DoubleLinkedListNode<T> * cur = mHead;
  while (cur->next)
    cur = cur->next;
  node->prev = cur;
  cur->next = node;
}

template <typename T>
void DoubleLinkedList<T>::push_front(DoubleLinkedListNode<T> * node) {
  node->next = mHead;
  node->prev = nullptr;
  if (mHead)
    mHead->prev = node;
  mHead = node;
}

template <typename T>
void DoubleLinkedList<T>::insert(unsigned int index, DoubleLinkedListNode<T> * node) {
  if (!mHead) {
    push_front(node);
    return;
  }
  DoubleLinkedListNode<T> * cur = mHead;
  while (index > 0 && cur->next) {
    index--;
    cur = cur->next;
  }

  node->next = cur;
  node->prev = cur->prev;
  cur->prev = node;
  if (node->prev)
    node->prev->next = node;
}

template <typename T>
bool DoubleLinkedList<T>::remove(DoubleLinkedListNode<T> * node) {
  DoubleLinkedListNode<T> * cur = mHead;
  //find the node
  while (cur && cur != node)
    cur = cur->next;
  if (!cur) //not found
    return false;

  //fix up the list
  if (cur->next)
    cur->next->prev = cur->prev;
  if (cur->prev)
    cur->prev->next = cur->next;
  if (cur == mHead)
    mHead = cur->next;
  cur->next = nullptr;
  cur->prev = nullptr;
  return true;
}

template <typename T>
void DoubleLinkedList<T>::each(std::function<void(T)> func) {
  DoubleLinkedListNode<T> * cur = mHead;
  while (cur) {
    func(cur->data());
    cur = cur->next;
  }
}

#endif
