#ifndef CALCULATOR
#define CALCULATOR

#include <vector>
#include <regex>
#include <cmath>
#include <iostream>
#include <numeric>
#include <memory>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <emscripten.h>
#include <cassert>

using namespace std;

extern double last_answer; // holds result of last computation

/* ~ ~ ~ ~ ~ Parsing Tree Class ~ ~ ~ ~ ~ */

enum node_type {
    // this list is in "lexicograpical" order: it effects simplification.
    nt_num,
    nt_id,
    nt_fn_call,
    nt_deriv,
    nt_negation,
    nt_exponentiation,
    nt_sum,
    nt_nary_sum,
    nt_product,
    nt_nary_product,
    nt_difference,
    nt_quotient,
    nt_int_quotient,
    nt_modulus,
    nt_eq,
    nt_ne,
    nt_lt,
    nt_le,
    nt_gt,
    nt_ge,
    nt_assignment,
    nt_none
};

struct TreeNode { // Abstract superclass for all other node types
    virtual string to_string(enum node_type parent_type = nt_none) = 0;
    virtual string to_latex_string(enum node_type parent_type = nt_none) { return to_string(parent_type); }
    virtual double eval() = 0;
    virtual unique_ptr<TreeNode> exe_on_children(unique_ptr<TreeNode>&& self,
            function<unique_ptr<TreeNode>(unique_ptr<TreeNode>&&)> fn) { return fn(std::move(self)); }
    virtual unique_ptr<TreeNode> copy() = 0;
    virtual enum node_type type() = 0;
    virtual ~TreeNode() { }
};


/* ~ ~ ~ ~ ~ Calculator Backend ~ ~ ~ ~ ~ */

double get_id_value(string id);
double set_id_value(string id, double val);
double call_function(string id, vector<unique_ptr<TreeNode>>& args);
void assign_function(string id, vector<string>&& args, unique_ptr<TreeNode>&& tree);
unique_ptr<TreeNode> execute_macro(string id, unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> tree_node_exe_macro(unique_ptr<TreeNode>&& node);

void init_constants();
void init_functions();
void init_math_functions();
void init_macro_functions();

typedef function<unique_ptr<TreeNode>(unique_ptr<TreeNode>&&)> macro_fn;

const double DERIV_STEP = 1e-6;

/* ~ ~ ~ ~ ~ Calculator Errors ~ ~ ~ ~ ~ */

struct calculator_error : public runtime_error {
    calculator_error(const string& what) : runtime_error(what) { }
    virtual string error_type() { return "Calculator Error"; }
    string to_string() {
        return error_type() + ": " + runtime_error::what() + ".";
    }
};

struct invalid_function_call_error : public calculator_error {
    invalid_function_call_error(const string& what) : calculator_error(what) { }
    string error_type() override { return "Invalid Function Call"; }
};

struct invalid_argument_error : public calculator_error {
    invalid_argument_error(const string& what) : calculator_error(what) { }
    string error_type() override { return "Invalid Argument"; }
};

struct invalid_token_error : public calculator_error {
    invalid_token_error(const string& what) : calculator_error(what) { }
    string error_type() override { return "Invalid Token"; }
};

struct invalid_expression_error : public calculator_error {
    invalid_expression_error(const string& what) : calculator_error(what) { }
    string error_type() override { return "Invalid Expression"; }
};

/* ~ ~ ~ ~ ~ Graping Functions ~ ~ ~ ~ ~ */

void set_x_y_minmax(double xi, double xa, double yi, double ya);
bool add_to_graph(unique_ptr<TreeNode>&& expr);
void draw_axes();
void undraw_axes();

/* ~ ~ ~ ~ ~ Exported Functions ~ ~ ~ ~ ~ */

extern "C" {
    /* ~ Calculator ~ */
    void init();
    char *calculate_text(const char *, bool);
    char *get_latex_result();

    /* ~ Graphing ~ */
    int *get_graph_buffer();
    bool remove_from_graph(int);
    void resize_graph(int, int, double, double, double, double);
    void draw_trace_line(int x_c);
}

#endif // CALCULATOR
