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

    /**
     * @brief Get the iterator for the beginning of the store lines.
     */
    auto begin() { return _lines.begin(); }

    /**
     * @brief Get the iterator for the end of the store lines.
     */
    auto end() { return _lines.end(); }

    /**
     * @brief Get the const iterator for the beginning of the store lines.
     */
    auto begin() const { return _lines.begin(); }

    /**
     * @brief Get the const iterator for the end of the store lines.
     */
    auto end() const { return _lines.end(); }
};
