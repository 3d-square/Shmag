#include <iostream>
#include <vector>
#include <string>
#include <functional>

class AssertTest {
   bool res;

 public:
   AssertTest() : res(true) {

   }

   // void assert_eq();
   // void assert_constains();
   
   void reset(){
      res = true;
   }

   bool result() const {
      return res;
   }
};

class AutoTest {
   int passed;
   AssertTest res;
   std::vector< std::function< bool(AssertTest &) > > tests;

 public:
   AutoTest() : passed(0) {
      std::cout << "Built Test Object" << std::endl;
   }

   void run_all(){
      std::cout << "Running " << tests.size() << " Test Cases" << std::endl;
      int i = 0;
      for(auto test : tests){
         i++;
         bool result = test(res);
         
         std::cout << "Test " << i << " - " << (result ? "PASSED" : "FAILED") << std::endl;

         if(result){
            passed += 1;
         }
         res.reset();
      }

      std::cout << "Tests Passed " << passed << "/" << tests.size() << std::endl;
   }

   AutoTest &add_test(std::function< bool(AssertTest &) > testCase){
      tests.push_back(testCase);
      return *this;
   }
};

#define TEST_CASE(bdy) [](AssertTest &ass){\
   bdy \
return ass.result();\
}

int main(){
   AutoTest tests;

   tests.add_test(TEST_CASE({
      std::cout << "Test 1" << std::endl;
   })).add_test(TEST_CASE({
      std::cout << "Test 2" << std::endl;
   })).run_all();
}
