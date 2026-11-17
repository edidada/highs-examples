#include "highs_data_interface.h"

#include <cstddef>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Highs.h"

namespace hdi {
namespace {

double parseDouble(const std::string& token, const std::string& key, int line) {
  if (token == "inf" || token == "+inf")
    return std::numeric_limits<double>::infinity();
  if (token == "-inf") return -std::numeric_limits<double>::infinity();
  try {
    std::size_t used = 0;
    const double value = std::stod(token, &used);
    if (used != token.size()) throw std::invalid_argument("trailing characters");
    return value;
  } catch (const std::exception&) {
    throw std::runtime_error("bad number '" + token + "' on line " +
                             std::to_string(line) + " (key " + key + ")");
  }
}

int parseInt(const std::string& token, const std::string& key, int line) {
  try {
    std::size_t used = 0;
    const long value = std::stol(token, &used);
    if (used != token.size()) throw std::invalid_argument("trailing characters");
    return static_cast<int>(value);
  } catch (const std::exception&) {
    throw std::runtime_error("bad integer '" + token + "' on line " +
                             std::to_string(line) + " (key " + key + ")");
  }
}

std::vector<double> parseDoubles(std::istringstream& in,
                                 const std::string& key, int line) {
  std::vector<double> out;
  std::string token;
  while (in >> token) out.push_back(parseDouble(token, key, line));
  return out;
}

std::vector<int> parseInts(std::istringstream& in, const std::string& key,
                           int line) {
  std::vector<int> out;
  std::string token;
  while (in >> token) out.push_back(parseInt(token, key, line));
  return out;
}

}  // namespace

bool LpData::validate(std::string* error) const {
  const auto bad = [error](const std::string& message) -> bool {
    if (error) *error = message;
    return false;
  };
  if (num_col <= 0 || num_row <= 0)
    return bad("num_col and num_row must be positive");

  const auto sized = [&](const std::vector<double>& v, const char* name,
                         int expected) -> bool {
    if (v.size() != static_cast<std::size_t>(expected))
      return bad(std::string(name) + " size mismatch");
    return true;
  };
  if (!sized(col_cost, "col_cost", num_col)) return false;
  if (!sized(col_lower, "col_lower", num_col)) return false;
  if (!sized(col_upper, "col_upper", num_col)) return false;
  if (!sized(row_lower, "row_lower", num_row)) return false;
  if (!sized(row_upper, "row_upper", num_row)) return false;

  if (a_start.size() != static_cast<std::size_t>(num_col) + 1)
    return bad("a_start size must be num_col + 1");
  if (a_start.front() != 0) return bad("a_start must begin with 0");
  if (a_start.back() != static_cast<int>(a_value.size()))
    return bad("a_start.back() must equal the number of nonzeros");
  if (a_index.size() != a_value.size())
    return bad("a_index and a_value size mismatch");
  for (std::size_t k = 0; k + 1 < a_start.size(); ++k)
    if (a_start[k] > a_start[k + 1]) return bad("a_start is not monotone");
  for (int row : a_index)
    if (row < 0 || row >= num_row) return bad("a_index out of range");
  return true;
}

LpData readLpData(const std::string& path) {
  std::ifstream file(path);
  if (!file) throw std::runtime_error("cannot open data file: " + path);

  LpData data;
  std::string line;
  int line_no = 0;
  while (std::getline(file, line)) {
    ++line_no;
    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) line.erase(comment);
    std::istringstream in(line);
    std::string key;
    if (!(in >> key)) continue;

    if (key == "sense") {
      std::string value;
      in >> value;
      if (value == "maximize" || value == "max")
        data.maximize = true;
      else if (value == "minimize" || value == "min")
        data.maximize = false;
      else
        throw std::runtime_error("bad sense on line " + std::to_string(line_no));
    } else if (key == "num_col") {
      std::string token;
      if (in >> token) data.num_col = parseInt(token, key, line_no);
    } else if (key == "num_row") {
      std::string token;
      if (in >> token) data.num_row = parseInt(token, key, line_no);
    } else if (key == "col_cost") {
      data.col_cost = parseDoubles(in, key, line_no);
    } else if (key == "col_lower") {
      data.col_lower = parseDoubles(in, key, line_no);
    } else if (key == "col_upper") {
      data.col_upper = parseDoubles(in, key, line_no);
    } else if (key == "row_lower") {
      data.row_lower = parseDoubles(in, key, line_no);
    } else if (key == "row_upper") {
      data.row_upper = parseDoubles(in, key, line_no);
    } else if (key == "a_start") {
      data.a_start = parseInts(in, key, line_no);
    } else if (key == "a_index") {
      data.a_index = parseInts(in, key, line_no);
    } else if (key == "a_value") {
      data.a_value = parseDoubles(in, key, line_no);
    } else {
      throw std::runtime_error("unknown key '" + key + "' on line " +
                               std::to_string(line_no));
    }
  }

  std::string error;
  if (!data.validate(&error))
    throw std::runtime_error("invalid LP data: " + error);
  return data;
}

SolveResult solve(const LpData& data) {
  std::string error;
  if (!data.validate(&error))
    throw std::invalid_argument("invalid LP data: " + error);

  HighsLp lp;
  lp.num_col_ = data.num_col;
  lp.num_row_ = data.num_row;
  lp.col_cost_ = data.col_cost;
  lp.col_lower_ = data.col_lower;
  lp.col_upper_ = data.col_upper;
  lp.row_lower_ = data.row_lower;
  lp.row_upper_ = data.row_upper;
  lp.sense_ = data.maximize ? ObjSense::kMaximize : ObjSense::kMinimize;
  lp.a_matrix_.format_ = MatrixFormat::kColwise;
  lp.a_matrix_.num_col_ = data.num_col;
  lp.a_matrix_.num_row_ = data.num_row;
  lp.a_matrix_.start_.assign(data.a_start.begin(), data.a_start.end());
  lp.a_matrix_.index_.assign(data.a_index.begin(), data.a_index.end());
  lp.a_matrix_.value_ = data.a_value;

  Highs highs;
  highs.setOptionValue("output_flag", false);

  SolveResult result;
  if (highs.passModel(std::move(lp)) == HighsStatus::kError) {
    result.status = "load_error";
    return result;
  }
  const HighsStatus run_status = highs.run();
  const HighsModelStatus model_status = highs.getModelStatus();
  result.status = highs.modelStatusToString(model_status);
  result.optimal = model_status == HighsModelStatus::kOptimal &&
                   run_status != HighsStatus::kError;
  if (!result.optimal) return result;

  const HighsSolution& solution = highs.getSolution();
  result.col_value = solution.col_value;
  result.row_value = solution.row_value;

  // Compute the objective from the primal solution and the original costs,
  // so it is expressed in the caller's sense for both min and max problems.
  result.objective = 0.0;
  if (result.col_value.size() == data.col_cost.size()) {
    for (std::size_t j = 0; j < data.col_cost.size(); ++j)
      result.objective += data.col_cost[j] * result.col_value[j];
  }
  return result;
}

}  // namespace hdi
