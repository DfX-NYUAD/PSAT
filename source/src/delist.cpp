#include "delist.h"
#include <assert.h>
#include <algorithm>
#include <iostream>

delist::delist(int size)
{
    reset(size);
}

delist::delist()
{
}

delist::~delist()
{
}

void delist::reset(int sz)
{
    if(sz == -1) {
        sz = size();
    }
    assert(sz >= 0);

    deleted.resize(sz);
    next_index.resize(sz+1);

    std::fill(deleted.begin(), deleted.end(), false);
    for(int i=0; i != sz; i++) {
        next_index[i] = i;
    }
    next_index[sz] = -1;
}

void delist::del(int index)
{
    deleted[index] = true;
}

delist::iterator::iterator(delist& lst) 
    : dl(lst) 
    , pos(dl.next_index[0])
{
    while(pos != -1 && dl.deleted[pos]) {
        pos = dl.next_index[pos+1];
    }
    dl.next_index[0] = pos;
}

void delist::iterator::next()
{
    if(pos == -1) return;
    int orig_pos = pos;
    pos = dl.next_index[pos+1];
    while(pos != -1 && dl.deleted[pos]) {
        pos = dl.next_index[pos+1];
    }
    dl.next_index[orig_pos+1] = pos;
}

void delist::dumb_iterator::incr_()
{
    if(pos == -1) return;
    while(pos < (int) dl.deleted.size() && dl.deleted[pos]) {
        pos++;
    }
    if(pos == (int) dl.deleted.size()) {
        pos = -1;
    }
}

delist::iterator delist::start() const
{
    return iterator((delist&) *this);
}

delist::dumb_iterator delist::start_dumb() const
{
    return dumb_iterator((delist&) *this);
}

void test_delist(void)
{
    delist dl(5);
    for(delist::iterator it = dl.start(); it.more(); it.next()) {
        std::cout << it.index() << " ";
    }
    std::cout << std::endl;

    dl.del(1); dl.del(3);
    for(delist::iterator it = dl.start(); it.more(); it.next()) {
        std::cout << it.index() << " ";
    }
    std::cout << std::endl;

    dl.reset();
    for(delist::iterator it = dl.start(); it.more(); it.next()) {
        std::cout << it.index() << " ";
        dl.del(it.index());
    }
    std::cout << std::endl;

    for(delist::iterator it = dl.start(); it.more(); it.next()) {
        std::cout << it.index() << " ";
        dl.del(it.index());
    }
    std::cout << std::endl;
}

