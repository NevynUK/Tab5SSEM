#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>

#include "Register.hpp"

using namespace std;

/**
 * @brief Object that will hold the contents of the store lines.
 */
class StoreLines
{
private:
    /**
     * @brief Vector of store lines.
     */
    vector<Register> _lines;

public:
    explicit StoreLines(uint size = 32);
    ~StoreLines();
    Register &operator[](uint index);

    /**
     * @brief Get the number of the store lines.
     *
     * @return uint Number of store lines.
     */
    uint Size() const
    {
        return (_lines.size());
    }

    void Clear();
};
