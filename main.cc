#include <iostream>
#include <map>

struct A {
  A() { inst_ = this; }

  static A *inst_;
};
A * A::inst_ = nullptr;

int main(int, char *[]) {
  static A a;
  std::cout <<  a.inst_ << std::endl;
  return 0;
}
