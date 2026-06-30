#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class TestNode : public Node {
  GDCLASS(TestNode, Node);

protected:
  static void _bind_methods();

public:
  TestNode();
  ~TestNode();
  int get_test_number();

  void _ready() override;
};

}
