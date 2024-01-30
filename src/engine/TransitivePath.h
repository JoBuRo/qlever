// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Florian Kramer (florian.kramer@neptun.uni-freiburg.de)

#pragma once

#include <functional>
#include <memory>

#include "engine/GrbMatrix.h"
#include "engine/Operation.h"
#include "engine/QueryExecutionTree.h"
#include "engine/idTable/IdTable.h"

using TreeAndCol = std::pair<std::shared_ptr<QueryExecutionTree>, size_t>;
struct TransitivePathSide {
  // treeAndCol contains the QueryExecutionTree of this side and the column
  // where the Ids of this side are located. This member only has a value if
  // this side was bound.
  std::optional<TreeAndCol> treeAndCol_;
  // Column of the sub table where the Ids of this side are located
  size_t subCol_;
  std::variant<Id, Variable> value_;
  // The column in the ouput table where this side Ids are written to.
  // This member is set by the TransitivePath class
  size_t outputCol_ = 0;

  bool isVariable() const { return std::holds_alternative<Variable>(value_); };

  bool isBoundVariable() const { return treeAndCol_.has_value(); };

  std::string getCacheKey() const {
    std::ostringstream os;
    if (!isVariable()) {
      os << "Id: " << std::get<Id>(value_);
    }

    os << ", subColumn: " << subCol_ << "to " << outputCol_;

    if (treeAndCol_.has_value()) {
      const auto& [tree, col] = treeAndCol_.value();
      os << ", Subtree:\n";
      os << tree->getCacheKey() << "with join column " << col << "\n";
    }
    return std::move(os).str();
  }

  bool isSortedOnInputCol() const {
    if (!treeAndCol_.has_value()) {
      return false;
    }

    auto [tree, col] = treeAndCol_.value();
    const std::vector<ColumnIndex>& sortedOn =
        tree->getRootOperation()->getResultSortedOn();
    // TODO<C++23> use std::ranges::starts_with
    return (!sortedOn.empty() && sortedOn[0] == col);
  }
};

// This struct keeps track of the mapping between Ids and matrix indices
struct IdMapping {
  constexpr static auto hash = [](Id id) {
    return std::hash<uint64_t>{}(id.getBits());
  };
  std::unordered_map<Id, size_t, decltype(hash), std::equal_to<Id>> idMap_{};

  std::vector<Id> indexMap_;

  size_t nextIndex_ = 0;

  bool isContained(Id id) { return idMap_.contains(id); }

  size_t addId(Id id) {
    if (!idMap_.contains(id)) {
      idMap_.insert({id, nextIndex_});
      indexMap_.push_back(id);
      nextIndex_++;
      return nextIndex_ - 1;
    }
    return idMap_[id];
  }

  Id getId(size_t index) const { return indexMap_.at(index); }

  size_t getIndex(const Id& id) const { return idMap_.at(id); }

  size_t size() const { return indexMap_.size(); }
};

class TransitivePath : public Operation {
  // We deliberately use the `std::` variants of a hash set and hash map because
  // `absl`s types are not exception safe.
  constexpr static auto hash = [](Id id) {
    return std::hash<uint64_t>{}(id.getBits());
  };
  using Set = std::unordered_set<Id, decltype(hash), std::equal_to<Id>,
                                 ad_utility::AllocatorWithLimit<Id>>;
  using Map = std::unordered_map<
      Id, Set, decltype(hash), std::equal_to<Id>,
      ad_utility::AllocatorWithLimit<std::pair<const Id, Set>>>;

  std::shared_ptr<QueryExecutionTree> subtree_;
  TransitivePathSide lhs_;
  TransitivePathSide rhs_;
  size_t resultWidth_ = 2;
  size_t minDist_;
  size_t maxDist_;
  VariableToColumnMap variableColumns_;

 public:
  TransitivePath(QueryExecutionContext* qec,
                 std::shared_ptr<QueryExecutionTree> child,
                 TransitivePathSide leftSide, TransitivePathSide rightSide,
                 size_t minDist, size_t maxDist);

  /**
   * Returns a new TransitivePath operation that uses the fact that leftop
   * generates all possible values for the left side of the paths. If the
   * results of leftop is smaller than all possible values this will result in a
   * faster transitive path operation (as the transitive paths has to be
   * computed for fewer elements).
   */
  std::shared_ptr<TransitivePath> bindLeftSide(
      std::shared_ptr<QueryExecutionTree> leftop, size_t inputCol) const;

  /**
   * Returns a new TransitivePath operation that uses the fact that rightop
   * generates all possible values for the right side of the paths. If the
   * results of rightop is smaller than all possible values this will result in
   * a faster transitive path operation (as the transitive paths has to be
   * computed for fewer elements).
   */
  std::shared_ptr<TransitivePath> bindRightSide(
      std::shared_ptr<QueryExecutionTree> rightop, size_t inputCol) const;

  bool isBoundOrId() const;

