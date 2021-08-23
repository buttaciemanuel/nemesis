#ifndef STRINGS_HPP
#define STRINGS_HPP

#include <algorithm>
#include <string>

namespace utils {
    unsigned levenshtein_distance(const std::string& x, const std::string& y)
    {
        unsigned** d = new unsigned*[x.size() + 1];

        for (unsigned i = 0; i <= x.size(); ++i) d[i] = new unsigned[y.size() + 1];

        for (unsigned i = 0; i <= x.size(); ++i) d[i][0] = i;

        for (unsigned j = 0; j <= y.size(); ++j) d[0][j] = j;

        for (unsigned i = 1; i <= x.size(); ++i) {
            for (unsigned j = 1; j <= y.size(); ++j) {
                if (x[i - 1] == y[j - 1]) {
                    d[i][j] = d[i - 1][j - 1];
                }
                else {
                    d[i][j] = std::min({d[i - 1][j], d[i][j - 1], d[i - 1][j - 1]}) + 1;
                }
            }
        }

        unsigned distance = d[x.size()][y.size()];

        for (unsigned i = 0; i <= x.size(); ++i) delete[] d[i];

        delete[] d;

        return distance;
    }
}

#endif // STRINGS_HPP