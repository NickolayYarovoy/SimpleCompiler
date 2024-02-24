#include "compiler.h"

std::map<char, compiler::tokensTypes> compiler::specialSymbols
{
	std::pair<char, tokensTypes>{'{', tokensTypes::LBRA},
	std::pair<char, tokensTypes>{'}', tokensTypes::RBRA},
	std::pair<char, tokensTypes>{'(', tokensTypes::LPAR},
	std::pair<char, tokensTypes>{')', tokensTypes::RPAR},
	std::pair<char, tokensTypes>{'+', tokensTypes::ADD},
	std::pair<char, tokensTypes>{'-', tokensTypes::SUB},
	std::pair<char, tokensTypes>{'*', tokensTypes::MUL},
	std::pair<char, tokensTypes>{'/', tokensTypes::DIV},
	std::pair<char, tokensTypes>{'<', tokensTypes::LESS},
	std::pair<char, tokensTypes>{'=', tokensTypes::ASSIG},
	std::pair<char, tokensTypes>{';', tokensTypes::SEMICOL},
};

std::map<std::string, compiler::tokensTypes> compiler::specialWords
{
	std::pair<std::string, tokensTypes>{"if", tokensTypes::IF},
	std::pair<std::string, tokensTypes>{"else", tokensTypes::ELSE},
	std::pair<std::string, tokensTypes>{"while", tokensTypes::WHILE},
	std::pair<std::string, tokensTypes>{"print", tokensTypes::PRINT},
};

compiler::compiler(char* text, int textLen)
{
	this->textLen = textLen;
	this->text = new char[textLen];
	memcpy(this->text, text, textLen);
}

compiler::~compiler()
{
	delete[] text;
}

bool compiler::compile(std::string& result)
{
	token token = getNexttoken();

	while (token.type != tokensTypes::ENDFILE)
	{
		if (token.type == tokensTypes::ERROR)
		{
			result = "Lexical error";
			return false;
		}
		tokens.push_back(token);
		token = getNexttoken();
	}
	tokens.push_back(token);

	compiler::node tree = compiler::node(nodeTypes::EMPTY);
	if (!compiler::parser::parse(tokens, tree))
		return false;

	compiler:linker linker = compiler::linker(&tree);

	result = linker.compile();
	return true;
}

compiler::token::token(tokensTypes type, int value, char variable)
{
	this->type = type;
	this->value = value;
	this->variable = variable;
}

compiler::node::node(nodeTypes type, int value, node* op1, node* op2, node* op3)
{
	this->type = type;
	this->value = value;
	this->op1 = op1;
	this->op2 = op2;
	this->op3 = op3;
}

compiler::node::~node()
{
	delete op1;
	delete op2;
	delete op3;
}

bool compiler::parser::parse(std::vector<token>& tokens, compiler::node& result)
{
	int count = 0;
	compiler::node* node = new compiler::node(nodeTypes::EMPTY);
	bool res = parser::statement(tokens, count, node);

	if (count != tokens.size() - 1 || !res)
		return false;

	result = *node;
	return true;
}