  /**
   * Getters, mainly necessary for testing
   */
  size_t getMinDist() const { return minDist_; }
  size_t getMaxDist() const { return maxDist_; }
  const TransitivePathSide& getLeft() const { return lhs_; }
  const TransitivePathSide& getRight() const { return rhs_; }

 protected:
  virtual std::string getCacheKeyImpl() const override;

 public:
  virtual std::string getDescriptor() const override;

  virtual size_t getResultWidth() const override;

  virtual vector<ColumnIndex> resultSortedOn() const override;

  virtual void setTextLimit(size_t limit) override;

  virtual bool knownEmptyResult() override;

  virtual float getMultiplicity(size_t col) override;

 private:
  uint64_t getSizeEstimateBeforeLimit() override;

 public:
  virtual size_t getCostEstimate() override;

  vector<QueryExecutionTree*> getChildren() override {
    std::vector<QueryExecutionTree*> res;
    auto addChildren = [](std::vector<QueryExecutionTree*>& res,
                          TransitivePathSide side) {
      if (side.treeAndCol_.has_value()) {
        res.push_back(side.treeAndCol_.value().first.get());
      }
    };
    addChildren(res, lhs_);
    addChildren(res, rhs_);
    res.push_back(subtree_.get());
    return res;
  }

  /**
   * @brief Compute the transitive hull with a bound side.
   * This function is called when the startSide is bound and
   * it is a variable. The other IdTable contains the result
   * of the start side and will be used to get the start nodes.
   *
   * @tparam RES_WIDTH Number of columns of the result table
   * @tparam SUB_WIDTH Number of columns of the sub table
   * @tparam SIDE_WIDTH Number of columns of the
   * @param res The result table which will be filled in-place
   * @param sub The IdTable for the sub result
   * @param startSide The start side for the transitive hull
   * @param targetSide The target side for the transitive hull
   * @param startSideTable The IdTable of the startSide
   */
  template <size_t RES_WIDTH, size_t SUB_WIDTH, size_t SIDE_WIDTH>
  void computeTransitivePathBound(IdTable* res, const IdTable& sub,
                                  const TransitivePathSide& startSide,
                                  const TransitivePathSide& targetSide,
                                  const IdTable& startSideTable) const;

  /**
   * @brief Compute the transitive hull.
   * This function is called when no side is bound (or an id).
   *
   * @tparam RES_WIDTH Number of columns of the result table
   * @tparam SUB_WIDTH Number of columns of the sub table
   * @param res The result table which will be filled in-place
   * @param sub The IdTable for the sub result
   * @param startSide The start side for the transitive hull
   * @param targetSide The target side for the transitive hull
   */
  template <size_t RES_WIDTH, size_t SUB_WIDTH>
  void computeTransitivePath(IdTable* res, const IdTable& sub,
                             const TransitivePathSide& startSide,
                             const TransitivePathSide& targetSide) const;

 private:
  /**
   * @brief Compute the result for this TransitivePath operation
   * This function chooses the start and target side for the transitive
   * hull computation. This choice of the start side has a large impact
   * on the time it takes to compute the hull. The set of nodes on the
   * start side should be as small as possible.
   *
   * @return ResultTable The result of the TransitivePath operation
   */
  virtual ResultTable computeResult() override;

  VariableToColumnMap computeVariableToColumnMap() const override;

  // The internal implementation of `bindLeftSide` and `bindRightSide` which
  // share a lot of code.
  std::shared_ptr<TransitivePath> bindLeftOrRightSide(
      std::shared_ptr<QueryExecutionTree> leftOrRightOp, size_t inputCol,
      bool isLeft) const;

  /**
   * @brief Compute the transitive hull of the graph. If given startNodes,
   * compute the transitive hull starting at the startNodes.
   *
   * @param graph Boolean, square, sparse, adjacency matrix. Row i, column j is
   * true, iff. there is an edge going from i to j in the graph.
   * @param startNodes Boolean, sparse, adjacency matrix, marking the start
   * nodes. There is one row for each start node. The number of columns has to
   * be equal to the number of columns of the graph matrix.
   * @return An adjacency matrix containing the transitive hull
   */
  GrbMatrix transitiveHull(const GrbMatrix& graph,
                           std::optional<GrbMatrix> startNodes) const;

  /**
   * @brief Fill the given table with the transitive hull and use the
   * startSideTable to fill in the rest of the columns.
   * This function is called if the start side is bound and a variable.
   *
   * @tparam WIDTH The number of columns of the result table.
   * @tparam START_WIDTH The number of columns of the start table.
   * @param table The result table which will be filled.
   * @param hull The transitive hull.
   * @param nodes The start nodes of the transitive hull. These need to be in
   * the same order and amount as the starting side nodes in the startTable.
   * @param startSideCol The column of the result table for the startSide of the
   * hull
   * @param targetSideCol The column of the result table for the targetSide of
   * the hull
   * @param startSideTable An IdTable that holds other results. The other
   * results will be transferred to the new result table.
   * @param skipCol This column contains the Ids of the start side in the
   * startSideTable and will be skipped.
   */

