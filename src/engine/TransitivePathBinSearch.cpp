// Copyright 2024, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Herrmann (johannes.r.herrmann(at)gmail.com)

#include "TransitivePathBinSearch.h"

#include <memory>
#include <optional>
#include <utility>

#include "engine/CallFixedSize.h"
#include "engine/TransitivePathBase.h"

// _____________________________________________________________________________
TransitivePathBinSearch::TransitivePathBinSearch(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> child,
    TransitivePathSide leftSide, TransitivePathSide rightSide, size_t minDist,
    size_t maxDist)
    : TransitivePathImpl<BinSearchMap>(qec, std::move(child),
                                       std::move(leftSide),
                                       std::move(rightSide), minDist, maxDist) {
  auto [startSide, targetSide] = decideDirection();
  subtree_ = QueryExecutionTree::createSortedTree(
      subtree_, {startSide.subCol_, targetSide.subCol_});
}

// _____________________________________________________________________________
BinSearchMap TransitivePathBinSearch::setupEdgesMap(
    const IdTable& dynSub, const TransitivePathSide& startSide,
    const TransitivePathSide& targetSide) const {
  return BinSearchMap(dynSub.getColumn(startSide.subCol_),
                      dynSub.getColumn(targetSide.subCol_));
}