bool compiler::parser::statement(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	node = new compiler::node(nodeTypes::EMPTY);
	bool parsed = true;
	switch (tokens[count].type)
	{
	case compiler::tokensTypes::IF:
		count++;
		node->type = nodeTypes::IF;
		parsed = compiler::parser::parenExpr(tokens, count, node->op1);

		if (!parsed)
			return false;

		parsed = compiler::parser::statement(tokens, count, node->op2);

		if (!parsed)
			return false;

		if (tokens[count].type == tokensTypes::ELSE)
		{
			count++;
			node->type = nodeTypes::IFELSE;
			bool parsed = compiler::parser::statement(tokens, count, node->op3);

			if (!parsed)
				return false;
		}
		break;

	case compiler::tokensTypes::WHILE:
		count++;
		node->type = nodeTypes::WHILE;
		parsed = compiler::parser::parenExpr(tokens, count, node->op1);

		if (!parsed)
			return false;

		parsed = compiler::parser::statement(tokens, count, node->op2);

		if (!parsed)
			return false;
		break;

	case compiler::tokensTypes::SEMICOL:
		count++;
		break;

	case compiler::tokensTypes::LBRA:
		count++;
		while (tokens[count].type != tokensTypes::RBRA && count < tokens.size())
		{
			node = new compiler::node(nodeTypes::SEQ, 0, node);

			bool parsed = compiler::parser::statement(tokens, count, node->op2);

			if (!parsed)
				return false;
		}
		count++;
		break;

	case compiler::tokensTypes::PRINT:
		count++;
		node->type = nodeTypes::PRINT;
		parsed = compiler::parser::parenExpr(tokens, count, node->op1);

		if (!parsed)
			return false;
		break;

		if (!parsed)
			return false;

	default:
		node->type = nodeTypes::EXPR;
		parsed = compiler::parser::expr(tokens, count, node->op1);

		if (tokens[count++].type != tokensTypes::SEMICOL)
			return false;

		if (!parsed)
			return false;
	}

	return true;
}

bool compiler::parser::parenExpr(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	if (tokens[count].type != tokensTypes::LPAR)
		return false;

	count++;

	bool res = expr(tokens, count, node);

	if (tokens[count].type != tokensTypes::RPAR)
		return false;

	count++;
	return true;
}

bool compiler::parser::expr(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	if (tokens[count].type != compiler::tokensTypes::VAR)
		return test(tokens, count, node);

	bool res = test(tokens, count, node);

	if (!res)
		return false;

	if (node->type == nodeTypes::VAR && tokens[count].type == tokensTypes::ASSIG)
	{
		node = new compiler::node(nodeTypes::SET, 0, node);
		count++;
		res = expr(tokens, count, node->op2);
	}

	return res;
}

bool compiler::parser::test(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	bool res = arExpr(tokens, count, node);

	if (!res)
		return false;

	if (tokens[count].type == tokensTypes::LESS)
	{
		count++;
		node = new compiler::node(nodeTypes::LESSTHEN, 0, node);
		res = arExpr(tokens, count, node->op2);
	}

	return res;
}

bool compiler::parser::arExpr(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	bool res = term(tokens, count, node);

	if (!res)
		return false;

	tokensTypes now = tokens[count].type;

	while (now == tokensTypes::ADD || now == tokensTypes::SUB || now == tokensTypes::MUL || now == tokensTypes::DIV)
	{
		nodeTypes nT = nodeTypes::EMPTY;
		switch (now)
		{
		case compiler::tokensTypes::ADD:
			nT = nodeTypes::ADD;
			break;
		case compiler::tokensTypes::SUB:
			nT = nodeTypes::SUB;
			break;
		case compiler::tokensTypes::MUL:
			nT = nodeTypes::MUL;
			break;
		case compiler::tokensTypes::DIV:
			nT = nodeTypes::DIV;
			break;
		}

		node = new compiler::node(nT, 0, node);
		count++;
		bool res = term(tokens, count, node->op2);

		if (!res)
			return false;

		now = tokens[count].type;
	}

	return true;
}

bool compiler::parser::term(std::vector<token>& tokens, int& count, compiler::node*& node)
{
	delete node;
	bool parsed;
	switch(tokens[count].type)
	{
	case tokensTypes::VAR:
		node = new compiler::node(nodeTypes::VAR, tokens[count].variable - 'a');
		count++;
		break;

	case tokensTypes::NUM:
		node = new compiler::node(nodeTypes::CONST, tokens[count].value);
		count++;
		break;

	default:
		bool res = parenExpr(tokens, count, node);

		if (!res)
			return false;

		break;
	}

	return true;
}

