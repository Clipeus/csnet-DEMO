#include <string>
#include <vector>
#include <cmath>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include "expression.h"

namespace csnet
{
	double expression_t::eval(const expression_t& e)
	{
		switch (e.args.size()) 
		{
		case 2: 
		{
			auto a = eval(e.args[0]);
			auto b = eval(e.args[1]);

			if (e.token == "+") 
				return a + b;
			if (e.token == "-") 
				return a - b;
			if (e.token == "*") 
				return a * b;
			if (e.token == "/") 
				return a / b;
			if (e.token == "**") 
				return pow(a, b);
			if (e.token == "mod") 
				return (int)a % (int)b;

			throw std::runtime_error("Unknown binary operator");
		}

		case 1: 
		{
			auto a = eval(e.args[0]);

			if (e.token == "+") 
				return +a;
			if (e.token == "-") 
				return -a;
			if (e.token == "abs") 
				return abs(a);
			if (e.token == "sin") 
				return sin(a);
			if (e.token == "cos") 
				return cos(a);

			throw std::runtime_error("Unknown unary operator");
		}

		case 0:
			return strtod(e.token.c_str(), nullptr);
		}

		throw std::runtime_error("Unknown expression type");
	}

	std::string parser_t::parse_token()
	{
		while (std::isspace(_input[_curpos])) 
			++_curpos;

		if (std::isdigit(_input[_curpos])) 
		{
			std::string number;
			for (; std::isdigit(_input[_curpos]) || _input[_curpos] == '.'; _curpos++)
				number.push_back(_input[_curpos]);

			return number;
		}

		static const std::string tokens[] =	{ "+", "-", "**", "*", "/", "mod", "abs", "sin", "cos", "(", ")" };

		for (auto& t : tokens) 
		{
			if (std::strncmp(_input.c_str() + _curpos, t.c_str(), t.size()) == 0) 
			{
				_curpos += t.size();
				return t;
			}
		}

		return "";
	}

	expression_t parser_t::parse_simple_expression()
	{
		auto token = parse_token();
		if (token.empty()) 
			throw std::runtime_error("Invalid input");

		if (token == "(") 
		{
			expression_t result = parse();
			if (parse_token() != ")") 
				throw std::runtime_error("Expected ')'");
			return result;
		}

		if (std::isdigit(token[0]))
			return expression_t(token);

		return expression_t(token, parse_simple_expression());
	}

	int parser_t::get_priority(const std::string& binary_op)
	{
		if (binary_op == "+") 
			return 1;
		if (binary_op == "-") 
			return 1;
		if (binary_op == "*") 
			return 2;
		if (binary_op == "/") 
			return 2;
		if (binary_op == "mod") 
			return 2;
		if (binary_op == "**") 
			return 3;
		return 0;
	}

	expression_t parser_t::parse_binary_expression(int min_priority)
	{
		auto left_expr = parse_simple_expression();

		while (true) 
		{
			std::string op = parse_token();
			int priority = get_priority(op);
			if (priority <= min_priority) 
			{
				_curpos -= op.size();
				return left_expr;
			}

			expression_t right_expr = parse_binary_expression(priority);
			left_expr = expression_t(op, left_expr, right_expr);
		}
	}

	expression_t parser_t::parse()
	{
		return parse_binary_expression(0);
	}
}