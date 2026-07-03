# Execution Plan — Step by Step

> This document tells you exactly what to do, in order. Work through sessions top to bottom. Do not start a session until the previous one's exit check passes. Pair on every session (driver/navigator, swap roles each session).

---

## Before Session 1: Setup

- [x] Install Godot 4.x (the .NET-free / standard C++ build, NOT the Mono version)
- [x] Install a C++ compiler toolchain (MSVC on Windows / clang or gcc on Mac/Linux) and CMake
- [x] Clone or create the GitHub repo
- [ ] Create folders: `/sim_core/` and `/godot_project/`
- [ ] Create a GitHub Project board with columns: `Up Next`, `In Progress`, `Done`
- [ ] Create one GitHub Issue per session below (copy each session's title and exit check into the issue)

---

## Session 1 — GDExtension Hello World

**Do this:**

1. Follow Godot's official "Getting started with GDExtension" guide (search the current Godot 4 docs for it — the exact steps depend on your Godot version, so use the in-version documentation, not a random blog post).
2. Set up `godot-cpp` as a dependency (usually a git submodule) inside `/godot_project/`.
3. Write a single C++ class with one method, e.g. `int get_test_number()` that returns `42`.
4. Register that class so Godot can see it (GDExtension registration boilerplate — the guide covers this).
5. In a GDScript script attached to a node in the Godot editor, call `get_test_number()` and `print()` the result.

**Exit check:** Running the Godot scene prints `42` (or your chosen test value) to the Godot output panel, proving C++ code is being called from GDScript.

**If stuck:** This step has the most setup friction of the whole project. Budget patience, not time. Common failure points: wrong godot-cpp version vs Godot version mismatch, missing build step, `.gdextension` file pointing to the wrong path.

---

## Session 2 — Core Data Structures (Plain C++, No Godot)

**Do this:**

1. In `/sim_core/`, create a standalone C++ project (its own `CMakeLists.txt`, its own `main.cpp`). This must compile and run with zero Godot dependency.
2. Define a `ResourceCell` struct with fields: `cell_id`, `food_amount`, `food_capacity`, `food_regen_rate`.
3. Define an `Entity` struct with fields: `entity_id`, `position` (a simple `{float x; float y;}` struct — write your own, don't pull in a math library yet), `energy`, `health`, `age_ticks`.
4. Define a `WorldState` struct containing: `tick` (int), `world_width`, `world_height`, a `vector<ResourceCell>` sized `width * height`, and a `map<int, Entity>` for entities.
5. In `main.cpp`, create one `WorldState`, set width/height to something small (e.g. 10x10), and print its dimensions.

**Exit check:** `cmake build` succeeds and running the resulting executable prints the world dimensions correctly. No Godot code anywhere in `/sim_core/` yet.

```make
cmake -S . -B build
cmake --build build
./build/sim_core
```

---

## Session 3 — Grid Initialization

**Do this:**

1. Write a function `init_grid(WorldState& world)` that fills every cell in `resource_grid` with: `food_amount = 0.0`, `food_capacity = 1.0`, `food_regen_rate = 0.01` (these are placeholder numbers — tune later).
2. Write a function `seed_food(WorldState& world, float density)` that randomly sets `food_amount` to `food_capacity` for `density` percent of cells (e.g. density = 0.3 means 30% of cells start with food).
3. Write a function `print_grid(WorldState& world)` that prints a simple text grid to console — use a character per cell, e.g. `#` for cells with food, `.` for empty.
4. Call all three from `main.cpp` and visually check the printed grid looks reasonable.

**Exit check:** Console output shows a grid of the correct width/height with roughly the expected percentage of food cells.

---

## Session 4 — Resource Regeneration

**Do this:**

1. Implement the logistic growth formula on each cell, once per tick: `food_amount += food_regen_rate * food_amount * (1 - food_amount / food_capacity)` (Handle the edge case where `food_amount` starts at 0 — logistic growth can't grow from exactly 0, so seed a small minimum like `0.01` instead of `0.0` for "empty" cells if you want them to regrow.)
2. Write a function `update_resources(WorldState& world)` that applies this to every cell.
3. In `main.cpp`, run a loop of 100 ticks calling `update_resources`, and print one specific cell's `food_amount` every 10 ticks.

**Exit check:** Printed values show the cell's food converging toward `food_capacity` over time, not exploding or going negative.

---

## Session 5 — Neural Network Forward Pass

**This is your first neural network. Go slow. Understand every line before moving on.**

**Do this:**

1. Define `struct NodeGene { int id; float bias; }` and `struct ConnectionGene { int from_node_id; int to_node_id; float weight; bool enabled; }`.
2. Define `struct Genome { vector<NodeGene> nodes; vector<ConnectionGene> connections; }`.
3. Write a function to build a **fixed-topology genome by hand** for testing: 36 input nodes (ids 0-35), 16 hidden nodes (ids 36-51), 5 output nodes (ids 52-56). Fully connect inputs→hidden and hidden→outputs with random weights between -1 and 1.
4. Implement two activation functions as separate functions: `float tanh_activation(float x)` (use `std::tanh`) and `float sigmoid_activation(float x)` (`1.0 / (1.0 + std::exp(-x))`).
5. Write `vector<float> forward_pass(const Genome& genome, const vector<float>& inputs)`:
    - Create a `map<int, float> node_values`.
    - Load the 36 input values into `node_values` for input node ids.
    - For each hidden node: sum up `node_values[from] * weight` over all enabled incoming connections, add bias, apply `tanh_activation`, store in `node_values`.
    - For each output node: same, but use `tanh_activation` for outputs 0-1 (movement) and `sigmoid_activation` for outputs 2-4 (impulses).
    - Return the 5 output values as a vector.
6. Test it: feed in `vector<float>` of 36 known values (e.g. all `0.5`), run `forward_pass`, print the 5 outputs. Confirm outputs 0-1 are in range -1 to 1, and outputs 2-4 are in range 0 to 1.

**Exit check:** `forward_pass` runs without crashing and produces 5 floats in the correct ranges for a hand-built test genome.

---

## Session 6 — Single Raycast

**Do this:**

1. Write a function `float cast_ray(WorldState& world, Vector2 origin, float angle_degrees, float max_range)` that:
    - Steps along the ray direction in increments of 1 grid cell (convert angle to a direction vector using `cos`/`sin`).
    - At each step, checks if any entity occupies that cell.
    - Returns the distance to the first hit, or `max_range` if nothing is hit.
2. In `main.cpp`, manually place 2-3 test entities at known positions.
3. Cast a ray from a known origin point at a known angle, print the returned distance.
4. Manually calculate by hand what the distance _should_ be for your test setup, and confirm the printed value matches.

**Exit check:** Ray distance printed matches your hand-calculated expected value for at least 2 different test configurations.

---

## Session 7 — Full 16-Ray Sensor Array

**Do this:**

1. Write `vector<float> get_sensor_inputs(WorldState& world, Entity& entity)` that:
    - Casts 16 rays at angles `0, 22.5, 45, 67.5, ..., 337.5` degrees (relative to entity facing direction — for now, facing can just be `0` degrees / fixed).
    - For each ray, get the distance (normalize: `distance / max_range`, clamped 0-1) and the entity type hit (use placeholder encoding: `0.0` if nothing hit, `0.5` if another entity hit).
    - Build the input array: 16 distances, then 16 types, then entity's own energy (normalized), health (normalized), speed (placeholder constant for now), age (normalized).
2. This should return exactly 36 floats — count them and verify the size with a print statement or assertion.
3. Test with your manually placed entities from Session 6, print the full array, sanity-check a few values.

**Exit check:** Function returns a `vector<float>` of size exactly 36, with values matching expectations from your test setup.

---

## Session 8 — Behaviour and Movement

**Do this:**

1. Write `void apply_behaviour(Entity& entity, const vector<float>& nn_outputs)`:
    - Set `entity.velocity = {nn_outputs[0], nn_outputs[1]}` (multiply by a speed constant, e.g. `0.5`, for now).
    - Store `nn_outputs[2]`, `nn_outputs[3]`, `nn_outputs[4]` somewhere on the entity temporarily (e.g. `eat_impulse`, `repro_impulse`, `attack_impulse` fields) — don't act on them yet, just store.
2. Write `void apply_movement(Entity& entity, WorldState& world)`:
    - `entity.position += entity.velocity`
    - Clamp position to stay within `[0, world_width]` and `[0, world_height]`.
3. In `main.cpp`, create one entity, run a loop: each tick, call `get_sensor_inputs` → `forward_pass` → `apply_behaviour` → `apply_movement`. Print the entity's position every 10 ticks for 50 ticks.

**Exit check:** Printed positions change over time and stay within world bounds. Movement direction should look plausible (not teleporting, not stuck at 0,0 the whole time unless that's genuinely where it settles).

---

## Session 9 — Integration with Godot

**Do this:**

1. In your GDExtension C++ class (from Session 1), add a member `WorldState world` and a method `void tick()` that calls, in order: `update_resources`, then for each entity: `get_sensor_inputs` → `forward_pass` → `apply_behaviour` → `apply_movement`.
2. Add a method `int get_tick_count()` that returns `world.tick`.
3. In Godot, attach a script to a node that calls `tick()` once per `_physics_process(delta)`.
4. Print `get_tick_count()` to the Godot output panel every frame (or every 30 frames to avoid spam) to confirm it's incrementing.

**Exit check:** Godot output panel shows the tick counter increasing as the scene runs, proving the full sim loop is being driven from inside Godot.

**Expect this session to take longer than others — this is where standalone C++ meets GDExtension's build system for the first time. Don't get discouraged by build errors here.**

---

## Session 10 — Minimal Rendering

**Do this:**

1. Add a method to your GDExtension class: `Array get_entity_positions()` that returns an array of `Vector2` (Godot's vector type) — one per living entity, converted from your internal `Entity` positions.
2. In Godot, create a scene with a `Node2D` parent and a script that:
    - Calls `get_entity_positions()` every frame.
    - For each position, draws a simple colored circle or square at that location (use `_draw()` with `draw_circle()`, or instantiate a `Sprite2D`/`ColorRect` per entity if simpler for a first pass).
3. Run the scene.

**Exit check:** You see dots/squares moving on screen in the Godot window, and their movement visually matches what you'd expect from the console-printed positions in Session 8.

---

## Session 11 — Eating

**Do this:**

1. Write `void resolve_eat(Entity& entity, WorldState& world)`:
    - If `entity.eat_impulse > 0.5`: find the grid cell at `entity.position`, check if `food_amount > 0`.
    - If so, transfer a fixed amount (e.g. `0.1`) from `cell.food_amount` to `entity.energy` (don't let `food_amount` go below 0, don't let `entity.energy` exceed a `max_energy` constant).
2. Call this in your tick loop after movement, for every entity.
3. Manually place one entity on a food-rich cell, force its `eat_impulse` to `1.0` for testing (bypass the NN temporarily if needed), run several ticks, print its `energy` value.

**Exit check:** Entity's energy visibly increases over ticks when positioned on a food cell, and the cell's `food_amount` visibly decreases.

---

## Session 12 — Death

**Do this:**

1. Add passive energy decay: each tick, every entity loses a fixed `metabolic_rate` (e.g. `0.02`) from `energy`.
2. Write `void check_deaths(WorldState& world, vector<int>& dead_ids)`: for every entity, if `energy <= 0` or `health <= 0`, add its id to `dead_ids`.
3. Write `void remove_dead(WorldState& world, vector<int>& dead_ids)`: erase those entities from `world.entities` after the main tick logic completes (never erase while iterating).
4. Test: spawn one entity with low starting energy, no food nearby, run ticks until it dies, confirm it's removed and no longer rendered.

**Exit check:** An entity with no food access starves and disappears from both console state and the Godot rendering within a reasonable number of ticks.

---

## Session 13 — Population Stability Test

**Do this:**

1. Spawn 50 entities at random positions, each with its own randomly-generated fixed-topology genome (reuse the hand-built genome generator from Session 5, but randomize weights per entity).
2. Seed the grid with food (use `seed_food` from Session 3, density ~0.3).
3. Run the simulation for 5,000 ticks with no manual intervention — let it run unattended (you can speed this up by calling `tick()` many times per frame in Godot, or just run it from a standalone `/sim_core/` test executable for speed).
4. Watch for: crashes, `NaN` values in position/energy (print a warning if you detect `NaN`), entities all dying immediately, entities never dying, all entities clustering at one point.

**Exit check:** Simulation completes 5,000 ticks without crashing. Final entity count is greater than 0 and less than 50 (some should have starved — if all 50 survive or all 50 die immediately, something's tuned wrong, revisit metabolic rate / food density / starting energy).

---

## Session 14 — Fix Whatever Broke

**Do this:**

1. Review whatever issues came up in Session 13.
2. Fix them. Common likely issues: metabolic rate too high/low, food regen too slow, raycast bugs causing entities to never find food, NN weights causing all outputs to saturate at the same value.
3. Re-run the Session 13 test until it passes cleanly.

**Exit check:** Same as Session 13, now passing reliably across multiple runs.

---

## What You Have After Session 14

A working simulation with: 50 entities, fixed random neural networks (no learning/evolution yet), rendered in Godot, that move, sense their environment via raycasts, eat, starve, and die — running stably for thousands of ticks.

**Deliberately not built yet:** NEAT (mutation, reproduction, speciation), UI/graphs, player controls, attack mechanic. These come next, once this foundation is confirmed solid.

---

## Next Phase (Do Not Start Until Session 14 Passes)

Once the above is done, the next block of work is the NEAT engine itself: mutation operators, asexual reproduction on entity death/reproduce, the innovation registry, and basic speciation. This will be broken into its own session-by-session list once you reach this point — ask for it then, since by then you'll have a better sense of your own pace.