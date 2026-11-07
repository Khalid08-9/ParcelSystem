#include <cstddef>
#include <iostream>

#include "ParcelTree.hpp"

struct treeNode {
    Parcel parcel;
    treeNode *left;
    treeNode *right;
};

namespace {
    bool isBlank(const std::string &text) {
        return text.find_first_not_of(" \t\r\n") == std::string::npos;
    }

    bool hasRequiredDetails(const Parcel &parcel) {
        return !isBlank(parcel.parcelID) && !isBlank(parcel.recipientName) &&
               !isBlank(parcel.recipientID) && !isBlank(parcel.deliveryAddress);
    }

    treeNode *treeCreate(const Parcel &parcel) {
        treeNode *node = new treeNode{parcel, nullptr, nullptr};
        node->parcel.next = nullptr; // A tree record is not part of the registry's linked list.
        return node;
    }

    treeNode *treeInsert(treeNode *root, const Parcel &parcel) {
        if (root == nullptr) return treeCreate(parcel);
        if (parcel.parcelID < root->parcel.parcelID) {
            root->left = treeInsert(root->left, parcel);
        } else {
            root->right = treeInsert(root->right, parcel);
        }
        return root;
    }

    const treeNode *findNode(const treeNode *root, const std::string &id) {
        if (root == nullptr || root->parcel.parcelID == id) return root;
        return id < root->parcel.parcelID
                   ? findNode(root->left, id)
                   : findNode(root->right, id);
    }

    treeNode *findNode(treeNode *root, const std::string &id) {
        if (root == nullptr || root->parcel.parcelID == id) return root;
        return id < root->parcel.parcelID
                   ? findNode(root->left, id)
                   : findNode(root->right, id);
    }

    void inorderTraversal(const treeNode *root) {
        if (root == nullptr) return;
        inorderTraversal(root->left);
        std::cout << root->parcel.parcelID << " ";
        inorderTraversal(root->right);
    }

    treeNode *minValueNode(treeNode *node) {
        while (node->left != nullptr) node = node->left;
        return node;
    }

    treeNode *treeRemove(treeNode *root, const std::string &id) {
        if (root == nullptr) return nullptr;
        if (id < root->parcel.parcelID) {
            root->left = treeRemove(root->left, id);
        } else if (id > root->parcel.parcelID) {
            root->right = treeRemove(root->right, id);
        } else {
            if (root->left == nullptr) {
                treeNode *right = root->right;
                delete root;
                return right;
            }
            if (root->right == nullptr) {
                treeNode *left = root->left;
                delete root;
                return left;
            }

            treeNode *successor = minValueNode(root->right);
            root->parcel = successor->parcel;
            root->right = treeRemove(root->right, successor->parcel.parcelID);
        }
        return root;
    }

    std::size_t flattenTree(treeNode *&root) {
        // Right rotations turn the BST into a sorted, right-only chain.
        // Iterate rather than recurse so a skewed tree does not exhaust the stack.
        std::size_t count = 0;
        treeNode **link = &root;
        while (*link != nullptr) {
            treeNode *node = *link;
            if (node->left != nullptr) {
                treeNode *left = node->left;
                node->left = left->right;
                left->right = node;
                *link = left;
            } else {
                ++count;
                link = &node->right;
            }
        }
        return count;
    }

    treeNode *buildBalancedTree(treeNode *&next, std::size_t count) {
        if (count == 0) return nullptr;

        // Consume the sorted chain in order, using its middle node as the root.
        const std::size_t leftCount = count / 2;
        treeNode *left = buildBalancedTree(next, leftCount);
        treeNode *root = next;
        next = next->right;
        root->left = left;
        root->right = buildBalancedTree(next, count - leftCount - 1);
        return root;
    }

    void clearTree(treeNode *root) {
        if (root == nullptr) return;
        clearTree(root->left);
        clearTree(root->right);
        delete root;
    }
}

ResultCode insertParcelRecord(treeNode *&root, const Parcel &parcel) {
    if (!hasRequiredDetails(parcel)) return ResultCode::InvalidInput;
    if (findNode(root, parcel.parcelID) != nullptr) return ResultCode::Duplicate;
    root = treeInsert(root, parcel);
    return ResultCode::Success;
}

ResultCode findParcelRecord(const treeNode *root, const std::string &id, const Parcel *&found) {
    found = nullptr;
    if (isBlank(id)) return ResultCode::InvalidInput;
    const treeNode *node = findNode(root, id);
    if (node == nullptr) return ResultCode::NotFound;
    found = &node->parcel;
    return ResultCode::Success;
}

ResultCode displayParcelRecords(const treeNode *root) {
    if (root == nullptr) return ResultCode::Empty;
    inorderTraversal(root);
    return ResultCode::Success;
}

ResultCode removeParcelRecord(treeNode *&root, const std::string &id) {
    if (isBlank(id)) return ResultCode::InvalidInput;
    if (findNode(root, id) == nullptr) return ResultCode::NotFound;
    root = treeRemove(root, id);
    return ResultCode::Success;
}

ResultCode updateParcelRecord(treeNode *root, const std::string &id, const Parcel &replacement) {
    if (isBlank(id) || replacement.parcelID != id || !hasRequiredDetails(replacement))
        return ResultCode::InvalidInput;

    treeNode *node = findNode(root, id);
    if (node == nullptr) return ResultCode::NotFound;
    node->parcel = replacement;
    node->parcel.next = nullptr;
    return ResultCode::Success;
}

ResultCode balanceParcelRecords(treeNode *&root) {
    if (root == nullptr) return ResultCode::Empty;
    const std::size_t count = flattenTree(root);
    treeNode *next = root;
    root = buildBalancedTree(next, count);
    return ResultCode::Success;
}

void clearParcelRecords(treeNode *&root) {
    clearTree(root);
    root = nullptr;
}
