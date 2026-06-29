#include "eco_node.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

EcoNode::EcoNode() {}
EcoNode::~EcoNode() {}

void EcoNode::_ready() {
    UtilityFunctions::print("EcoNode is alive 🚀");
}

void EcoNode::_bind_methods() {}
