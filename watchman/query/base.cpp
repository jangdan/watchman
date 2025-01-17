/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "watchman/Errors.h"
#include "watchman/watchman_query.h"

#include <memory>
#include <vector>

using namespace watchman;

/* Basic boolean and compound expressions */

class NotExpr : public QueryExpr {
  std::unique_ptr<QueryExpr> expr;

 public:
  explicit NotExpr(std::unique_ptr<QueryExpr> other_expr)
      : expr(std::move(other_expr)) {}

  EvaluateResult evaluate(QueryContext* ctx, FileResult* file) override {
    auto res = expr->evaluate(ctx, file);
    if (!res.has_value()) {
      return res;
    }
    return !*res;
  }

  static std::unique_ptr<QueryExpr> parse(
      w_query* query,
      const json_ref& term) {
    /* rigidly require ["not", expr] */
    if (!term.isArray() || json_array_size(term) != 2) {
      throw QueryParseError("must use [\"not\", expr]");
    }

    const auto& other = term.at(1);
    auto other_expr = w_query_expr_parse(query, other);
    return std::make_unique<NotExpr>(std::move(other_expr));
  }
};

W_TERM_PARSER("not", NotExpr::parse)

class TrueExpr : public QueryExpr {
 public:
  EvaluateResult evaluate(QueryContext*, FileResult*) override {
    return true;
  }

  static std::unique_ptr<QueryExpr> parse(w_query*, const json_ref&) {
    return std::make_unique<TrueExpr>();
  }
};

W_TERM_PARSER("true", TrueExpr::parse)

class FalseExpr : public QueryExpr {
 public:
  EvaluateResult evaluate(QueryContext*, FileResult*) override {
    return false;
  }

  static std::unique_ptr<QueryExpr> parse(w_query*, const json_ref&) {
    return std::make_unique<FalseExpr>();
  }
};

W_TERM_PARSER("false", FalseExpr::parse)

class ListExpr : public QueryExpr {
  bool allof;
  std::vector<std::unique_ptr<QueryExpr>> exprs;

 public:
  ListExpr(bool isAll, std::vector<std::unique_ptr<QueryExpr>> exprs)
      : allof(isAll), exprs(std::move(exprs)) {}

  EvaluateResult evaluate(QueryContext* ctx, FileResult* file) override {
    bool needData = false;

    for (auto& expr : exprs) {
      auto res = expr->evaluate(ctx, file);

      if (!res.has_value()) {
        needData = true;
      } else if (!*res) {
        if (allof) {
          // Return NoMatch even if we have needData set, as allof
          // requires that all terms match and this one doesn't,
          // so we can avoid loading the data for prior terms
          // in this list
          return false;
        }
      } else {
        // Matched

        if (!allof) {
          // Similar to the condition above, we can short circuit loading
          // other data if this one matches for the anyof case.
          return true;
        }
      }
    }

    if (needData) {
      // We're not sure yet
      return folly::none;
    }
    return allof;
  }

  static std::unique_ptr<QueryExpr>
  parse(w_query* query, const json_ref& term, bool allof) {
    std::vector<std::unique_ptr<QueryExpr>> list;

    /* don't allow "allof" on its own */
    if (!term.isArray() || json_array_size(term) < 2) {
      if (allof) {
        throw QueryParseError("must use [\"allof\", expr...]");
      }
      throw QueryParseError("must use [\"anyof\", expr...]");
    }

    auto n = json_array_size(term) - 1;
    list.reserve(n);

    for (size_t i = 0; i < n; i++) {
      const auto& exp = term.at(i + 1);

      auto op = allof ? AggregateOp::AllOf : AggregateOp::AnyOf;
      auto parsed = w_query_expr_parse(query, exp);
      if (list.empty()) {
        list.emplace_back(std::move(parsed));
      } else {
        // Try to aggregate with previous expression
        auto aggExpr = list.back().get()->aggregate(parsed.get(), op);
        if (aggExpr) {
          list.back() = std::move(aggExpr);
        } else {
          list.emplace_back(std::move(parsed));
        }
      }
    }

    return std::make_unique<ListExpr>(allof, std::move(list));
  }

  static std::unique_ptr<QueryExpr> parseAllOf(
      w_query* query,
      const json_ref& term) {
    return parse(query, term, true);
  }
  static std::unique_ptr<QueryExpr> parseAnyOf(
      w_query* query,
      const json_ref& term) {
    return parse(query, term, false);
  }
};

W_TERM_PARSER("anyof", ListExpr::parseAnyOf)
W_TERM_PARSER("allof", ListExpr::parseAllOf)

/* vim:ts=2:sw=2:et:
 */
