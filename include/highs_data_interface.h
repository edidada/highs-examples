#pragma once

#include <string>
#include <vector>

namespace hdi {

// Plain, solver-independent description of a linear program.
// The interface owns its buffers so callers never need HiGHS types.
struct LpData {
  bool maximize = false;
  int num_col = 0;
  int num_row = 0;
  std::vector<double> col_cost;
  std::vector<double> col_lower;
  std::vector<double> col_upper;
  std::vector<double> row_lower;
  std::vector<double> row_upper;

  // Column-wise (CSC) constraint matrix: column j owns the entries
  // [a_start[j], a_start[j + 1]) of a_index / a_value.
  std::vector<int> a_start;
  std::vector<int> a_index;
  std::vector<double> a_value;

  bool validate(std::string* error = nullptr) const;
};

struct SolveResult {
  bool optimal = false;
  std::string status;
  double objective = 0.0;
  std::vector<double> col_value;
  std::vector<double> row_value;
};

// Text data format (one key per line, '#' starts a comment):
//   sense minimize|maximize
//   num_col N
//   num_row M
//   col_cost   <N doubles>
//   col_lower  <N doubles>
//   col_upper  <N doubles>
//   row_lower  <M doubles>
//   row_upper  <M doubles>
//   a_start    <N+1 ints>
//   a_index    <nnz ints>
//   a_value    <nnz doubles>
// The tokens inf / +inf / -inf are accepted. See data/example.lpdat.
// Throws std::runtime_error when the file is missing or malformed.
LpData readLpData(const std::string& path);

// Solves `data` with HiGHS as the underlying solver.
// Throws std::invalid_argument when `data` fails validation.
SolveResult solve(const LpData& data);

}  // namespace hdi
