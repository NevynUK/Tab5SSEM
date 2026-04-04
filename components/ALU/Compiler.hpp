#pragma once

#include "StoreLines.hpp"
#include <vector>

/**
 * @brief Implement a simple compiler for the SSEM.
 */
class Compiler
{
public:
    Compiler() = delete;
    ~Compiler() = delete;

    static StoreLines *Compile(const char *);
    static StoreLines *Compile(const vector<const char *> &);
    static bool IsComment(const char *);
    static bool IsNumber(const char *);
    static bool IsBlank(const char *);
    static bool IsBinary(const char *);

private:
    /**
     * @brief Structure to hold the tokenised line.
     */
    struct TokenisedLine
    {
        /**
         * @brief Store line number that this line will be stored in.
         */
        uint32_t storeLineNumber;

        /**
         * @brief Opcode for this line.
         */
        uint32_t opcode;

        /**
         * @brief Operand to be used by the opcode.
         */
        uint32_t operand;
    };

    static vector<TokenisedLine *> *Tokenise(const vector<const char *> &);
    static uint32_t GetStoreLineNumber(const char *);
    static int32_t GetOperand(const char *);
    static uint32_t GetBinary(const char *);
    static void CleanUp(vector<Compiler::TokenisedLine *> *tokens);
};
