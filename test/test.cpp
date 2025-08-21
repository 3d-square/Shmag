#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

#define TINY_TEST
#include "tinytest.hpp"

auto test_case_1 = TEST_CASE(int ,{
   std::cout << "Data " << data << std::endl;

   assert.greater<int>(1, 2);
});

auto test_case_2 = TEST_CASE(int ,{
   std::cout << "Data " << data << std::endl;
});

auto before_all = []() -> int {
   system("cd test; mkdir -p archive");
   return 0;
};

auto before_each = [](int &tc) -> int &{
   tc += 1;
   std::string command = "mkdir -p archive/testcase-" + std::to_string(tc);
   system(command.c_str());
   return tc;
};

int main(){
   TinyTest::AutoTest<int> tests;

   tests.set_before_all(before_all);
   tests.set_before_each(before_each);

   tests\
   .add_test(test_case_1)\
   .add_test(test_case_2)\
   .run_all();
}
