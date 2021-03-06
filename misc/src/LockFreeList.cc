#include "LockFreeList.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>


/*
 * all the nodes will either be linked in the lock-free list(using next) or
 * linked in the unused-list (using next too)
 */
struct LockFreeListQueue::LockFreeListNode
{
    DoublePointer next;

    void* volatile data;

    static void PrintList(LockFreeListQueue::LockFreeListNode*);
};

void LockFreeListQueue::LockFreeListNode::PrintList(LockFreeListQueue::LockFreeListNode* head)
{
    using namespace std;

    int co = 0;

    while (head)
    {
        cout << "addr:" << hex << head << ", data:" << hex << head->data
             << ", id:" << hex << head->next.vals[0] << endl;

        head = (LockFreeListNode*)(head->next.vals[1]);
        ++co;
    }

    cout << "total:" << co << endl;
}

LockFreeListQueue::LockFreeListQueue(size_t capacity)
    :id_(0)
    ,max_(capacity)
    ,id2_(0)
    ,freeList_(NULL)
{
    InitInternalNodeList();
    LockFreeListNode* tmp = AllocNode();
    assert(tmp);
    SetDoublePointer(in_, (void*)id_, tmp);
    id_ = 1;
    out_ = in_;
}

LockFreeListQueue::~LockFreeListQueue()
{
    delete[] freeList_;
}

void LockFreeListQueue::InitInternalNodeList()
{
    assert(!freeList_);

    freeList_ = new LockFreeListNode[max_];

    size_t i = 0;
    for (; i < max_ - 1; ++i)
    {
        freeList_[i].next.vals[0] = (void*)(i + 1);
        freeList_[i].next.vals[1] = &freeList_[i + 1];
    }

    freeList_[i].next.val = 0;

    SetDoublePointer(head_, (void*)0, freeList_);
    id2_ = i;
}

LockFreeListQueue::LockFreeListNode* LockFreeListQueue::AllocNode()
{
    DoublePointer old_head;

    do
    {
        old_head.val = atomic_read_double(&head_);
        if (IsDoublePointerNull(old_head)) return NULL;

        // freeList_ will never be freed untill queue is destroyed.
        // so it is safe to access old_head->next
        if (atomic_cas2(&head_.val, old_head.val, ((LockFreeListNode*)old_head.vals[1])->next.val)) break;

    } while (1);

    LockFreeListNode* ret = (LockFreeListNode*)old_head.vals[1];

    // must initialize node manually, we don't have constructor to call.
    // maybe I should have provided a placement constructor for it.
    ret->next.val = 0;

    return ret;
}

void LockFreeListQueue::ReleaseNode(LockFreeListNode* node)
{
    assert(node >= freeList_ && node < freeList_ + max_);

    DoublePointer old_head;
    DoublePointer new_node;

    size_t id = atomic_increment(&id2_);
    SetDoublePointer(new_node, (void*)id, node);

    do
    {
        old_head.val = atomic_read_double(&head_);
        node->next = old_head;

        if (atomic_cas2(&head_.val, old_head.val, new_node.val)) break;

    } while (1);
}

bool LockFreeListQueue::Push(void* data)
{
    LockFreeListNode* node = AllocNode();

    if (node == NULL) return false;

    node->data = data;
    node->next.val = 0;

    DoublePointer in, next;
    DoublePointer new_node;

    size_t id = atomic_increment(&id_);
    SetDoublePointer(new_node, (void*)id, node);

    while (1)
    {
        // just a reminder:
        // DoublePointer1 = DoublePointer2;
        // is not an atomic operation.

        in.val = atomic_read_double(&in_);
        LockFreeListNode* node_tail = (LockFreeListNode*)atomic_read(&in.vals[1]);
        next.val = atomic_read_double(&node_tail->next);

        if (!IsDoublePointerNull(next))
        {
            atomic_cas2(&(in_.val), in.val, next.val);
            continue;
        }

        if (atomic_cas2((&(((LockFreeListNode*)(in.vals[1]))->next.val)), 0, new_node.val)) break;
    }

    // this line may fail, but doesn't matter, line 149 will fix it up.
    atomic_cas2((&in_.val), in.val, new_node.val);

    return true;
}

bool LockFreeListQueue::Pop(void*& data)
{
    DoublePointer out, in, next;

    while (1)
    {
        // out points to the first node, this a dummy node.
        out.val  = atomic_read_double(&out_);
        in.val   = atomic_read_double(&in_);

        // read the dummy node
        LockFreeListNode* node_head = (LockFreeListNode*)atomic_read(&out.vals[1]);

        // get next node, which is a real node if exists
        next.val = atomic_read_double(&node_head->next);

        if (IsDoublePointerNull(next)) return false;

        if (IsDoublePointerEqual(in, out))
        {
            atomic_cas2(&(in_.val), in.val, next.val);
            continue;
        }

        // LockFreeListNode will  never be freed until queue is destroyed.
        // so it is safe to access data, even when that node is released.
        data = ((LockFreeListNode*)(next.vals[1]))->data;

        if (atomic_cas2(&(out_.val), out.val, next.val)) break;
    }

    LockFreeListNode* cur = (LockFreeListNode*)(out.vals[1]);
    ReleaseNode(cur);

    return true;
}

bool LockFreeListQueue::Pop(void** data)
{
    if (data == NULL) return false;

    return Pop(*data);
}

