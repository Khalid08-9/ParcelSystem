#include <iostream>
#include <initializer_list>
#include <sstream>
#include <string>

#include "ParcelRegistry.hpp"
#include "ParcelTree.hpp"

// tree_tests only links tree.cpp. Include these implementations here so this
// test can exercise the registry without changing CMakeLists.txt or main.cpp.
#include "../src/ParcelQueue.cpp"
#include "../src/Status.cpp"
#include "../src/ParcelRegistry.cpp"

namespace {

Parcel makeParcel() {
    Parcel parcel{};
    parcel.recipientName = "Test Recipient";
    parcel.recipientID = "TEST001";
    parcel.deliveryAddress = "Campus";
    parcel.type = ParcelType::Document;
    return parcel;
}

void destroyTree(treeNode* root) {
    if (root == nullptr) return;
    destroyTree(root->left);
    destroyTree(root->right);
    delete root;
}

void printTree(const treeNode* root, int depth = 0) {
    if (root == nullptr) return;
    printTree(root->right, depth + 1);
    for (int i = 0; i < depth; ++i) std::cout << "    ";
    std::cout << root->parcel.parcelID << '\n';
    printTree(root->left, depth + 1);
}

bool check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

treeNode* makeTree(std::initializer_list<std::string> ids) {
    treeNode* root = nullptr;
    for (const std::string& id : ids) {
        Parcel parcel = makeParcel();
        parcel.parcelID = id;
        root = treeInsert(root, parcel);
    }
    return root;
}

std::string treeIDs(const treeNode* root) {
    if (root == nullptr) return "";
    return treeIDs(root->left) + root->parcel.parcelID + " " + treeIDs(root->right);
}

bool testTreeRemove() {
    bool passed = check(treeRemove(nullptr, "PCL0001") == nullptr,
                        "removing from an empty tree returns null");

    treeNode* root = makeTree({"PCL0001"});
    root = treeRemove(root, "PCL0001");
    passed = check(root == nullptr, "removing the only node empties the tree") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001"});
    root = treeRemove(root, "PCL0003");
    passed = check(treeIDs(root) == "PCL0001 ",
                   "removing the root replaces it with its left child") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0005"});
    root = treeRemove(root, "PCL0003");
    passed = check(treeIDs(root) == "PCL0005 ",
                   "removing the root replaces it with its right child") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001", "PCL0005"});
    treeNode* originalRoot = root;
    root = treeRemove(root, "PCL0001");
    if (!check(root == originalRoot, "removing a leaf returns the unchanged root")) {
        destroyTree(originalRoot);
        return false;
    }
    passed = check(treeIDs(root) == "PCL0003 PCL0005 ",
                   "removing a leaf preserves the root and other nodes") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001", "PCL0002", "PCL0005"});
    originalRoot = root;
    root = treeRemove(root, "PCL0001");
    if (!check(root == originalRoot, "removing a right-child node returns the root")) {
        destroyTree(originalRoot);
        return false;
    }
    passed = check(treeIDs(root) == "PCL0002 PCL0003 PCL0005 " &&
                       root != nullptr && root->left != nullptr &&
                       root->left->parcel.parcelID == "PCL0002",
                   "removing a node with a right child reconnects that child") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001", "PCL0005", "PCL0004"});
    originalRoot = root;
    root = treeRemove(root, "PCL0005");
    if (!check(root == originalRoot, "removing a left-child node returns the root")) {
        destroyTree(originalRoot);
        return false;
    }
    passed = check(treeIDs(root) == "PCL0001 PCL0003 PCL0004 " &&
                       root != nullptr && root->right != nullptr &&
                       root->right->parcel.parcelID == "PCL0004",
                   "removing a node with a left child reconnects that child") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001", "PCL0005", "PCL0004", "PCL0006"});
    originalRoot = root;
    root = treeRemove(root, "PCL0003");
    if (!check(root == originalRoot, "removing a two-child root returns the root")) {
        destroyTree(originalRoot);
        return false;
    }
    passed = check(root->parcel.parcelID == "PCL0004" &&
                       treeIDs(root) == "PCL0001 PCL0004 PCL0005 PCL0006 ",
                   "removing a two-child root uses its inorder successor once") && passed;
    destroyTree(root);

    root = makeTree({"PCL0003", "PCL0001", "PCL0005"});
    originalRoot = root;
    root = treeRemove(root, "PCL0004");
    if (!check(root == originalRoot, "removing a missing ID returns the root")) {
        destroyTree(originalRoot);
        return false;
    }
    passed = check(treeIDs(root) == "PCL0001 PCL0003 PCL0005 ",
                   "removing a missing ID leaves the tree unchanged") && passed;
    destroyTree(root);

    return passed;
}

bool testTreeUpdate() {
    treeNode* root = makeTree({"PCL0003", "PCL0001", "PCL0005"});
    treeNode* key = root->left;
    Parcel replacement = makeParcel();
    replacement.parcelID = "PCL0001";
    replacement.recipientName = "Updated Recipient";
    replacement.deliveryAddress = "New Address";

    treeUpdate(root, key, replacement);
    Parcel* found = treeSearch(root, "PCL0001");
    bool passed = check(found != nullptr && found->recipientName == "Updated Recipient" &&
                            found->deliveryAddress == "New Address",
                        "updating an existing parcel replaces its details");
    passed = check(treeIDs(root) == "PCL0001 PCL0003 PCL0005 ",
                   "updating details preserves tree ordering") && passed;

    treeNode missing{makeParcel(), nullptr, nullptr};
    missing.parcel.parcelID = "PCL0002";
    treeUpdate(root, &missing, replacement);
    passed = check(treeIDs(root) == "PCL0001 PCL0003 PCL0005 " &&
                       treeSearch(root, "PCL0003")->recipientName == "Test Recipient",
                   "updating a missing parcel leaves the tree unchanged") && passed;

    replacement.parcelID = "PCL0009";
    treeUpdate(root, key, replacement);
    passed = check(treeSearch(root, "PCL0001") == found &&
                       treeSearch(root, "PCL0009") == nullptr &&
                       treeIDs(root) == "PCL0001 PCL0003 PCL0005 ",
                   "updating a parcel cannot change its tree key") && passed;

    treeUpdate(root, nullptr, replacement);
    treeUpdate(nullptr, key, replacement);
    passed = check(treeIDs(root) == "PCL0001 PCL0003 PCL0005 ",
                   "null key or empty tree leaves parcels unchanged") && passed;

    destroyTree(root);
    return passed;
}

} // namespace

