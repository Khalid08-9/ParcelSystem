#include <iostream>
#include <initializer_list>
#include <sstream>
#include <string>

#include "ParcelRegistry.hpp"
#include "ParcelTree.hpp"

// The registry implementations are included here so the test target can exercise
// the existing integration example without adding another executable.
#include "../src/ParcelQueue.cpp"
#include "../src/Status.cpp"
#include "../src/ParcelRegistry.cpp"

namespace {

Parcel makeParcel(const std::string& id = "") {
    Parcel parcel{};
    parcel.parcelID = id;
    parcel.recipientName = "Test Recipient";
    parcel.recipientID = "RC012345";
    parcel.deliveryAddress = "Campus";
    parcel.type = ParcelType::Document;
    return parcel;
}

bool check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

treeNode* makeTree(std::initializer_list<std::string> ids) {
    treeNode* root = nullptr;
    for (const std::string& id : ids) {
        if (insertParcelRecord(root, makeParcel(id)) != ResultCode::Success)
            std::cerr << "FAIL: inserting test parcel " << id << '\n';
    }
    return root;
}

std::string displayedIDs(const treeNode* root, ResultCode& result) {
    std::ostringstream output;
    std::streambuf* original = std::cout.rdbuf(output.rdbuf());
    result = displayParcelRecords(root);
    std::cout.rdbuf(original);
    return output.str();
}

bool testInsertAndSearch() {
    treeNode* root = nullptr;
    ResultCode displayResult;
    bool passed = check(displayedIDs(root, displayResult).empty() &&
                        displayResult == ResultCode::Empty,
                        "empty tree has no records to display");

    passed = check(insertParcelRecord(root, makeParcel(" \t")) == ResultCode::InvalidInput &&
                   root == nullptr, "blank IDs are rejected") && passed;
    Parcel incomplete = makeParcel("PCL0001");
    incomplete.recipientName = "  ";
    passed = check(insertParcelRecord(root, incomplete) == ResultCode::InvalidInput &&
                   root == nullptr, "incomplete parcels are rejected") && passed;

    passed = check(insertParcelRecord(root, makeParcel("PCL0002")) == ResultCode::Success &&
                   insertParcelRecord(root, makeParcel("PCL0001")) == ResultCode::Success &&
                   insertParcelRecord(root, makeParcel("PCL0003")) == ResultCode::Success,
                   "valid parcels can be inserted") && passed;
    passed = check(insertParcelRecord(root, makeParcel("PCL0002")) == ResultCode::Duplicate,
                   "duplicate IDs are rejected") && passed;
    passed = check(displayedIDs(root, displayResult) == "PCL0001 PCL0002 PCL0003 " &&
                   displayResult == ResultCode::Success,
                   "inorder traversal displays each ID once in sorted order") && passed;

    const Parcel* found = nullptr;
    passed = check(findParcelRecord(root, "PCL0001", found) == ResultCode::Success &&
                   found != nullptr && found->recipientName == "Test Recipient",
                   "search returns the requested parcel") && passed;
    passed = check(findParcelRecord(root, "PCL0004", found) == ResultCode::NotFound &&
                   found == nullptr, "missing search clears the result") && passed;
    passed = check(findParcelRecord(root, " \t", found) == ResultCode::InvalidInput &&
                   found == nullptr, "blank search ID is invalid") && passed;

    clearParcelRecords(root);
    passed = check(root == nullptr, "clearing the tree resets the root") && passed;
    clearParcelRecords(root);
    return passed;
}

bool testRemovalCase(std::initializer_list<std::string> ids,
                     const std::string& id, const std::string& expected,
                     const std::string& message) {
    treeNode* root = makeTree(ids);
    ResultCode displayResult;
    bool passed = check(removeParcelRecord(root, id) == ResultCode::Success &&
                        displayedIDs(root, displayResult) == expected,
                        message);
    clearParcelRecords(root);
    return passed;
}

bool testTreeRemove() {
    treeNode* root = nullptr;
    bool passed = check(removeParcelRecord(root, "PCL0001") == ResultCode::NotFound &&
                        root == nullptr, "removing from an empty tree reports NotFound");
    passed = testRemovalCase({"PCL0001"}, "PCL0001", "",
                "removing the only node empties the tree") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0001"}, "PCL0003", "PCL0001 ",
                "removing a root with only a left child") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0005"}, "PCL0003", "PCL0005 ",
                "removing a root with only a right child") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0001", "PCL0005"}, "PCL0001",
                "PCL0003 PCL0005 ", "removing a leaf") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0001", "PCL0002", "PCL0005"},
                "PCL0001", "PCL0002 PCL0003 PCL0005 ",
                "removing a node with a right child") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0001", "PCL0005", "PCL0004"},
                "PCL0005", "PCL0001 PCL0003 PCL0004 ",
                "removing a node with a left child") && passed;
    passed = testRemovalCase({"PCL0003", "PCL0001", "PCL0005", "PCL0004", "PCL0006"},
                "PCL0003", "PCL0001 PCL0004 PCL0005 PCL0006 ",
                "removing a two-child root uses its inorder successor") && passed;

    root = makeTree({"PCL0003", "PCL0001", "PCL0005"});
    ResultCode displayResult;
    passed = check(removeParcelRecord(root, "PCL0004") == ResultCode::NotFound &&
                   removeParcelRecord(root, "  ") == ResultCode::InvalidInput &&
                   displayedIDs(root, displayResult) == "PCL0001 PCL0003 PCL0005 ",
                   "invalid removal leaves the tree unchanged") && passed;
    clearParcelRecords(root);
    return passed;
}