  /**
   * @brief Fill the given table with the transitive hull.
   * This function is called if the sides are unbound or ids.
   *
   * @tparam WIDTH The number of columns of the result table.
   * @param table The result table which will be filled.
   * @param hull The transitive hull.
   * @param startSideCol The column of the result table for the startSide of the
   * hull
   * @param targetSideCol The column of the result table for the targetSide of
   * the hull
   */

  /**
   * @brief Fill the IdTable with the given transitive hull.
   *
   * @tparam WIDTH The number of columns of the result table.
   * @param table The result table which will be filled.
   * @param hull The transitive hull. Represented by a sparse, boolean adjacency
   * matrix
   * @param mapping IdMapping, which maps Ids to matrix indices and vice versa.
   * @param startSideCol The column of the result table for the startSide of the
   * hull
   * @param targetSideCol The column of the result table for the targetSide of
   * the hull
   */
  template <size_t WIDTH>
  static void fillTableWithHull(IdTableStatic<WIDTH>& table,
                                const GrbMatrix& hull, const IdMapping& mapping,
                                size_t startSideCol, size_t targetSideCol);

  /**
   * @brief Fill the IdTable with the given transitive hull. This function is
   * used in case the hull computation has one (or more) Ids as start nodes.
   *
   * @tparam WIDTH The number of columns of the result table.
   * @param table The result table which will be filled.
   * @param hull The transitive hull. Represented by a sparse, boolean adjacency
   * matrix
   * @param mapping IdMapping, which maps Ids to matrix indices and vice versa.
   * @param startNodes Ids of the start nodes.
   * @param startSideCol The column of the result table for the startSide of the
   * hull
   * @param targetSideCol The column of the result table for the targetSide of
   * the hull
   */
  template <size_t WIDTH>
  static void fillTableWithHull(IdTableStatic<WIDTH>& table,
                                const GrbMatrix& hull, const IdMapping& mapping,
                                std::span<const Id> startNodes,
                                size_t startSideCol, size_t targetSideCol);

  /**
   * @brief Fill the IdTable with the given transitive hull. This function is
   * used if the start side was already bound and there is an IdTable from which
   * data has to be copied to the result table.
   *
   * @tparam WIDTH The number of columns of the result table.
   * @tparam START_WIDTH The number of columns of the start table.
   * @param table The result table which will be filled.
   * @param hull The transitive hull. Represented by a sparse, boolean adjacency
   * matrix
   * @param mapping IdMapping, which maps Ids to matrix indices and vice versa.
   * @param startNodes Ids of the start nodes.
   * @param startSideCol The column of the result table for the startSide of the
   * hull
   * @param targetSideCol The column of the result table for the targetSide of
   * the hull
   * @param skipCol This column contains the Ids of the start side in the
   * startSideTable and will be skipped.
   */
  template <size_t WIDTH, size_t START_WIDTH>
  static void fillTableWithHull(IdTableStatic<WIDTH>& table,
                                const GrbMatrix& hull, const IdMapping& mapping,
                                const IdTable& startSideTable,
                                std::span<const Id> startNodes,
                                size_t startSideCol, size_t targetSideCol,
                                size_t skipCol);

  GrbMatrix getTargetRow(GrbMatrix& hull, size_t targetIndex) const;

  /**
   * @brief Create a boolean, sparse adjacency matrix from the given edges. The
   * edges are given as lists, where one list contains the start node of the
   * edge and the other list contains the target node of the edge.
   * Also create an IdMapping, which maps the given Ids to matrix indices.
   *
   * @param startCol Column from the IdTable, which contains edge start nodes
   * @param targetCol Column from the IdTable, which contains edge target nodes
   * @param numRows Number of rows in the IdTable
   */
  std::tuple<GrbMatrix, IdMapping> setupMatrix(std::span<const Id> startCol,
                                               std::span<const Id> targetCol,
                                               size_t numRows) const;

  /**
   * @brief Create a boolean, sparse, adjacency matrix which holds the starting
   * nodes for the transitive hull computation.
   *
   * @param startIds List of Ids where the transitive hull computation should
   * start
   * @param numRows Number of rows in the IdTable where startIds comes from
   * @param mapping An IdMapping between Ids and matrix indices
   * @return Matrix with one row for each start node
   */
  GrbMatrix setupStartNodeMatrix(std::span<const Id> startIds, size_t numRows,
                                 IdMapping mapping) const;

  // Copy the columns from the input table to the output table
  template <size_t INPUT_WIDTH, size_t OUTPUT_WIDTH>
  static void copyColumns(const IdTableView<INPUT_WIDTH>& inputTable,
                          IdTableStatic<OUTPUT_WIDTH>& outputTable,
                          size_t inputRow, size_t outputRow, size_t skipCol);
};
