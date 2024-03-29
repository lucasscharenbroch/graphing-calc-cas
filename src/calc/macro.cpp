#include "cas.h"

unique_ptr<TreeNode> print_tree(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> get_last_answer(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> clear_screen(unique_ptr<TreeNode>&& node);

unique_ptr<TreeNode> graph_expression(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> ungraph_expression(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> graph_axes(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> ungraph_axes(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> set_graph_window(unique_ptr<TreeNode>&& node);

unique_ptr<TreeNode> sqrt_macro(unique_ptr<TreeNode>&& node);

unique_ptr<TreeNode> deriv(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> simp(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> expand(unique_ptr<TreeNode>&& node);

void init_macro_functions() {
    // debug/runtime:
    macro_table["print_tree"] = make_unique<macro_fn>(print_tree);
    macro_table["ans"] = make_unique<macro_fn>(get_last_answer);
    macro_table["clear"] = make_unique<macro_fn>(clear_screen);

    // graphing:
    macro_table["graph"] = make_unique<macro_fn>(graph_expression);
    macro_table["ungraph"] = make_unique<macro_fn>(ungraph_expression);
    macro_table["graph_axes"] = make_unique<macro_fn>(graph_axes);
    macro_table["ungraph_axes"] = make_unique<macro_fn>(ungraph_axes);
    macro_table["set_graph_window"] = make_unique<macro_fn>(set_graph_window);

    // math:
    macro_table["sqrt"] = make_unique<macro_fn>(sqrt_macro);

    // cas:
    macro_table["deriv"] = make_unique<macro_fn>(deriv);
    macro_table["simp"] = make_unique<macro_fn>(simp);
    macro_table["expand"] = make_unique<macro_fn>(expand);
}

void init_macro_constants() {
    identifier_table["DERIV_STEP"] = DERIV_STEP;
    identifier_table["INT_NUM_RECTS"] = 100;
    identifier_table["TICS_ENABLED"] = 1;

    identifier_table["ECHO_AUTO"] = 1;
    identifier_table["ECHO_TREE"] = 0;
    identifier_table["ECHO_ANS"] = 0;
    identifier_table["PARTIAL"] = 1;
    identifier_table["AUTO_SIMP"] = 1;
    identifier_table["INT_POWER_EXPANSION_THRESHOLD"] = 3;
}

unique_ptr<TreeNode> tree_node_exe_macro(unique_ptr<TreeNode>&& node) {
    if(node->type() == nt_fn_call) {
        string id = ((FunctionCallNode *)node.get())->fn_id;
        return execute_macro(id, std::move(node));
    } else return node;
}

/* ~ ~ ~ ~ ~ Debug/Runtime Functions ~ ~ ~ ~ ~ */

// print_tree: debug function - prints out the parsed grammar tree of
// each of the passed arguments.
unique_ptr<TreeNode> print_tree(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;
    for(auto& arg : args) cout << arg->to_string() << endl;
    return make_unique<NumberNode>(NAN);
}

unique_ptr<TreeNode> get_last_answer(unique_ptr<TreeNode>&& node) {
    return make_unique<NumberNode>(last_answer);
}

unique_ptr<TreeNode> clear_screen(unique_ptr<TreeNode>&& node) {
    emscripten_run_script("clear_screen();");
    return make_unique<NumberNode>(NAN);
}

/* ~ ~ ~ ~ ~ Graphing Functions ~ ~ ~ ~ ~ */

unique_ptr<TreeNode> graph_expression(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;
    if(args.size() != 1) throw calculator_error("graph(...) accepts exactly 1 argument: " +
                                                to_string(args.size()) + " were supplied");

    if(get_id_value("AUTO_SIMP")) add_to_graph(pretty_tree(binarize(symb_simp(std::move(args[0])))));
    else add_to_graph(std::move(args[0]));
    return make_unique<NumberNode>(NAN);
}

unique_ptr<TreeNode> ungraph_expression(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;
    int arg = args.size() ? args[0]->eval() : 0;

    emscripten_run_script(("remove_graph_fn(" + to_string(arg) + ")").data());

    return make_unique<NumberNode>(NAN);
}

unique_ptr<TreeNode> graph_axes(unique_ptr<TreeNode>&& node) {
    draw_axes();
    return make_unique<NumberNode>(NAN);
}

unique_ptr<TreeNode> ungraph_axes(unique_ptr<TreeNode>&& node) {
    undraw_axes();
    return make_unique<NumberNode>(NAN);
}

unique_ptr<TreeNode> set_graph_window(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;
    if(args.size() != 4) throw calculator_error("set_graph_window(...) accepts exactly 4 argument: "
                                                "got " + to_string(args.size()));

    double x_min = args[0]->eval();
    double y_min  = args[1]->eval();
    double width = args[2]->eval();
    double height = args[3]->eval();
    double x_max = x_min + width;
    double y_max = y_min + height;

    emscripten_run_script(("x_min = " + to_string(x_min) + ","
                           "y_min = " + to_string(y_min) + ","
                           "x_max = " + to_string(x_max) + ","
                           "y_max = " + to_string(y_max) + ","
                           "graph_dimensions_changed = true;").c_str());

    return make_unique<NumberNode>(NAN);
}

/* ~ ~ ~ ~ ~ Math ~ ~ ~ ~ ~ */

unique_ptr<TreeNode> sqrt_macro(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;
    if(args.size() != 1) throw calculator_error("sqrt(...) accepts exactly 1 argument: " +
                                                to_string(args.size()) + " were supplied");
    return make_unique<BinaryOpNode>(std::move(args[0]), make_unique<NumberNode>(0.5), "^");
}

/* ~ ~ ~ ~ ~ Computer Algebra System Functions ~ ~ ~ ~ ~ */

extern string diff_id;

unique_ptr<TreeNode> deriv(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;

    if(args.size() == 1) { // TODO differentiate w/r/t x by default
        diff_id = "x";
    } else if(args.size() != 2) {
        throw calculator_error("deriv(...) accepts exactly 2 argument; got " +
                                to_string(args.size()) + " instead");
    } else {
        if(args[1]->type() != nt_id) throw calculator_error("can't differentiate with respect to "
                                                            "non-identifier");

        diff_id = ((VariableNode *)args[1].get())->id;
    }

    is_partial = get_id_value("PARTIAL");

    if(get_id_value("AUTO_SIMP")) return pretty_tree(binarize(symb_simp(symb_deriv(std::move(args[0])))));
    else return symb_deriv(std::move(args[0]));
}

unique_ptr<TreeNode> simp(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;

    if(args.size() != 1)
        throw calculator_error("simp(...) accepts exactly 1 argument; got " +
                                to_string(args.size()) + " instead");

    return pretty_tree(binarize(symb_simp(std::move(args[0]))));
}

unique_ptr<TreeNode> expand(unique_ptr<TreeNode>&& node) {
    vector<unique_ptr<TreeNode>>& args = ((FunctionCallNode *)node.get())->args;

    if(args.size() != 1)
        throw calculator_error("expand(...) accepts exactly 1 argument; got " +
                                to_string(args.size()) + " instead");

    return pretty_tree(binarize(symb_expand(std::move(args[0]))));
}
