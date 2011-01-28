#ifndef DOUBLE_LINKED_LIST_HPP
#define DOUBLE_LINKED_LIST_HPP

class DoubleLinkedListNode {
   public:
      DoubleLinkedListNode();
      virtual ~DoubleLinkedListNode();

      //is this the first node in the list?
      bool first();
      //is this the last node in the list?
      bool last();

      //return the next node in the list, can be NULL
      DoubleLinkedListNode * next();
      //return the previous node in the list, can be NULL
      DoubleLinkedListNode * prev();

      //returns true if the newly inserted node is now the head of the list
      //returns false otherwise
      bool insert_before(DoubleLinkedListNode * node);

      //returns true if the newly inserted node is now the tail of the list
      //returns false otherwise
      bool insert_after(DoubleLinkedListNode * node);

      //returns the previous node.. unless it is the first node, then it returns the next node
      DoubleLinkedListNode * remove();
   private:
      DoubleLinkedListNode * mNext;
      DoubleLinkedListNode * mPrev;
};

#endif
