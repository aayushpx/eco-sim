#include "my_node.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MyNode::MyNode() {}
MyNode::~MyNode() {}

void MyNode::_ready() {
  UtilityFunctions::print("My number is ", get_test_number());
}

void MyNode::_bind_methods() {}
