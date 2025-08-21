#pragma once

#ifdef TINY_TEST

namespace TinyTest {
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

      template <typename T>
      void greater(const T &arg1, const T &arg2){
         res = arg1 > arg2 && res;
      }

      template <typename T>
      void less(const T &arg1, const T &arg2){
         res = arg1 < arg2 && res;
      }
 
      template <typename T>
      void greater_eq(const T &arg1, const T &arg2){
         res = arg1 >= arg2 && res;
      }

      template <typename T>
      void less_eq(const T &arg1, const T &arg2){
         res = arg1 <= arg2 && res;
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
      std::function<A&(A&)> setup_func;
      std::function<void(A&)> cleanup_func;
      std::function<A()> before_all;
      std::function<void()> after_all;
      A dep;
      AssertTest res;
      std::vector< std::function< bool(A &, AssertTest &) > > tests;
   
    public:
      AutoTest() : passed(0) {
         std::cout << "Built Test Object" << std::endl;
      }
   
      AutoTest(std::function<A()> setup, std::function<void(A&)> cleanup) : passed(0), setup_func(setup), cleanup_func(cleanup){
         std::cout << "Built Test Object With a Setup Function" << std::endl;
      }
   
      void run_all(){
         std::cout << "Running " << tests.size() << " Test Cases" << std::endl;
         int i = 0;
         if(before_all){
            dep = before_all();
         }
         for(auto test : tests){
            i++;
            
            before_each();
            bool result = test(dep, res);
            after_each();
            
            std::cout << "Test " << i << " - " << (result ? "PASSED" : "FAILED") << std::endl;
   
            if(result){
               passed += 1;
            }
            res.reset();
         }
         if(after_all){
            after_all();
         }  
         std::cout << "Tests Passed " << passed << "/" << tests.size() << std::endl;
      }
   
      AutoTest &add_test(std::function< bool(A &, AssertTest &) > testCase){
         tests.push_back(testCase);
         return *this;
      }
   
      void before_each(){
         if(setup_func){
            dep = setup_func(dep);
         }
      }
   
      void after_each(){
         if(cleanup_func){
            cleanup_func(dep);
         }
      }

      void set_before_each(std::function<A&(A&)> before){
         setup_func = before;
      }
   
      void set_after_each(std::function<void()> after){
         cleanup_func = after;
      }

      void set_before_all(std::function<A()> before){
         before_all = before;
      }
   
      void set_after_all(std::function<void()> after){
         after_all = after;
      }
   
      A &get_val(){
         return dep;
      }
   };
}

#define TEST_CASE(type, bdy) [](type &data, TinyTest::AssertTest &assert) -> bool {\
   bdy \
   return assert.result();\
}

#endif
