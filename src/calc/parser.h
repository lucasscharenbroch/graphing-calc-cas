#ifndef PARSER
#define PARSER

#include "../calculator.h"

/* ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ Token Classes ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ */

// Token base-class (abstract - only used for inheritance)
struct Token {
    virtual string to_string() = 0;
    virtual string str_val() { return ""; }
    virtual double dbl_val() { return NAN; }
    virtual bool is_var() { return false; }
    virtual bool is_num() { return false; }
    virtual bool is_op() { return false; }
    virtual ~Token() { }
};

struct VarToken : Token {
    string identifier;
    VarToken(string id) : identifier(id) { }
    string to_string() override { return identifier; }
    string str_val() override { return identifier; }
    bool is_var() override { return true; }
};

struct NumToken : Token {
    double value;
    NumToken(double val) : value(val) { }
    string to_string() override { return std::to_string(value); }
    double dbl_val() override { return value; }
    bool is_num() override { return true; }
};

struct OpToken : Token {
    string operand;
    OpToken(string op) : operand(op) { }
    string to_string() override { return operand; }
    string str_val() override { return operand; }
    bool is_op() override { return true; }
};

/* ~ ~ ~ ~ ~ Lexing Functions ~ ~ ~ ~ ~ */

vector<unique_ptr<Token>> tokenize(const string& expr_str);

/* ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ Operation (Parsing Tree Node) Classes ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ */

int precedence(enum node_type type);
bool is_binary_op(enum node_type type);
bool is_unary_op(enum node_type type);

struct NumberNode : TreeNode {
    double val;

    NumberNode(double v) : val(v) { }

    string to_string(enum node_type parent_type = nt_none) override {
        return std::to_string(val);
    }

    double eval() override {
        return val;
    }

    unique_ptr<TreeNode> copy() override {
        return unique_ptr<TreeNode> {new NumberNode(val)};
    }

    enum node_type type() override {
        return nt_num;
    }
};

struct VariableNode : TreeNode {
    string id;

    VariableNode(string i) : id(i) { }

    string to_string(enum node_type parent_type = nt_none) override {
        return id;
    }

    double eval() override {
        return get_id_value(id);
    }

    unique_ptr<TreeNode> copy() override {
        return unique_ptr<TreeNode> {new VariableNode(id)};
    }

    enum node_type type() override {
        return nt_id;
    }
};

struct FunctionCallNode : TreeNode {
    string function_id;
    vector<unique_ptr<TreeNode>> args;

    FunctionCallNode(string i, vector<unique_ptr<TreeNode>>&& a) :
        function_id(i),
        args(std::move(a)) { }

    string to_string(enum node_type parent_type = nt_none) override {
        string s = function_id + "(";

        for(int i = 0; i < args.size(); i++) {
            s += args[i]->to_string(nt_none);
            if(i != args.size() - 1) s += ", ";
        }

        s += ")";
        return s;
    }

    double eval() override {
        return call_function(function_id, args);
    }

    unique_ptr<TreeNode> exe_macros(unique_ptr<TreeNode>&& self) override {
        for(auto& arg : args) arg = arg->exe_macros(std::move(arg));
        return execute_macro(function_id, std::move(self)); // this does nothing if
                                                            // function_id isn't a macro
    }

    unique_ptr<TreeNode> copy() override {
        vector<unique_ptr<TreeNode>> args_copy;
        for(int i = 0; i < args.size(); i++) args_copy.push_back(args[i]->copy());

        return unique_ptr<TreeNode> {new FunctionCallNode(function_id, std::move(args_copy))};
    }

    enum node_type type() override {
        return nt_fn_call;
    }
};

struct BinaryOpNode : TreeNode {
    unique_ptr<TreeNode> left, right;
    string operand;

    BinaryOpNode(unique_ptr<TreeNode>&& l, unique_ptr<TreeNode>&& r, string op) :
        left(std::move(l)),
        right(std::move(r)),
        operand(op) { }

    string to_string(enum node_type parent_type = nt_none) override {
        if(precedence(parent_type) < precedence(type()) ||
           precedence(parent_type) == precedence(type()) &&
           (type() == nt_sum || type() == nt_difference || type() == nt_product)) // these are obvious enough
            return left->to_string(type()) + ' ' + operand + ' ' + right->to_string(type());
        else
            return '(' + left->to_string(type()) + ' ' + operand + ' ' + right->to_string(type()) + ')';
    }

    double eval() override {
        if(operand == "//" || operand == "%") {
            long long numerator = (long long)left->eval();
            long long denominator = (long long)right->eval();

            if(denominator == 0) return NAN;
            return operand == "//" ? numerator / denominator : numerator % denominator;
        } else if(operand == "=") {
            if(left->type() == nt_id) { // variable assignment
                return set_id_value(((VariableNode *)left.get())->id, right->eval());
            } else { // function assignment
                FunctionCallNode *lhs = (FunctionCallNode *)left.get();
                string function_id = lhs->function_id;
                vector<string> arg_ids;

                for(unique_ptr<TreeNode>& arg_node : lhs->args) {
                    if(arg_node->type() != nt_id)
                        throw invalid_expression_error("cannot assign to a function with"
                                                       " a non-identifier parameter");
                    arg_ids.push_back(((VariableNode *)arg_node.get())->id);
                }

                assign_function(function_id, std::move(arg_ids), std::move(right->copy()));

                return NAN;
            }
        }
        else if(operand == "+") return left->eval() + right->eval();
        else if(operand == "-") return left->eval() - right->eval();
        else if(operand == "*") return left->eval() * right->eval();
        else if(operand == "/") return left->eval() / right->eval();
        else if(operand == "^" || operand == "**") return pow(left->eval(), right->eval());
        else if(operand == "==") return left->eval() == right->eval();
        else if(operand == "!=") return left->eval() != right->eval();
        else if(operand == "<") return left->eval() < right->eval();
        else if(operand == ">") return left->eval() > right->eval();
        else if(operand == "<=") return left->eval() <= right->eval();
        else if(operand == ">=") return left->eval() >= right->eval();
        else assert(false);
    }

