#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class EcoNode : public Node {
  GDCLASS(EcoNode, Node);

protected:
  static void _bind_methods();

public:
  EcoNode();
  ~EcoNode();

  void _ready() override;
};

}
