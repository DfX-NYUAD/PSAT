#ifndef _STATS_H_DEFINED_
#define _STATS_H_DEFINED_

#include <map>
#include <iostream>
#include <iomanip>

namespace stack_n {
    template<class T>
    class Hist
    {
        typedef std::map<T, int> map_t;
        map_t counts;
    public:
        void incr(T ev) { counts[ev] += 1; }
        double avg() {
            double sum = 0.0;
            double cnt = 0;
            for(typename map_t::iterator it = counts.begin(); it != counts.end(); it++) {
                sum += it->first;
                cnt += 1;
            }
            return sum / cnt;
        }

        std::ostream& dump(std::ostream& out) {
            for(typename map_t::iterator it = counts.begin(); it != counts.end(); it++) {
                out << std::setw(10) << it->first << "->" << std::setw(10) << it->second << std::endl;
            }
            return out;
        }
    };
}

#endif
