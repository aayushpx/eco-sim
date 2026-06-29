#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "eco_node.hpp"

using namespace godot;

static void initialize_ecosim(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

  ClassDB::register_class<EcoNode>();
}

static void uninitialize_ecosim(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
}

extern "C" GDExtensionBool GDE_EXPORT ecosim_library_init(
  GDExtensionInterfaceGetProcAddress p_get_proc_address,
  GDExtensionClassLibraryPtr p_library,
  GDExtensionInitialization *r_initialization
) {
  GDExtensionBinding::InitObject init_obj(
    p_get_proc_address,
    p_library,
    r_initialization
  );

  init_obj.register_initializer(initialize_ecosim);
  init_obj.register_terminator(uninitialize_ecosim);
  init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}
