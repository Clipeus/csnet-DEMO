#pragma once

namespace csnet
{
  struct expression_t
  {
    expression_t(const std::string& token) : token(token) {}
    expression_t(const std::string& token, expression_t a) : token(token), args{ a } {}
    expression_t(const std::string& token, expression_t a, expression_t b) : token(token), args{ a, b } {}

    static double eval(const expression_t& e);

    std::string token;
    std::vector<expression_t> args;
  };

  class parser_t {
  public:
    explicit parser_t(const std::string& input) : _input(input) {}
    expression_t parse();

  private:
    std::string parse_token();
    expression_t parse_simple_expression();
    expression_t parse_binary_expression(int min_priority);
    static int get_priority(const std::string& binary_op);

    std::string _input;
    size_t _curpos = 0;
  };
}

