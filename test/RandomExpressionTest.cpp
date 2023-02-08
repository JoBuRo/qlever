//  Copyright 2023, University of Freiburg,
//                  Chair of Algorithms and Data Structures.
//  Author: Johannes Kalmbach <kalmbach@cs.uni-freiburg.de>

#include "./SparqlExpressionTestHelpers.h"
#include "engine/sparqlExpressions/RandomExpression.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace sparqlExpression;
TEST(RandomExpression, evaluate) {
  TestContext testContext{};
  auto& evaluationContext = testContext.context;
  evaluationContext._beginIndex = 43;
  evaluationContext._endIndex = 1044;
  auto resultAsVariant = RandomExpression{}.evaluate(&evaluationContext);

  using V = VectorWithMemoryLimit<int64_t>;
  ASSERT_TRUE(std::holds_alternative<V>(resultAsVariant));
  const auto& resultVector = std::get<V>(resultAsVariant);
  ASSERT_EQ(resultVector.size(), 1001);

  ad_utility::HashMap<int64_t, size_t> histogram;
  for (auto rand : resultVector) {
    histogram[rand % 10]++;
  }

  // A simple check whether the numbers are sufficiently random. It has a
  // very low percentage of failure.
  for (const auto& [rand, count] : histogram) {
    ASSERT_GE(count, 10);
    ASSERT_LE(count, 200);
  }

  size_t numSwaps = 0;
  for (size_t i = 1; i < resultVector.size(); ++i) {
    numSwaps += resultVector[i] < resultVector[i - 1];
  }
  ASSERT_GE(numSwaps, 100);
  ASSERT_LE(numSwaps, 900);
}

TEST(RandomExpression, simpleMemberFunctions) {
  RandomExpression expr;
  ASSERT_TRUE(expr.getUnaggregatedVariables().empty());
  auto cacheKey = expr.getCacheKey({});
  ASSERT_THAT(cacheKey, ::testing::StartsWith("RAND "));
  ASSERT_EQ(cacheKey, expr.getCacheKey({}));
  // Note: Since the cache key is sampled randomly, the following test has a
  // probability of `1 / 2^64` of a spurious failure.
  ASSERT_NE(cacheKey, RandomExpression{}.getCacheKey({}));
}
