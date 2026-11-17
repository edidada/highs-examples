#include "highs_data_interface.h"

#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void printArray(const std::vector<double>& values) {
  std::cout << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ",";
    std::cout << values[i];
  }
  std::cout << "]";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: highs_cli <data-file>\n";
    return 2;
  }
  try {
    const hdi::LpData data = hdi::readLpData(argv[1]);
    const hdi::SolveResult result = hdi::solve(data);
    std::cout << std::setprecision(12) << "{\"status\":\"" << result.status
              << "\",\"objective\":" << result.objective
              << ",\"columns\":";
    printArray(result.col_value);
    std::cout << ",\"rows\":";
    printArray(result.row_value);
    std::cout << "}\n";
    return result.optimal ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 2;
  }
}
