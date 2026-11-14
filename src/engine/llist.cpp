// llist.cpp -- engine.lib intrusive lists (Will Rock 1.2, v12.exe; feb obj engine:llist.obj)
// lstLIST_SIMPLE (the list the collision grids use), lstLIST and lstPLIST (the message system's lists).
//
// lstNODE is { prev, next }; lstLIST_SIMPLE keeps only the head (no tail, no count); lstLIST adds both;
// lstPLIST keeps its nodes sorted by the virtual Compare.

#include "engine.h"

// v12 0x004FE060  (feb 0x004EBA80)
// Insert n in front of 'before' (the head when before = 0).
void lstLIST_SIMPLE::Insert(lstNODE *n, lstNODE *before)
{
    n->prev = 0;
    n->next = 0;
    if (!head) {
        head = n;
        return;
    }
    if (!before)
        before = head;
    n->next = before;
    n->prev = before->prev;
    before->prev = n;
    if (!n->prev)
        head = n;
    else
        n->prev->next = n;
}

// v12 0x004AC310  (emitted in another obj; inline in the header)
void lstLIST_SIMPLE::Add(lstNODE *n)
{
    Insert(n, 0);
}

// v12 0x004FE0B0  (feb 0x004EBAD0)
void lstLIST_SIMPLE::Remove(lstNODE *n)
{
    lstNODE *p = n->prev, *x = n->next;
    if (!p) {
        head = x;
        if (x)
            x->prev = 0;
    } else {
        p->next = x;
        if (x)
            x->prev = p;
    }
    n->prev = 0;
    n->next = 0;
}

// v12 0x004FE0F0  (feb 0x004EBB10)
void lstLIST_SIMPLE::Clear()
{
    while (head)
        Remove(head);
}

// v12 0x004FE560  (feb 0x004EBF80)  vector deleting destructor (emitted in cdt_sys_anim.obj)
void *lstLIST_SIMPLE::__vecDelDtor(unsigned flags)
{
    return wrVecDelDtor(this, flags);
}

lstNODE *lstPListSortBuf[10000];  // v12 0x00978F58 (size not known; no bound check, original) // ?

// v12 0x004FE110  (feb 0x004EBB30)
void lstLIST::Insert(lstNODE *n, lstNODE *before)
{
    if (!head)
        tail = n;
    n->prev = 0;
    n->next = 0;
    if (!head) {
        head = n;
        count++;
        return;
    }
    if (!before)
        before = head;
    n->next = before;
    n->prev = before->prev;
    before->prev = n;
    if (!n->prev)
        head = n;
    else
        n->prev->next = n;
    count++;
}

// v12 0x004FE180  (feb 0x004EBBA0)
void lstLIST::Remove(lstNODE *n)
{
    if (tail == n)
        tail = n->prev;
    lstNODE *p = n->prev, *x = n->next;
    if (!p) {
        head = x;
        if (x)
            x->prev = p;
    } else {
        p->next = x;
        if (x)
            x->prev = p;
    }
    n->prev = 0;
    n->next = 0;
    count--;
}

// v12 0x004AC320  (feb 0x0049DF30)
int lstLIST::GetNumItems()
{
    return count;
}

// v12 0x004EAD50  (the folded deleting destructor of every lstLIST kind: back to lstLIST_SIMPLE, no Clear)
void *lstLIST::__vecDelDtor(unsigned flags)
{
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x004036D0  (folded "return 0")
int lstPLIST::Compare(lstNODE *, lstNODE *)
{
    return 0;
}

// v12 0x004FE1D0  (feb 0x004EBBF0)
// Insert n in front of the first node it sorts before (Compare(n, node) == 1), or at the tail.
void lstPLIST::Insert(lstNODE *n)
{
    if (!tail) {
        head = n;
        tail = n;
        n->prev = 0;
        n->next = 0;
        count++;
        return;
    }
    lstNODE *g = FindFirstGreater_Fast(n);
    if (g) {
        n->next = g;
        n->prev = g->prev;
        g->prev = n;
        if (g == head)
            head = n;
        else
            n->prev->next = n;
        count++;
        return;
    }
    tail->next = n;
    n->prev = tail;
    n->next = 0;
    tail = n;
    count++;
}

// v12 0x004FE260  (feb 0x004EBC80)
// Copies the nodes into a static array, then a binary search with Compare: the first node n sorts before,
// 0 when it sorts after the last one.
lstNODE *lstPLIST::FindFirstGreater_Fast(lstNODE *n)
{
    if (!head)
        return 0;
    int hi = 0;
    for (lstNODE *x = head; x; x = x->next)
        lstPListSortBuf[hi++] = x;
    hi--;
    int lo = 0;
    int first = Compare(n, lstPListSortBuf[0]);
    if (!Compare(n, lstPListSortBuf[hi]))
        return 0;
    if (first == 1)
        return lstPListSortBuf[0];
    while (hi != lo) {
        int mid = lo + (hi - lo) / 2;
        if (Compare(n, lstPListSortBuf[mid]) == 1) {
            hi--;
            if (mid < hi)
                hi = mid;
        } else {
            lo++;
            if (mid > lo)
                lo = mid;
        }
    }
    return lstPListSortBuf[hi];
}
