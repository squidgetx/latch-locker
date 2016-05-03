#include "tester.h"
#include "correct_test.h"

int main(void) {
  // Basic correctness
  CorrectTester c;
  c.Run();
	// need to rewrite correctness

  // Basic benchmarking
  Tester t;
  t.Run();
}