compiler::token compiler::getNexttoken()
{
	if (nowPos == textLen)
		return token(compiler::tokensTypes::ENDFILE);

	std::string str = "";
	char now = text[nowPos++];

	while ((now == ' ' || now == '\r' || now == '\n' || now == '\t')&& nowPos < textLen)
		now = text[nowPos++];

	if (nowPos == textLen)
		return token(compiler::tokensTypes::ENDFILE);

	while (!(now == ' ' || now == '\r' || now == '\n' || now == '\t'))
	{
		if (str.size() == 0)
		{
			if (specialSymbols.count(now))
				return token(specialSymbols[now]);
			else if (isdigit(now))
			{
				do
				{
					str += now;
					now = text[nowPos++];
				} while (isdigit(now));
				nowPos--;
				break;
			}
			else
				str += now;
		}
		else
		{
			if (specialSymbols.count(now))
			{
				nowPos--;
				break;
			}
			str += now;
		}

		if (specialWords.count(str))
			return token(specialWords[str]);

		if (nowPos == textLen)
			break;

		now = text[nowPos++];
	}

	if (str.size() == 1 && str[0] >= 'a' && str[0] <= 'z')
		return  token(tokensTypes::VAR, 0, str[0]);

	unsigned short value = 0;
	for (int i = 0; i < str.size(); i++)
	{
		if (isdigit(str[i]))
			value = value * 10 + (str[i] - '0');
		else
			return token(compiler::tokensTypes::ERROR);
	}

	return token(compiler::tokensTypes::NUM, value);
}

compiler::linker::linker(node* tree)
{
	this->tree = tree;
}

