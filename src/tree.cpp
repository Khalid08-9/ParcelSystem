#include <iostream>

#include "ParcelTree.hpp"
#include "Result.hpp"

treeNode *treeCreate(const Parcel &p) {
    treeNode *newNode = new treeNode();
    newNode->parcel = Parcel(p);
    newNode->left = newNode->right = nullptr;
    return newNode;
}

treeNode *treeInsert(treeNode *root, const Parcel &p) {
    if (root == nullptr) {
        return treeCreate(p);
    }
    if (p.parcelID < root->parcel.parcelID) {
        root->left = treeInsert(root->left, p);
    } else {
        root->right = treeInsert(root->right, p);
    }
    return root;
}

void inorderTraversal(treeNode *root) {
    if (root != nullptr) {
        inorderTraversal(root->left);
        std::cout << root->parcel.parcelID << " ";
        inorderTraversal(root->right);
    }
}

Parcel *treeSearch(treeNode *root, const std::string &key) {
    if (root == nullptr || root->parcel.parcelID == key) {
        return &root->parcel;
    }
    if (key < root->parcel.parcelID) {
        return treeSearch(root->left, key);
    } else {
        return treeSearch(root->right, key);
    }
}

treeNode *minValueNode(treeNode *node) {
    treeNode *current = node;
    // loop down to find the leftmost leaf
    while (current && current->left != nullptr) {
        current = current->left;
    }
    return current;
}

treeNode *treeRemove(treeNode *root, const std::string &key) {
    if (root == nullptr) return root;

    if (key < root->parcel.parcelID) {
        root->left = treeRemove(root->left, key);
    } else if (key > root->parcel.parcelID) {
        root->right = treeRemove(root->right, key);
    } else {
        // node with only one child or no child
        if (root->left == nullptr) {
            treeNode *temp = root->right;
            delete root;
            return temp;
        } else if (root->right == nullptr) {
            treeNode *temp = root->left;
            delete root;
            return temp;
        }

        // node with two children: Get the inorder successor
        // (smallest in the right subtree)
        treeNode *temp = minValueNode(root->right);

        // Copy the inorder successor's content to this node
        root->parcel = temp->parcel;

        // Delete the inorder successor
        root->right = treeRemove(root->right, temp->parcel.parcelID);
    }
    return root;
}
