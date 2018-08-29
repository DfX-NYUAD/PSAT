/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef CSET_H
#define CSET_H

#include <limits>
#include "cmsat/Vec.h"
#include "cmsat/constants.h"

namespace CMSat {

class Clause;

/**
@brief A class to hold a clause and a related index

This class is used in Subsumer. Basically, the index could be added to the
Clause class, but it would take space, and that would slow down the solving.

NOTE: On 64-bit systems, this datastructure needs 128 bits :O
*/
class ClauseSimp
{
    public:
        ClauseSimp(Clause* c, const uint32_t _index) :
        clause(c)
        , index(_index)
        {}

        Clause* clause; ///<The clause to be stored
        uint32_t index; ///<The index of the clause in Subsumer::clauses
};

/**
@brief Used to quicky add, remove and iterate through a clause set

Used in Subsumer to put into a set all clauses that need to be treated
*/
class CSet {
    vec<uint32_t>       where;  ///<Map clause ID to position in 'which'.
    vec<ClauseSimp>     which;  ///< List of clauses (for fast iteration). May contain 'Clause_NULL'.
    vec<uint32_t>       free;   ///<List of positions holding 'Clause_NULL'.

    public:
        //ClauseSimp& operator [] (uint32_t index) { return which[index]; }
        void reserve(uint32_t size) { where.reserve(size);}
        //uint32_t size(void) const { return which.size(); }
        ///@brief Number of elements in the set
        uint32_t nElems(void) const { return which.size() - free.size(); }

        /**
        @brief Add a clause to the set
        */
        bool add(const ClauseSimp& c) {
            assert(c.clause != NULL);
            where.growTo(c.index+1, std::numeric_limits<uint32_t>::max());
            if (where[c.index] != std::numeric_limits<uint32_t>::max()) {
                return false;
            }
            if (free.size() > 0){
                where[c.index] = free.last();
                which[free.last()] = c;
                free.pop();
            }else{
                where[c.index] = which.size();
                which.push(c);
            }
            return true;
        }

        bool alreadyIn(const ClauseSimp& c) const {
            assert(c.clause != NULL);
            if (where.size() < c.index+1) return false;
            if (where[c.index] != std::numeric_limits<uint32_t>::max())
                return true;
            return false;
        }

        /**
        @brief Remove clause from set

        Handles it correctly if the clause was not in the set anyway
        */
        bool exclude(const ClauseSimp& c) {
            assert(c.clause != NULL);
            if (c.index >= where.size() || where[c.index] == std::numeric_limits<uint32_t>::max()) {
                //not inside
                return false;
            }
            free.push(where[c.index]);
            which[where[c.index]].clause = NULL;
            where[c.index] = std::numeric_limits<uint32_t>::max();
            return true;
        }

        /**
        @brief Fully clear the set
        */
        void clear(void) {
            for (uint32_t i = 0; i < which.size(); i++)  {
                if (which[i].clause != NULL) {
                    where[which[i].index] = std::numeric_limits<uint32_t>::max();
                }
            }
            which.clear();
            free.clear();
        }

        /**
        @brief A normal iterator to iterate through the set

        No other way exists of iterating correctly.
        */
        class iterator
        {
            public:
                iterator(ClauseSimp* _it) :
                it(_it)
                {}

                void operator++()
                {
                    it++;
                }

                bool operator!=(const iterator& iter) const
                {
                    return (it != iter.it);;
                }

                ClauseSimp& operator*() {
                    return *it;
                }

                ClauseSimp*& operator->() {
                    return it;
                }
            private:
                ClauseSimp* it;
        };

        /**
        @brief A constant iterator to iterate through the set

        No other way exists of iterating correctly.
        */
        class const_iterator
        {
            public:
                const_iterator(const ClauseSimp* _it) :
                it(_it)
                {}

                void operator++()
                {
                    it++;
                }

                bool operator!=(const const_iterator& iter) const
                {
                    return (it != iter.it);;
                }

                const ClauseSimp& operator*() {
                    return *it;
                }

                const ClauseSimp*& operator->() {
                    return it;
                }
            private:
                const ClauseSimp* it;
        };

        ///@brief Get starting iterator
        iterator begin()
        {
            return iterator(which.getData());
        }

        ///@brief Get ending iterator
        iterator end()
        {
            return iterator(which.getData() + which.size());
        }

        ///@brief Get starting iterator (constant version)
        const_iterator begin() const
        {
            return const_iterator(which.getData());
        }

        ///@brief Get ending iterator (constant version)
        const_iterator end() const
        {
            return const_iterator(which.getData() + which.size());
        }
};

}

#endif //CSET_H

