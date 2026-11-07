#ifndef PARCEL_TREE_HPP
#define PARCEL_TREE_HPP

#include <string>
#include "Parcel.hpp"
#include "Result.hpp"

struct treeNode;

ResultCode insertParcelRecord(treeNode*& root, const Parcel& parcel);
ResultCode findParcelRecord(const treeNode* root, const std::string& id, const Parcel*& found);
ResultCode displayParcelRecords(const treeNode* root);
ResultCode removeParcelRecord(treeNode*& root, const std::string& id);
ResultCode updateParcelRecord(treeNode* root, const std::string& id, const Parcel& replacement);
// Rebalance existing nodes without invalidating parcel pointers. Empty trees return Empty.
ResultCode balanceParcelRecords(treeNode*& root);
void clearParcelRecords(treeNode*& root);
#endif
