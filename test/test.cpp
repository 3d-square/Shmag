#include <iostream>
#include <vector>
#include <string>
#include <functional>

class AssertTest {
   bool res;

 public:
   AssertTest() : res(true) {

   }

   template <typename T>
   void equals(const T &arg1, const T &arg2){
      res = arg1 == arg2 && res;
   }

   template <typename T>
   void not_equals(const T &arg1, const T &arg2){
      res = arg1 != arg2 && res;
   }

   void contains(const std::string &str, const std::string &substr){
      bool contains = str.find(substr) != std::string::npos;
      res = contains && res;
   }
   
   void reset(){
      res = true;
   }

   bool result() const {
      return res;
   }
};

class AutoTypeNull{
   
};

template <typename A>
class AutoTest{
   int passed;
   std::function<A()> setup_func;
   std::function<void(A&)> cleanup_func;
   A dep;
   AssertTest res;
   std::vector< std::function< bool(AutoTest<A> &, AssertTest &) > > tests;

 public:
   AutoTest() : passed(0) {
      std::cout << "Built Test Object" << std::endl;
   }

   AutoTest(std::function<A()> func) : passed(0), setup_func(func){
      std::cout << "Built Test Object With a Setup Function" << std::endl;
   }

   AutoTest(std::function<A()> setup, std::function<void(A&)> cleanup) : passed(0), setup_func(setup), cleanup_func(cleanup){
      std::cout << "Built Test Object With a Setup Function" << std::endl;
   }

   void run_all(){
      std::cout << "Running " << tests.size() << " Test Cases" << std::endl;
      int i = 0;
      for(auto test : tests){
         i++;
         
         bool result = test(*this , res);
         
         std::cout << "Test " << i << " - " << (result ? "PASSED" : "FAILED") << std::endl;

         if(result){
            passed += 1;
         }
         res.reset();
      }

      std::cout << "Tests Passed " << passed << "/" << tests.size() << std::endl;
   }

   AutoTest &add_test(std::function< bool(AutoTest<A> &, AssertTest &) > testCase){
      tests.push_back(testCase);
      return *this;
   }

   void setup(){
      if(setup_func){
         dep = setup_func();
      }
   }

   void cleanup(){
      if(cleanup_func){
         cleanup_func(dep);
      }
   }

   A &get_val(){
      return dep;
   }
};

#define TEST_CASE(type, bdy) [](AutoTest<type> &obj, AssertTest &assert){\
   obj.setup(); \
   bdy \
   obj.cleanup(); \
return assert.result();\
}

int main(){
   AutoTest<AutoTypeNull> tests;

   tests.add_test(TEST_CASE(AutoTypeNull, {
      assert.not_equals<int>(1, 1);
   })).add_test(TEST_CASE(AutoTypeNull, {
      assert.equals<int>(1, 1);
   })).run_all();
}
