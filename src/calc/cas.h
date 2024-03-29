#ifndef CAS
#define CAS

#include "backend.h"

// symbolic derivative options
extern string diff_id;
extern bool is_partial;

unique_ptr<TreeNode> symb_deriv(unique_ptr<TreeNode>&& node);
unique_ptr<TreeNode> symb_simp(unique_ptr<TreeNode>&& tree);
unique_ptr<TreeNode> symb_expand(unique_ptr<TreeNode>&& tree, bool is_simplified = false);

pair<unique_ptr<TreeNode>, unique_ptr<TreeNode>> const_and_base(unique_ptr<TreeNode>& node);
pair<unique_ptr<TreeNode>, unique_ptr<TreeNode>> base_and_exp(unique_ptr<TreeNode>& node);

unique_ptr<TreeNode> binarize(unique_ptr<TreeNode>&& tree);
unique_ptr<TreeNode> pretty_tree(unique_ptr<TreeNode>&& tree);

unique_ptr<TreeNode> merge_sums(unique_ptr<TreeNode>&& _a, unique_ptr<TreeNode>&& _b);
unique_ptr<TreeNode> merge_products(unique_ptr<TreeNode>&& _a, unique_ptr<TreeNode>&& _b);

#endif // CAS
