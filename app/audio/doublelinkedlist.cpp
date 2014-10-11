#include "doublelinkedlist.h"
#include <stdlib.h>

DoubleLinkedListNode::DoubleLinkedListNode() { }
DoubleLinkedListNode::~DoubleLinkedListNode() { }

DoubleLinkedList::DoubleLinkedList() {
}

DoubleLinkedList::~DoubleLinkedList() {
}

void DoubleLinkedList::push_back(DoubleLinkedListNode * node) {
  node->next = nullptr;
  node->prev = nullptr;
  if (!mHead) {
    mHead = node;
    return;
  }

  //find the last element and append node
  DoubleLinkedListNode * cur = mHead;
  while (cur->next)
    cur = cur->next;
  node->prev = cur;
  cur->next = node;
}

void DoubleLinkedList::push_front(DoubleLinkedListNode * node) {
  node->next = mHead;
  node->prev = nullptr;
  if (mHead)
    mHead->prev = node;
  mHead = node;
}

bool DoubleLinkedList::remove(DoubleLinkedListNode * node) {
  DoubleLinkedListNode * cur = mHead;
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

void DoubleLinkedList::each(std::function<void(DoubleLinkedListNode * node)> func) {
  DoubleLinkedListNode * cur = mHead;
  while (cur) {
    func(cur);
    cur = cur->next;
  }
}