int main() {
    std::ostringstream output;
    std::streambuf* original = std::cout.rdbuf(output.rdbuf());
    inorderTraversal(nullptr);
    std::cout.rdbuf(original);
    bool passed = check(output.str().empty(), "empty tree should print nothing");

    ParcelRegistry registry;
    std::string ids[5];
    for (int i = 0; i < 5; ++i) {
        passed = check(registry.registerParcel(makeParcel(), ids[i]) == ResultCode::Success,
                       "registry accepts a valid parcel") && passed;
        passed = check(ids[i] == "PCL000" + std::to_string(i + 1),
                       "registry assigns the expected ID") && passed;
    }
    passed = check(registry.getSize() == 5 && registry.waitingCount() == 5,
                   "registry contains five waiting parcels") && passed;

    // Insert registry records out of order to exercise both sides of the tree.
    // This is an explicit copy, not automatic registry/tree integration.
    treeNode* root = nullptr;
    const int insertionOrder[] = {2, 0, 4, 1, 3};
    for (int index : insertionOrder) {
        Parcel* registered = registry.findParcel(ids[index]);
        passed = check(registered != nullptr, "findParcel locates the registered ID") && passed;
        if (registered != nullptr) root = treeInsert(root, *registered);
    }

    passed = check(root != nullptr, "inserting into an empty tree creates a root") && passed;
    if (root != nullptr) {
        passed = check(root->parcel.parcelID == "PCL0003", "root keeps its parcel") && passed;
        passed = check(root->parcel.recipientName == "Test Recipient", "parcel details are copied") && passed;
        passed = check(root->left != nullptr && root->left->parcel.parcelID == "PCL0001",
                       "smaller IDs go left") && passed;
        passed = check(root->right != nullptr && root->right->parcel.parcelID == "PCL0005",
                       "larger IDs go right") && passed;
    }

    output.str("");
    output.clear();
    original = std::cout.rdbuf(output.rdbuf());
    inorderTraversal(root);
    std::cout.rdbuf(original);
    passed = check(output.str() == "PCL0001 PCL0002 PCL0003 PCL0004 PCL0005 ",
                   "inorder traversal prints IDs in sorted order") && passed;

    std::cout << "Tree built from ParcelRegistry records (larger IDs above smaller IDs):\n";
    printTree(root);
    std::cout << "Inorder: " << output.str() << '\n';

    passed = check(registry.updateParcelDetails(ids[2], "Updated Recipient", "", "")
                       == ResultCode::Success,
                   "registry can update a registered parcel") && passed;
    Parcel* updated = registry.findParcel(ids[2]);
    passed = check(updated != nullptr && updated->recipientName == "Updated Recipient",
                   "registry record reflects the update") && passed;
    if (root != nullptr) {
        passed = check(root->parcel.recipientName == "Test Recipient",
                       "tree holds a copy; registry updates do not sync automatically") && passed;
    }
    std::cout << "Note: tree records are copies; registry updates do not sync to the tree.\n";

    destroyTree(root);
    passed = testTreeRemove() && passed;
    passed = testTreeUpdate() && passed;
    if (passed) std::cout << "Tree tests passed.\n";
    return passed ? 0 : 1;
}