std::string compiler::linker::compileNode(compiler::node* block, int& labelCount)
{
	std::string result = "";

	int lCount1, lCount2, firstLabelNum, secondLabelNum, thirdLabelNum;

	switch (block->type)
	{
	case nodeTypes::PROG:
		result += compileNode(block->op1, labelCount);
		break;

	case nodeTypes::PRINT:
		result += compileNode(block->op1, labelCount);
		result += "        PRINT;\n";
		break;

	case nodeTypes::VAR:
		result += "        mov ax, [si + " + std::to_string(block->value * 2) + "];\n";
		break;

	case nodeTypes::CONST:
		result += "        mov ax, " + std::to_string(block->value) + ";\n";
		break;

	case nodeTypes::ADD:
		result += compileNode(block->op1, labelCount);
		result += "        push ax;\n";
		result += compileNode(block->op2, labelCount);
		result += "        mov bx, ax;\n";
		result += "        pop ax;\n";
		result += "        add ax, bx;\n";
		break;

	case nodeTypes::SUB:
		result += compileNode(block->op1, labelCount);
		result += "        push ax;\n";
		result += compileNode(block->op2, labelCount);
		result += "        mov bx, ax;\n";
		result += "        pop ax;\n";
		result += "        sub ax, bx;\n";
		break;

	case nodeTypes::MUL:
		result += compileNode(block->op1, labelCount);
		result += "        push ax;\n";
		result += compileNode(block->op2, labelCount);
		result += "        mov bx, ax;\n";
		result += "        pop ax;\n";
		result += "        mul bx;\n";
		break;

	case nodeTypes::DIV:
		result += compileNode(block->op1, labelCount);
		result += "        push ax;\n";
		result += compileNode(block->op2, labelCount);
		result += "        mov bx, ax;\n";
		result += "        pop ax;\n";
		result += "        xor dx, dx;\n";
		result += "        div bx;\n";
		break;

	case nodeTypes::SET:
		result += compileNode(block->op2, labelCount);
		result += "        mov [si + " + std::to_string(block->op1->value * 2) + "], ax;\n";
		break;

	case nodeTypes::EXPR:
		result += compileNode(block->op1, labelCount) + '\n';
		break;

	case nodeTypes::LESSTHEN:
		result += compileNode(block->op1, labelCount);
		result += "        push ax;\n";
		result += compileNode(block->op2, labelCount);
		result += "        mov bx, ax;\n";
		result += "        pop ax;\n";
		result += "        cmp ax, bx;\n";
		result += "        jae LBL" + std::to_string(labelCount++) + ";\n";
		result += "        mov ax, 1;\n";
		result += "        jmp LBL" + std::to_string(labelCount++) + ";\n";
		result += "    LBL" + std::to_string(labelCount - 2) + ":\n";
		result += "        mov ax, 0;\n";
		result += "    LBL" + std::to_string(labelCount - 1) + ": nop\n";
		break;

	case nodeTypes::IF:
		result += compileNode(block->op1, labelCount);
		result += "        cmp ax, 0;\n";
		lCount1 = labelCount;
		result += "        jne LBL" + std::to_string(labelCount++) + ";\n";
		lCount2 = labelCount;
		result += "        jmp LBL" + std::to_string(labelCount++) + ";\n";
		result += "    LBL" + std::to_string(lCount1) + ": nop\n";
		result += compileNode(block->op2, labelCount);
		result += "    LBL" + std::to_string(lCount2) + ": nop\n";
		break;

	case nodeTypes::SEQ:
		result += compileNode(block->op1, labelCount);
		result += compileNode(block->op2, labelCount);
		break;

	case nodeTypes::IFELSE:
		result += compileNode(block->op1, labelCount);
		result += "        cmp ax, 0;\n";
		firstLabelNum = labelCount;
		result += "        je LBL" + std::to_string(labelCount++) + ";\n";
		result += compileNode(block->op2, labelCount);
		secondLabelNum = labelCount;
		result += "        jmp LBL" + std::to_string(labelCount++) + ";\n";
		result += "    LBL" + std::to_string(firstLabelNum) + ": nop\n";
		result += compileNode(block->op3, labelCount);
		result += "    LBL" + std::to_string(secondLabelNum) + ": nop\n";
		break;

	case nodeTypes::WHILE:
		secondLabelNum = labelCount;
		result += "        jmp LBL" + std::to_string(labelCount++) + "; \n";
		firstLabelNum = labelCount;
		result += "    LBL" + std::to_string(labelCount++) + ": nop\n";
		result += compileNode(block->op2, labelCount);
		result += "    LBL" + std::to_string(secondLabelNum) + ": nop\n";
		result += compileNode(block->op1, labelCount);
		result += "        cmp ax, 0;\n";
		thirdLabelNum = labelCount;
		result += "        je LBL" + std::to_string(thirdLabelNum) + "\n";
		result += "        jmp LBL" + std::to_string(firstLabelNum) + "\n";
		result += "    LBL" + std::to_string(thirdLabelNum) + ": nop\n";
		break;

	default:
		break;
	}

	return result;
}

std::string compiler::linker::compile()
{
	std::string program = "";
	program += programsStart;
	int a = 0;
	program += compileNode(tree, a);
	program += programsEnd;
	return  program;
}

const std::string compiler::linker::programsStart = R"STR(ASSUME CS:CODE, DS:DATA

DATA SEGMENT
        vars dw 26 dup (0)
DATA ENDS;

CODE SEGMENT
        PRINT MACRO
        local DIVCYC, PRINTNUM;
        push ax;
        push bx;
        push cx;
        push dx;
        mov bx, 10;
        mov cx, 0;
    DIVCYC:
        mov dx, 0;
        div bx;
        push dx;
        inc cx;
        cmp ax, 0;
        jne DIVCYC;
    PRINTNUM:
        pop dx;
        add dl, '0';
        mov ah, 02h;
        int 21h;
        dec cx;
        cmp cx, 0;
        jne PRINTNUM;
		mov dl, 0dh;
        mov ah, 02h;
        int 21h;
		mov dl, 0ah;
        mov ah, 02h;
        int 21h;
        pop dx;
        pop cx;
        pop bx;
        pop ax;
        ENDM;

    begin:
        lea si, vars;
)STR";

const std::string compiler::linker::programsEnd = R"STR(        mov ah, 4Ch;
        int 21h;
CODE ENDS;
end begin)STR";