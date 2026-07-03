#include <iostream>
#include <vector>
#include <map>

struct ResourceCell {
    int cell_id;
    int food_amount;
    int food_capacity;
    float food_regen_rate;
};

struct Entity {
    int entity_id;
    float position [2] = {0, 0};
    int food_amount;
    int food_capacity;
    float food_consumption_rate;
};

struct WorldState {
    int tick;
    int world_width;
    int world_height;
    
};

int main() {
    
    WorldState world{60, 100, 100};

    std::map<int, Entity> entities;
    std::vector<ResourceCell> resource_cells; // resources_cell<wiedth * height>

    resource_cells.resize(world.world_width * world.world_height); // Ensure the vector is the correct size    

    
    // Initialize resource cells
    for (int i = 0; i < world.world_width; ++i) {
        for (int j = 0; j < world.world_height; ++j) {
            int index = i * world.world_height + j;
            resource_cells[index] = {index, 100, 100, 5}; // Example values
        }
    }

    // Add an entity
    Entity entity = {1, {5.0f, 5.0f}, 50, 100, 2}; // Example values
    entities[entity.entity_id] = entity;

    // Simulate a tick
    world.tick++;
    std::cout << "Tick: " << world.tick << std::endl;

    // Print resource cell info
    for (const auto& cell : resource_cells) {
        std::cout << "Cell ID: " << cell.cell_id 
                  << ", Food Amount: " << cell.food_amount 
                  << ", Food Capacity: " << cell.food_capacity 
                  << ", Food Regen Rate: " << cell.food_regen_rate 
                  << std::endl;
    }

    // Print entity info
    for (const auto& [id, ent] : entities) {
        std::cout << "Entity ID: " << ent.entity_id 
                  << ", Position: (" << ent.position[0] << ", " << ent.position[1] << ")"
                  << ", Food Amount: " << ent.food_amount 
                  << ", Food Capacity: " << ent.food_capacity 
                  << ", Food Consumption Rate: " << ent.food_consumption_rate 
                  << std::endl;
    }

    return 0;
}