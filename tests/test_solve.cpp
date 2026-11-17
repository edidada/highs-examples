#include "highs_data_interface.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: hdi_test <data-file>\n";
    return 2;
  }
  const double inf = std::numeric_limits<double>::infinity();

  // 1) Text data interface -> HiGHS.
  //    max 3x + 2y, 2x+y<=100, x+y<=80, x<=40, x,y>=0  =>  180 at (20,60).
  const hdi::LpData data = hdi::readLpData(argv[1]);
  check(data.maximize, "sense is maximize");
  check(data.num_col == 2, "num_col == 2");
  check(data.num_row == 3, "num_row == 3");
  check(data.a_start.size() == 3 && data.a_start.back() == 5,
        "CSC start has num_col+1 entries and 5 nonzeros");

  const hdi::SolveResult result = hdi::solve(data);
  check(result.optimal, "model status is optimal");
  check(!result.status.empty(), "status string is reported");
  check(near(result.objective, 180.0), "objective == 180");
  check(result.col_value.size() == 2, "two primal column values");
  if (result.col_value.size() == 2) {
    check(near(result.col_value[0], 20.0), "x == 20");
    check(near(result.col_value[1], 60.0), "y == 60");
  }
  check(result.row_value.size() == 3, "three primal row values");

  // 2) Programmatic data interface (minimization).
  //    min x + 2y, x+y>=1, 2x+y<=4, x,y>=0  =>  1 at (1,0).
  hdi::LpData min_data;
  min_data.maximize = false;
  min_data.num_col = 2;
  min_data.num_row = 2;
  min_data.col_cost = {1.0, 2.0};
  min_data.col_lower = {0.0, 0.0};
  min_data.col_upper = {inf, inf};
  min_data.row_lower = {1.0, -inf};
  min_data.row_upper = {inf, 4.0};
  min_data.a_start = {0, 2, 4};
  min_data.a_index = {0, 1, 0, 1};
  min_data.a_value = {1.0, 2.0, 1.0, 1.0};

  const hdi::SolveResult min_result = hdi::solve(min_data);
  check(min_result.optimal, "min model status is optimal");
  check(near(min_result.objective, 1.0), "min objective == 1");
  check(min_result.col_value.size() == 2 &&
            near(min_result.col_value[0], 1.0) &&
            near(min_result.col_value[1], 0.0),
        "min solution == (1, 0)");

  // 3) Validation rejects a malformed model.
  hdi::LpData broken;
  broken.num_col = 2;
  broken.num_row = 1;
  broken.col_cost = {1.0};
  std::string error;
  check(!broken.validate(&error) && !error.empty(),
        "validate rejects inconsistent data");

  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all checks passed\n";
  return 0;
}
