#include "tester.h"
#include "correct_test.h"

int main(void) {
  // Basic benchmarking
  Tester t;
  t.Run();
  // Basic correctness
  CorrectTester ct;
  ct.Run();
}
