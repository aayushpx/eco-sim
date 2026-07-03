#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
class MyNode : public Node {
  GDCLASS(MyNode, Node);

protected:
  static void _bind_methods();

public:
  MyNode();
  ~MyNode();

  void _ready();

  int get_test_number() {
    return 42;
  }
};
}