    unique_ptr<TreeNode> exe_macros(unique_ptr<TreeNode>&& self) override {
        left = left->exe_macros(std::move(left));
        right = right->exe_macros(std::move(right));
        return std::move(self);
    }

    unique_ptr<TreeNode> copy() override {
        return unique_ptr<TreeNode> {new BinaryOpNode(left->copy(), right->copy(), operand)};
    }


    enum node_type type() override {
        if(operand == "//") return nt_int_quotient;
        else if(operand == "%") return nt_modulus;
        else if(operand == "=") return nt_assignment;
        else if(operand == "+") return nt_sum;
        else if(operand == "-") return nt_difference;
        else if(operand == "*") return nt_product;
        else if(operand == "/") return nt_quotient;
        else if(operand == "^" || operand == "**") return nt_exponentiation;
        else if(operand == "==") return nt_eq;
        else if(operand == "!=") return nt_ne;
        else if(operand == "<") return nt_lt;
        else if(operand == ">") return nt_gt;
        else if(operand == "<=") return nt_le;
        else if(operand == ">=") return nt_ge;
        else assert(false);
    }
};

struct UnaryOpNode : TreeNode {
    unique_ptr<TreeNode> arg;
    string operand;

    UnaryOpNode(unique_ptr<TreeNode>&& a, string op) :
        arg(std::move(a)),
        operand(op) { }

    string to_string(enum node_type parent_type = nt_none) override {
        if(precedence(parent_type) < precedence(type()))
            return operand + arg->to_string(type());
        else
            return '(' + operand + arg->to_string(type()) + ')';
    }

    double eval() override {
        if(operand == "-") return -1 * arg->eval();
        else assert(false);
    }

    unique_ptr<TreeNode> exe_macros(unique_ptr<TreeNode>&& self) override {
        arg = arg->exe_macros(std::move(arg));
        return std::move(self);
    }

    unique_ptr<TreeNode> copy() override {
        return unique_ptr<TreeNode> {new UnaryOpNode(arg->copy(), operand)};
    }


    enum node_type type() override {
        if(operand == "-") return nt_negation;
        else assert(false);
    }
};

struct DerivativeNode : TreeNode {
    int nth_deriv;
    string fn_id;
    vector<unique_ptr<TreeNode>> args;


    DerivativeNode(string f, vector<unique_ptr<TreeNode>>&& a, int n) :
        fn_id(f),
        args(std::move(a)),
        nth_deriv(n) { }

    string to_string(enum node_type parent_type = nt_none) override {
        string s = fn_id;
        for(int i = 0; i < nth_deriv; i++) s += "'";
        s += "(";
        s += args.size() == 0 ? "" : args[0]->to_string(nt_none);
        s += ")";
        return s;
    }

    // calculates the n'th derivative of fn_id at %at%
    double nderiv(int n, double at) {
        if(n == 0) {
            vector<unique_ptr<TreeNode>> arg_vec;
            arg_vec.push_back(make_unique<NumberNode>(at));
            return call_function(fn_id, arg_vec);
        }
        return (nderiv(n - 1, at + DERIV_STEP) - nderiv(n - 1, at - DERIV_STEP)) / (2 * DERIV_STEP);
    }

    double eval() override {
        if(args.size() == 0) throw invalid_expression_error("can't implicitly differentiate a "
                                                            "function with no arguments");
        if(args.size() > 1) throw invalid_expression_error("can't implicitly differentiate a "
                                                           "function with more than one argument "
                                                           "(consider using nderiv)");
        return nderiv(nth_deriv, args[0]->eval());
    }

    unique_ptr<TreeNode> exe_macros(unique_ptr<TreeNode>&& self) override {
        for(auto& arg : args) arg = arg->exe_macros(std::move(arg));
        return std::move(self);
    }

    unique_ptr<TreeNode> copy() override {
        vector<unique_ptr<TreeNode>> args_copy;
        for(int i = 0; i < args.size(); i++) args_copy.push_back(args[i]->copy());

        return unique_ptr<TreeNode> {new DerivativeNode(fn_id, std::move(args_copy), nth_deriv)};
    }


    enum node_type type() override {
        return nt_deriv;
    }
};

/* ~ ~ ~ ~ ~ Grammar Parsing Functions ~ ~ ~ ~ ~ */

unique_ptr<TreeNode> parseS(vector<unique_ptr<Token>>&& token_vec);
unique_ptr<TreeNode> parseE();
unique_ptr<TreeNode> parseT();
unique_ptr<TreeNode> parseF();
unique_ptr<TreeNode> parseX();
vector<unique_ptr<TreeNode>> parseARGS();

#endif // PARSER
