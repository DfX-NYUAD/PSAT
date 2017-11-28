#ifndef _DELIST_H_DEFINED_
#define _DELIST_H_DEFINED_

// A "deletable" list.

#include <vector>

class delist  {
private:
    std::vector<bool> deleted;
    std::vector<int> next_index;
public:
    // types.
    class iterator {
        delist& dl;
        int pos;

        iterator(delist& lst); 
    public:
        // get the current index.
        int index() const { return pos; }
        // move to the next index.
        void next();
        // are we at the end?
        bool more() const { return pos != -1; }

        friend class delist;
    };

    class dumb_iterator {
        delist& dl;
        int pos;
        
        dumb_iterator(delist& lst)
          : dl(lst)
          , pos(0)
        {
            incr_();
        }
        void incr_();
    public:
        int index() const { return pos; }
        void next() { pos++; incr_(); }
        bool more() const { return pos != -1; }

        friend class delist;
    };

    // construction and desctruction.
    delist(int size);
    delist();
    ~delist();

    // operations on the list.
    unsigned size() const { return deleted.size(); }
    void del(int index);
    void reset(int size = -1);
    iterator start() const;
    dumb_iterator start_dumb() const;

    friend class delist::iterator;
    friend class delist::dumb_iterator;
};

void test_delist(void);
#endif
