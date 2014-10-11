#ifndef DOUBLE_LINKED_LIST_H
#define DOUBLE_LINKED_LIST_H

#include <functional>

class DoubleLinkedListNode;

class DoubleLinkedList {
  public:
    DoubleLinkedList();
    ~DoubleLinkedList();

    //DoubleLinkedList takes ownership of node
    void push_back(DoubleLinkedListNode * node);
    void push_front(DoubleLinkedListNode * node);

    //returns true when found and removed, ownership ditched
    bool remove(DoubleLinkedListNode * node);

    void each(std::function<void(DoubleLinkedListNode * node)> func);
  private:
    DoubleLinkedListNode * mHead = nullptr;
};

class DoubleLinkedListNode {
   public:
      DoubleLinkedListNode();
      virtual ~DoubleLinkedListNode();
   private:
      friend class DoubleLinkedList;
      DoubleLinkedListNode * next = nullptr;
      DoubleLinkedListNode * prev = nullptr;
};

#endif