bool testTreeUpdate() {
    treeNode* root = makeTree({"PCL0003", "PCL0001", "PCL0005"});
    Parcel replacement = makeParcel("PCL0001");
    replacement.recipientName = "Updated Recipient";
    replacement.deliveryAddress = "New Address";

    bool passed = check(updateParcelRecord(root, "PCL0001", replacement) == ResultCode::Success,
                        "updating an existing parcel succeeds");
    const Parcel* found = nullptr;
    passed = check(findParcelRecord(root, "PCL0001", found) == ResultCode::Success &&
                   found != nullptr && found->recipientName == "Updated Recipient" &&
                   found->deliveryAddress == "New Address",
                   "updating an existing parcel replaces its details") && passed;

    replacement.parcelID = "PCL0002";
    passed = check(updateParcelRecord(root, "PCL0002", replacement) == ResultCode::NotFound,
                   "updating a missing parcel reports NotFound") && passed;
    replacement.parcelID = "PCL0009";
    passed = check(updateParcelRecord(root, "PCL0001", replacement) == ResultCode::InvalidInput,
                   "updating cannot change the tree key") && passed;
    replacement.parcelID = "PCL0001";
    replacement.deliveryAddress = "  ";
    passed = check(updateParcelRecord(root, "PCL0001", replacement) == ResultCode::InvalidInput,
                   "updating cannot erase required details") && passed;
    replacement.deliveryAddress = "New Address";
    passed = check(updateParcelRecord(root, " ", replacement) == ResultCode::InvalidInput &&
                   updateParcelRecord(nullptr, "PCL0001", replacement) == ResultCode::NotFound,
                   "blank ID and empty tree are handled") && passed;

    ResultCode displayResult;
    passed = check(displayedIDs(root, displayResult) == "PCL0001 PCL0003 PCL0005 " &&
                   findParcelRecord(root, "PCL0001", found) == ResultCode::Success &&
                   found->recipientName == "Updated Recipient",
                   "rejected updates leave the tree unchanged") && passed;
    clearParcelRecords(root);
    return passed;
}

bool testRegistryCopy() {
    ParcelRegistry registry;
    treeNode* root = nullptr;
    std::string ids[5];
    bool passed = true;
    for (int i = 0; i < 5; ++i) {
        passed = check(registry.registerParcel(makeParcel(), ids[i]) == ResultCode::Success,
                       "registry accepts a valid parcel") && passed;
    }
    const int insertionOrder[] = {2, 0, 4, 1, 3};
    for (int index : insertionOrder) {
        Parcel* registered = registry.findParcel(ids[index]);
        passed = check(registered != nullptr &&
                       insertParcelRecord(root, *registered) == ResultCode::Success,
                       "tree accepts a copy of a registered parcel") && passed;
    }
    ResultCode displayResult;
    passed = check(displayedIDs(root, displayResult) ==
                   "PCL0001 PCL0002 PCL0003 PCL0004 PCL0005 ",
                   "registry records inserted out of order display in order") && passed;

    passed = check(registry.updateParcelDetails(ids[2], "Updated Recipient", "", "")
                   == ResultCode::Success, "registry details can be updated") && passed;
    const Parcel* treeCopy = nullptr;
    passed = check(findParcelRecord(root, ids[2], treeCopy) == ResultCode::Success &&
                   treeCopy != nullptr && treeCopy->recipientName == "Test Recipient",
                   "tree copies do not automatically sync with registry updates") && passed;
    clearParcelRecords(root);
    return passed;
}

} // namespace

int main() {
    bool passed = testInsertAndSearch();
    passed = testTreeRemove() && passed;
    passed = testTreeUpdate() && passed;
    passed = testRegistryCopy() && passed;
    if (passed) std::cout << "Tree tests passed.\n";
    return passed ? 0 : 1;
}
