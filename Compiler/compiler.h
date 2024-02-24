#pragma once
#include <map>
#include <string>
#include <vector>

class compiler
{
public:
    compiler(char* text, int textLen);
    ~compiler();
    
    bool compile(std::string& out);

private:
    enum class tokensTypes
    {
        NUM,
        VAR,
        IF,
        ELSE,
        WHILE,
        LBRA,
        RBRA,
        LPAR,
        RPAR,
        ADD,
        SUB,
        MUL,
        DIV,
        LESS,
        ASSIG,
        SEMICOL,
        ENDFILE,
        ERROR,
        PRINT
    };

    enum class nodeTypes
    {
        CONST,
        VAR,
        ADD,
        SUB,
        MUL,
        DIV,
        LESSTHEN,
        SET,
        IF,
        IFELSE,
        WHILE,
        EMPTY,
        SEQ,
        EXPR,
        PROG,
        PRINT
    };

    class token
    {
    public:
        tokensTypes type;
        int value;
        char variable;

        token(tokensTypes type, int value = 0, char variable = 'a');
    };

    class node
    {
    public:
        nodeTypes type;
        int value;
        node* op1;
        node* op2;
        node* op3;

        node(nodeTypes type, int value = 0, node* op1 = nullptr, node* op2 = nullptr, node* op3 = nullptr);
        ~node();
    };

    static class parser
    {
    public:
        static bool parse(std::vector<token>& tokens, node& result);

    private:
        static bool statement(std::vector<token>& tokens, int& count, node*& node);
        static bool parenExpr(std::vector<token>& tokens, int& count, node*& node);
        static bool expr(std::vector<token>& tokens, int& count, node*& node);
        static bool test(std::vector<token>& tokens, int& count, node*& node);
        static bool arExpr(std::vector<token>& tokens, int& count, node*& node);
        static bool term(std::vector<token>& tokens, int& count, node*& node);
    };

    class linker
    {
    private:
        static const std::string programsStart;
        static const std::string programsEnd;

        node* tree;

        std::string compileNode(node* node, int& labelCount);

    public:
        linker(node* tree);
        std::string compile();
    };

    char* text;
    int textLen;
    int nowPos = 0;
    static std::map<char, tokensTypes> specialSymbols;
    static std::map<std::string, tokensTypes> specialWords;

    std::vector<token> tokens;

    token getNexttoken();
};