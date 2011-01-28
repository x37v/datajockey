#include "doublelinkedlist.hpp"
#include <stdlib.h>

DoubleLinkedListNode::DoubleLinkedListNode() {
   mPrev = mNext = NULL;
}

DoubleLinkedListNode::~DoubleLinkedListNode() {
   //we don't remove ourselves from the list because we might just be a copy of
   //the node
}

bool DoubleLinkedListNode::first() {
   return mPrev == NULL;
}

bool DoubleLinkedListNode::last() {
   return mNext == NULL;
}

DoubleLinkedListNode * DoubleLinkedListNode::next() {
   return mNext;
}

DoubleLinkedListNode * DoubleLinkedListNode::prev() {
   return mPrev;
}

bool DoubleLinkedListNode::insert_before(DoubleLinkedListNode * node) {

   //if we have a node behind us, just insert the node after that
   if (mPrev) {
      mPrev->insert_after(node);
      return false;
   }

   //otherwise set the node's pointers and our prev pointer
   node->mNext = this;
   node->mPrev = NULL;
   mPrev = node;

   return true;
}

bool DoubleLinkedListNode::insert_after(DoubleLinkedListNode * node) {
   bool tail = last();

   //set the nodes pointers
   node->mNext = mNext;
   node->mPrev = this;
   //if we have a next node, set its prev pointer
   if (mNext)
      mNext->mPrev = node;
   //set our new next pointer
   mNext = node;

   return tail;
}

DoubleLinkedListNode * DoubleLinkedListNode::remove() {
   DoubleLinkedListNode * ret = mPrev;

   //if there is a next node set its prev node to our prev node
   if (mNext)
      mNext->mPrev = mPrev;

   //if there is a prev node, set its next node to our next node
   //otherwise, set our return value to our next node
   if (mPrev)
      mPrev->mNext = mNext;
   else
      ret = mNext;

   //clear out our pointers
   mPrev = NULL;
   mNext = NULL;
   return ret;
}

