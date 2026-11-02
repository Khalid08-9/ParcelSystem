#ifndef PARCEL_TREE_HPP
#define PARCEL_TREE_HPP

#include <string>
#include "Parcel.hpp"

struct treeNode {
    Parcel parcel;
    treeNode* left;
    treeNode* right;
};

treeNode* treeCreate(const Parcel& p);
treeNode* treeInsert(treeNode* root, const Parcel& p);
Parcel* treeSearch(treeNode* root, const std::string& key);
treeNode* treeRemove(treeNode* root, const std::string& key);
void treeUpdate(treeNode* root, treeNode* key, const Parcel& p);
void inorderTraversal(treeNode* root);

#endif
