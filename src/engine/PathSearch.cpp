// Copyright 2024, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Herrmann (johannes.r.herrmann(at)gmail.com)

#include "PathSearch.h"

#include <boost/graph/detail/adjacency_list.hpp>
#include <functional>
#include <memory>
#include <sstream>

#include "engine/CallFixedSize.h"
#include "util/Exception.h"

// _____________________________________________________________________________
PathSearch::PathSearch(QueryExecutionContext* qec,
                       std::shared_ptr<QueryExecutionTree> subtree,
                       PathSearchConfiguration config)
    : Operation(qec),
      subtree_(std::move(subtree)),
      graph_(),
      config_(std::move(config)),
      indexToId_(),
      idToIndex_(allocator()) {
  AD_CORRECTNESS_CHECK(qec != nullptr);
  resultWidth_ = 4 + config_.edgePropertyIndices_.size();
}

// _____________________________________________________________________________
std::vector<QueryExecutionTree*> PathSearch::getChildren() {
  std::vector<QueryExecutionTree*> res;
  res.push_back(subtree_.get());
  return res;
};

// _____________________________________________________________________________
std::string PathSearch::getCacheKeyImpl() const {
  std::ostringstream os;
  AD_CORRECTNESS_CHECK(subtree_);
  os << "Subtree:\n" << subtree_->getCacheKey() << '\n';
  return std::move(os).str();
};

// _____________________________________________________________________________
string PathSearch::getDescriptor() const {
  std::ostringstream os;
  os << "PathSearch";
  return std::move(os).str();
};

// _____________________________________________________________________________
size_t PathSearch::getResultWidth() const { return resultWidth_; };

// _____________________________________________________________________________
size_t PathSearch::getCostEstimate() {
  // TODO: Figure out a smart way to estimate cost
  return 1000;
};

// _____________________________________________________________________________
uint64_t PathSearch::getSizeEstimateBeforeLimit() {
  // TODO: Figure out a smart way to estimate size
  return 1000;
};

// _____________________________________________________________________________
float PathSearch::getMultiplicity(size_t col) {
  (void)col;
  return 1;
};

// _____________________________________________________________________________
bool PathSearch::knownEmptyResult() { return subtree_->knownEmptyResult(); };

// _____________________________________________________________________________
vector<ColumnIndex> PathSearch::resultSortedOn() const { return {}; };

// _____________________________________________________________________________
ResultTable PathSearch::computeResult() {
  shared_ptr<const ResultTable> subRes = subtree_->getResult();
  IdTable idTable{allocator()};
  idTable.setNumColumns(getResultWidth());

  const IdTable& dynSub = subRes->idTable();

  std::vector<std::span<const Id>> edgePropertyLists;
  for (auto edgePropertyIndex : config_.edgePropertyIndices_) {
    edgePropertyLists.push_back(dynSub.getColumn(edgePropertyIndex));
  }

  buildGraph(dynSub.getColumn(config_.startColumn_),
             dynSub.getColumn(config_.endColumn_), edgePropertyLists);

  auto paths = findPaths();

  CALL_FIXED_SIZE(std::array{getResultWidth()}, &PathSearch::pathsToResultTable,
                  this, idTable, paths);

  return {std::move(idTable), resultSortedOn(), subRes->getSharedLocalVocab()};
};

// _____________________________________________________________________________
VariableToColumnMap PathSearch::computeVariableToColumnMap() const {
  return variableColumns_;
};

// _____________________________________________________________________________
void PathSearch::buildMapping(std::span<const Id> startNodes,
                              std::span<const Id> endNodes) {
  auto addNode = [this](const Id node) {
    if (idToIndex_.find(node) == idToIndex_.end()) {
      idToIndex_[node] = indexToId_.size();
      indexToId_.push_back(node);
    }
  };
  for (size_t i = 0; i < startNodes.size(); i++) {
    addNode(startNodes[i]);
    addNode(endNodes[i]);
  }
}

