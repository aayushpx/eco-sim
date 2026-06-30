#include "test.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

TestNode::TestNode() {}
TestNode::~TestNode() {}
int TestNode::get_test_number() {
  return 42;
}

void TestNode::_ready() {
  UtilityFunctions::print(get_test_number());
  return 0;
}

void TestNode::_bind_methods() {}
