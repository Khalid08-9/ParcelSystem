#ifndef PARCEL_TREE_HPP
#define PARCEL_TREE_HPP

#include "Parcel.hpp"

struct treeNode {
    Parcel parcel;
    treeNode* left;
    treeNode* right;
};

treeNode* treeCreate(const Parcel& p);
treeNode* treeInsert(treeNode* root, const Parcel& p);
void inorderTraversal(treeNode* root);

#endif