// _____________________________________________________________________________
void PathSearch::buildGraph(std::span<const Id> startNodes,
                            std::span<const Id> endNodes,
                            std::span<std::span<const Id>> edgePropertyLists) {
  AD_CORRECTNESS_CHECK(startNodes.size() == endNodes.size());
  buildMapping(startNodes, endNodes);

  while (boost::num_vertices(graph_) < indexToId_.size()) {
    boost::add_vertex(graph_);
  }

  for (size_t i = 0; i < startNodes.size(); i++) {
    auto startIndex = idToIndex_[startNodes[i]];
    auto endIndex = idToIndex_[endNodes[i]];

    std::vector<Id> edgeProperties;
    for (size_t j = 0; j < edgePropertyLists.size(); j++) {
      edgeProperties.push_back(edgePropertyLists[j][i]);
    }

    Edge edge{startNodes[i].getBits(), endNodes[i].getBits(), edgeProperties};
    boost::add_edge(startIndex, endIndex, edge, graph_);
  }
}

// _____________________________________________________________________________
std::vector<Path> PathSearch::findPaths() const {
  switch (config_.algorithm_) {
    case ALL_PATHS:
      return allPaths();
    case SHORTEST_PATHS:
      return shortestPaths();
    default:
      AD_FAIL();
  }
}

// _____________________________________________________________________________
std::vector<Path> PathSearch::allPaths() const {
  std::vector<Path> paths;
  Path path;
  auto startIndex = idToIndex_.at(config_.source_);

  std::unordered_set<uint64_t> targets;
  for (auto target : config_.targets_) {
    targets.insert(target.getBits());
  }

  AllPathsVisitor vis(targets, path, paths, indexToId_);
  boost::depth_first_search(graph_,
                            boost::visitor(vis).root_vertex(startIndex));
  return paths;
}

// _____________________________________________________________________________
std::vector<Path> PathSearch::shortestPaths() const {
  std::vector<Path> paths;
  Path path;
  auto startIndex = idToIndex_.at(config_.source_);

  std::unordered_set<uint64_t> targets;
  for (auto target : config_.targets_) {
    targets.insert(target.getBits());
  }
  std::vector<VertexDescriptor> predecessors(indexToId_.size());
  std::vector<double> distances(indexToId_.size(),
                                std::numeric_limits<double>::max());

  DijkstraAllPathsVisitor vis(startIndex, targets, path, paths, predecessors,
                              distances);

  auto weight_map = get(&Edge::weight_, graph_);

  boost::dijkstra_shortest_paths(
      graph_, startIndex,
      boost::visitor(vis)
          .weight_map(weight_map)
          .predecessor_map(predecessors.data())
          .distance_map(distances.data())
          .distance_compare(std::less_equal<double>()));
  return paths;
}

// _____________________________________________________________________________
template <size_t WIDTH>
void PathSearch::pathsToResultTable(IdTable& tableDyn,
                                    std::vector<Path>& paths) const {
  IdTableStatic<WIDTH> table = std::move(tableDyn).toStatic<WIDTH>();

  size_t rowIndex = 0;
  for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
    auto path = paths[pathIndex];
    for (size_t edgeIndex = 0; edgeIndex < path.size(); edgeIndex++) {
      auto edge = path.edges_[edgeIndex];
      auto [start, end] = edge.toIds();
      table.emplace_back();
      table(rowIndex, config_.startColumn_) = start;
      table(rowIndex, config_.endColumn_) = end;
      table(rowIndex, config_.pathIndexColumn_) = Id::makeFromInt(pathIndex);
      table(rowIndex, config_.edgeIndexColumn_) = Id::makeFromInt(edgeIndex);

      for (size_t edgePropertyIndex = 0;
           edgePropertyIndex < edge.edgeProperties_.size();
           edgePropertyIndex++) {
        table(rowIndex, 4 + edgePropertyIndex) =
            edge.edgeProperties_[edgePropertyIndex];
      }

      rowIndex++;
    }
  }

  tableDyn = std::move(table).toDynamic();
}
