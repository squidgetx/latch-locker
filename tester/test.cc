#include "tester.h"
#include "correct_test.h"

int main(void) {
  // Basic correctness
  CorrectTester ct;
  ct.Run();

  // Basic benchmarking
  Tester t;
  t.Run();

}
