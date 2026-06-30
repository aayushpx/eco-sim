# Neuroevolution Ecological Simulation 
> Status: Pre-production. All decisions below are locked unless explicitly reopened. Last updated: 2026-06-29

---

## 0. What This Project Actually Is

This is **not** a standard ecological simulation. It is a **continuous steady-state neuroevolution sandbox** where:

- Every entity is controlled by a neural network (its "brain")
- Brains are inherited and mutated across generations
- No species are predefined — herbivore/carnivore behaviour **emerges** from evolved network weights
- The player observes and perturbs the system; they do not control entity behaviour
- There is no win/loss condition — it is a pure sandbox

The correct reference class is: **Karl Sims' Evolved Virtual Creatures (1994)** + **Polyworld (1992)** implemented with NEAT topology evolution.

---

## 1. Locked Design Decisions

|Decision|Value|Status|
|---|---|---|
|Spatial representation|Fine-grained grid (visually continuous)|Locked|
|NN control model|All behaviour via NN outputs — no hardcoded rules|Locked|
|NN architecture style|NEAT (custom C++ implementation)|Locked|
|Raycast sensors|16 rays × [distance, entity_type] = 32 inputs|Locked|
|Output neurons|5 (move_x, move_y, eat, reproduce, attack)|Locked|
|MVP reproduction|Asexual — mutated copy of one parent genome|Locked|
|Post-MVP reproduction|Sexual — NEAT crossover between two parent genomes|Deferred|
|Evolution model|Steady-state (continuous, not generational)|Locked|
|Player role|Observer + perturbation only|Locked|
|Win/loss|None — pure sandbox|Locked|

---

## 2. Neural Network Specification

### 2.1 Input Layer (36 neurons)

|Index|Source|Encoding|
|---|---|---|
|0–15|Ray 0–15 hit distance|float, normalised 0.0–1.0 (1.0 = no hit / max range)|
|16–31|Ray 0–15 entity_type|float encoded: 0.0=empty, 0.25=food, 0.5=herbivore-like, 0.75=carnivore-like, 1.0=wall|
|32|Own energy|float, normalised 0.0–1.0|
|33|Own health|float, normalised 0.0–1.0|
|34|Own speed (evolved trait)|float, normalised 0.0–1.0|
|35|Own age|float, normalised against max observed lifespan|

**Total input neurons: 36**

> Note on entity_type encoding: a single float per ray is used for MVP. Post-MVP this can expand to multiple values per ray (e.g., one-hot or additional attributes), which will change the input layer size and invalidate existing genomes — plan a genome migration strategy before expanding.

### 2.2 Output Layer (5 neurons)

|Index|Controls|Range|Threshold rule|
|---|---|---|---|
|0|move_x|–1.0 to 1.0|Continuous — applied directly to velocity|
|1|move_y|–1.0 to 1.0|Continuous — applied directly to velocity|
|2|eat_impulse|0.0 to 1.0|Triggers eat attempt if > 0.5 and food entity in range|
|3|reproduce_impulse|0.0 to 1.0|Triggers reproduction if > 0.5 and energy > repro_threshold|
|4|attack_impulse|0.0 to 1.0|Triggers attack if > 0.5 and target entity in range|

**Total output neurons: 5**

### 2.3 Hidden Layer (MVP)

- **MVP:** Fixed topology — 1 hidden layer, **16 neurons**, tanh activation
- Input → 16 hidden → 5 output
- Only weights and biases mutate in MVP
- Topology mutation (add node, add connection) added post-MVP when full NEAT is implemented

### 2.4 Activation Functions

|Layer|Function|Reason|
|---|---|---|
|Input|Identity (pass-through)|Raw normalised values|
|Hidden|tanh|Outputs –1 to 1; handles negative signal cleanly|
|Output 0–1 (movement)|tanh|Natural –1 to 1 range|
|Output 2–4 (impulses)|Sigmoid|Natural 0 to 1 range; threshold at 0.5|

---

## 3. NEAT C++ Implementation Specification

### 3.1 Key Concept: Steady-State vs. Generational

Traditional NEAT evaluates an entire population, assigns fitness scores, then produces the next generation in a batch. This simulation is **continuous** — entities are born and die asynchronously at any tick.

**Steady-state NEAT** handles this: when an entity reproduces, its child's genome is produced immediately from the parent(s). There are no discrete generations. Speciation still applies — each entity belongs to a species cluster, reassigned periodically.

### 3.2 Core Data Structures

```
// Global — one instance exists for the entire simulation
struct InnovationRegistry {
    map<pair<int,int>, int>  connection_registry   // (from_node_id, to_node_id) → innovation_number
    int                       next_innovation_id
    int                       next_node_id

    int get_or_create_innovation(from, to)
    int create_node_id()
}

// Per genome
struct NodeGene {
    int            id
    NodeType       type          // INPUT, HIDDEN, OUTPUT
    float          bias
    ActivationFunc activation    // tanh or sigmoid
}

struct ConnectionGene {
    int   innovation_number      // globally unique, enables crossover alignment
    int   from_node_id
    int   to_node_id
    float weight                 // range: -2.0 to 2.0 (tunable)
    bool  enabled                // disabled genes are inherited but not expressed
}

struct Genome {
    int                    id
    vector<NodeGene>       nodes
    vector<ConnectionGene> connections
    float                  fitness          // updated each tick: f(age, energy_gained, offspring_count)
    int                    species_id       // assigned by Speciation system
}
```

### 3.3 Mutation Operators (MVP — weights only)

All mutations apply to a **copy** of the parent genome before assigning to the child.

|Operator|Probability (tunable)|Effect|
|---|---|---|
|Perturb weight|0.80|Each weight += gaussian noise (σ = 0.1)|
|Replace weight|0.10|Each weight = random uniform in [–2, 2]|
|Toggle connection|0.05|Flip `enabled` on a random connection|
|Perturb bias|0.60|Each bias += gaussian noise (σ = 0.05)|

Post-MVP operators (add when topology mutation is activated):

|Operator|Probability|Effect|
|---|---|---|
|Add connection|0.05|Connect two previously unconnected nodes; register innovation|
|Add node|0.03|Split an existing connection; old connection disabled; two new created; register two innovations|

### 3.4 Speciation (Compatibility Distance)

Every entity is assigned to a species. Species prevent a single dominant genome from eliminating all diversity.

**Compatibility distance formula:**

```
δ = (c1 × E) / N  +  (c2 × D) / N  +  c3 × W̄

where:
  E  = number of excess genes (beyond the range of the shorter genome)
  D  = number of disjoint genes (within the range, but not matching)
  W̄  = mean absolute weight difference of matching genes
  N  = number of genes in the larger genome (normalisation factor; use 1 if N < 20)
  c1, c2, c3 = coefficients (start: c1=1.0, c2=1.0, c3=0.4)
```

**Species assignment rule:**

- Compare each entity's genome against the representative genome of each existing species
- If δ < compatibility_threshold (start: 3.0), assign to that species
- If no species matches, create a new species with this entity as representative
- Representatives are updated periodically (every N ticks, or on species member death)

### 3.5 Fitness Function

There is no external trainer assigning fitness. Fitness is **implicit** and computed from survival outcomes:

```
fitness(entity) =
    w1 × age_ticks_survived
  + w2 × total_energy_consumed
  + w3 × offspring_count
  - w4 × times_attacked_successfully

// Starting weights (tunable by player at sim init):
w1 = 1.0
w2 = 0.5
w3 = 10.0
w4 = 2.0
```

Fitness is used only for **species representative selection** and **crossover parent selection** (post-MVP). In MVP asexual reproduction, fitness influences which entity the player sees thriving — it is an observation metric, not a training signal.

### 3.6 Forward Pass (Fixed Topology, MVP)

```
// Pseudocode — not real code

propagate_forward(genome, inputs[36]) → outputs[5]:

    // 1. Load inputs into input nodes
    for i in 0..35:
        node_values[input_node_ids[i]] = inputs[i]

    // 2. Topological order (for MVP fixed topology: inputs → hidden → outputs)
    //    Post-MVP: compute topological sort of arbitrary DAG each time
    for each hidden_node in topological_order:
        sum = node.bias
        for each enabled incoming_connection:
            sum += node_values[connection.from] × connection.weight
        node_values[hidden_node.id] = activate(sum, node.activation)

    // 3. Compute outputs
    for each output_node:
        sum = node.bias
        for each enabled incoming_connection:
            sum += node_values[connection.from] × connection.weight
        node_values[output_node.id] = activate(sum, node.activation)

    return [node_values[out_0], ..., node_values[out_4]]
```

---

## 4. Entity Data Structures (Locked)

```
struct Genome { ... }  // defined in Section 3.2

struct EvolvableTraits {
    float speed               // affects move_x/move_y magnitude scaling
    float size                // affects attack success probability, energy cost
    float raycast_range       // how far rays extend (in grid cells)
    float reproduction_cost   // energy required to reproduce
    float attack_power        // damage dealt per successful attack
    float metabolic_rate      // passive energy decay per tick multiplier
}
// All EvolvableTraits are encoded in the genome and mutated.
// They are NOT separate from the NN — they emerge from weight evolution.
// EXCEPTION: for MVP, these are fixed constants per entity to reduce complexity.

struct Entity {
    int              entity_id
    Vector2          position           // float x, float y in grid-space
    Vector2          velocity           // derived from NN output each tick
    float            energy             // 0.0–max_energy; death at 0
    float            health             // 0.0–max_health; death at 0
    int              age_ticks
    int              species_id         // assigned by speciation
    Genome           genome
    EntityState      state              // ALIVE, DEAD (deferred removal)
    int              offspring_count
    float            fitness            // updated each tick
    // Inspection stats (for player UI):
    int              kills
    int              near_misses_escaped
    int              parent_entity_id
    int              generation_number
}
```

---

## 5. Resource and Environment Structures

```
struct ResourceCell {
    int   cell_id
    float food_amount          // vegetation / plant matter
    float food_capacity        // carrying capacity for this cell
    float food_regen_rate      // per tick
    float water_amount         // if modeled; leave 0.0 if not in MVP
    bool  is_walkable
}

struct EnvCell {
    int   cell_id
    int   biome_type           // enum: GRASSLAND, WATER, ROCK, FOREST
    float temperature          // normalised 0.0–1.0
    float moisture
}

// These are separate arrays indexed by cell_id for cache efficiency
// World is W × H cells; cell_id = y * W + x

struct WorldState {
    int              tick
    int              world_width
    int              world_height
    float            cell_size           // world units per cell
    ResourceCell[]   resource_grid       // size: W × H
    EnvCell[]        env_grid            // size: W × H
    map<int,Entity>  entities            // entity_id → Entity
    InnovationRegistry innovation_reg    // global, single instance
    SimMetrics       metrics
}
```

---

## 6. System Responsibilities

### 6.1 SensorSystem

- **Owns:** Nothing (reads WorldState)
- **Per tick:** For each living entity, cast 16 rays at angles `[0, 22.5, 45, ..., 337.5]` degrees relative to entity heading
- **Ray resolution:** Step along ray in increments of 1 grid cell until hitting an entity or max_range
- **Output:** fills `float inputs[36]` per entity, ready for forward pass
- **Performance note:** This is the most expensive system. Optimise first with spatial hashing.

### 6.2 NeuralSystem

- **Owns:** Nothing (reads genomes from entities)
- **Per tick:** For each entity, run `propagate_forward(entity.genome, inputs[36])` → `outputs[5]`
- **Output:** `float nn_outputs[5]` per entity, stored temporarily for BehaviourSystem

### 6.3 BehaviourSystem

- **Owns:** Nothing (reads nn_outputs, writes action requests)
- **Per tick:** Translate NN outputs into action requests:
    - `velocity = Vector2(outputs[0], outputs[1]) × entity_speed`
    - If `outputs[2] > 0.5`: enqueue EAT_REQUEST(entity_id)
    - If `outputs[3] > 0.5` and `energy > repro_threshold`: enqueue REPRO_REQUEST(entity_id)
    - If `outputs[4] > 0.5`: enqueue ATTACK_REQUEST(entity_id)

### 6.4 MovementSystem

- **Owns:** Nothing
- **Per tick:** Apply velocity to position; clamp to world bounds; resolve cell occupancy

### 6.5 InteractionSystem

- **Owns:** Action request queues (cleared each tick)
- **Per tick:**
    - Resolve EAT requests: check food in current cell; transfer energy; deplete resource
    - Resolve ATTACK requests: find nearest entity in attack range; apply damage; transfer energy on kill; update kill/escape counters
    - Resolve REPRO requests: spawn child entity with mutated genome copy; deduct energy from parent

### 6.6 EvolutionSystem

- **Owns:** InnovationRegistry; species list
- **Per tick (not every tick — runs every N ticks):**
    - Recompute species assignments via compatibility distance
    - Cull empty species
    - Update species representatives
- **On reproduction event:**
    - Clone parent genome
    - Apply mutation operators
    - Assign child to species

### 6.7 ResourceSystem

- **Owns:** resource_grid
- **Per tick:** Apply logistic growth to food_amount per cell: `dF/dt = r × F × (1 – F/K)`

### 6.8 MetricsSystem

- **Owns:** Time-series buffers (ring buffer, configurable depth)
- **Per tick:** Snapshot population counts per species, mean fitness, food availability, birth/death rates

---

## 7. Simulation Tick Loop (Pseudocode)

```
INIT:
    load player tuning parameters
    generate world grid (W × H)
    seed resource_grid
    spawn N initial entities with randomised minimal genomes
    innovation_registry.init(36 input nodes, 5 output nodes)
    assign all entities to species_0 (initial species)

PER TICK:
    [PHASE 1 — ENVIRONMENT]
        resource_system.apply_logistic_growth()
        environment_system.advance_abiotic_factors(tick)

    [PHASE 2 — SENSE]
        for each living entity:
            inputs[36] = sensor_system.cast_rays(entity, world)

    [PHASE 3 — THINK]
        for each living entity:
            outputs[5] = neural_system.forward_pass(entity.genome, inputs)

    [PHASE 4 — QUEUE ACTIONS]
        for each living entity:
            behaviour_system.translate(entity, outputs)
            → queues: move, eat, attack, reproduce

    [PHASE 5 — RESOLVE INTERACTIONS]
        movement_system.apply_all_moves()
        interaction_system.resolve_eat_requests()
        interaction_system.resolve_attack_requests()
        interaction_system.resolve_repro_requests()
            → EvolutionSystem.create_child(parent) called here

    [PHASE 6 — LIFECYCLE]
        for each entity:
            entity.energy -= metabolic_rate
            entity.age_ticks += 1
            entity.fitness = compute_fitness(entity)
            if entity.energy <= 0 or entity.health <= 0:
                mark DEAD → add to dead_queue

        entity_manager.flush_dead(dead_queue)

    [PHASE 7 — EVOLUTION HOUSEKEEPING] (every 100 ticks)
        evolution_system.reassign_species()
        evolution_system.update_representatives()

    [PHASE 8 — METRICS + RENDER SNAPSHOT]
        metrics_system.snapshot(tick, world_state)
        render_snapshot = world_state.extract_render_data()
        // Godot reads render_snapshot on next frame — never WorldState directly
```

---

## 8. Godot Integration Plan

### 8.1 Layer Boundary

```
┌─────────────────────────────────────────────────────┐
│                    GODOT LAYER                       │
│  TileMapLayer      — cell biome / resource heatmap  │
│  EntitySpritePoo   — reads render_snapshot          │
│  RaycastDebugLayer — optional: draw ray lines       │
│  UILayer           — population graph, entity HUD   │
│  InputHandler      — translates to SimCommands      │
└────────────────────────┬────────────────────────────┘
                         │  SimCommand queue (one-way)
                         │  render_snapshot (one-way)
┌────────────────────────▼────────────────────────────┐
│              C++ SIMULATION LAYER (GDExtension)      │
│  WorldState + all systems                           │
│  NEAT genome + forward pass                         │
│  Tick loop driven by Godot _physics_process         │
└─────────────────────────────────────────────────────┘
```

**Rules:**

- Godot **never** holds a pointer into WorldState
- Player actions arrive as `SimCommand` objects enqueued at top of tick
- `render_snapshot` is a lightweight struct: `{entity_id, position, species_id, energy_normalised}[]` + cell resource levels — nothing more

### 8.2 Scene Tree

```
Main (Node)
├── SimulationController   [C++ GDExtension Node]
│       tick()
│       enqueue_command(SimCommand)
│       get_render_snapshot() → RenderSnapshot
│       get_entity_detail(entity_id) → EntityDetail
│
├── WorldRenderer (Node2D)
│   ├── TileMapLayer           biome tiles
│   ├── ResourceOverlay        food density heatmap (optional)
│   ├── EntityLayer (Node2D)   sprite pool — one sprite per living entity
│   └── RaycastDebugLayer      toggleable — draws sensor rays
│
├── UILayer (CanvasLayer)
│   ├── TopBar                 tick counter, speed controls
│   ├── PopulationGraph        per-species count over time
│   ├── SelectedEntityPanel    full entity inspection on click
│   ├── SpeciesPanel           list of current species clusters
│   └── InterventionPanel      player controls (add food, perturb, etc.)
│
└── GameManager (Node)
        sim speed state, pause, init config
```

### 8.3 Player Tuning Parameters (Configurable at Sim Start)

|Parameter|Type|Default|Notes|
|---|---|---|---|
|World width (cells)|int|200||
|World height (cells)|int|200||
|Initial population|int|100||
|Initial food density|float|0.4|0.0–1.0|
|Mutation rate (weight perturb)|float|0.80||
|Mutation magnitude (σ)|float|0.10|gaussian std dev|
|Metabolic rate|float|0.05|energy/tick passive decay|
|Raycast range|float|10.0|grid cells|
|Reproduction energy threshold|float|0.70|fraction of max_energy|
|Compatibility threshold (δ)|float|3.0|speciation sensitivity|
|Fitness weights w1–w4|float[4]|[1.0, 0.5, 10.0, 2.0]||

---

## 9. MVP Strict Scope

### 9.1 Must Ship in MVP

- [ ] 36-input → 16-hidden → 5-output fixed topology network per entity
- [ ] 16-ray sensor system per entity
- [ ] Asexual reproduction with weight-only mutation
- [ ] Logistic food regeneration per cell
- [ ] Eat, attack, movement interaction resolution
- [ ] Speciation via compatibility distance (observe only in MVP — not required for reproduction)
- [ ] Population graph per species over time
- [ ] Full entity inspection panel (all stats listed in Section 4)
- [ ] Player tuning panel at sim start
- [ ] Pause / slow / fast-forward
- [ ] Simulation stable for 10,000 ticks without crash

### 9.2 No-Build List (MVP Hard Exclusions)

- [ ] Topology mutation (add node / add connection) — post-MVP
- [ ] Sexual crossover — post-MVP
- [ ] Sensory system beyond raycasts (sound, smell) — post-MVP
- [ ] Water as a resource — post-MVP
- [ ] Abiotic factor evolution (temperature change over time) — post-MVP
- [ ] Natural disaster events — post-MVP
- [ ] Save / load simulation state
- [ ] Procedural world generation (use fixed seeded grid for MVP)
- [ ] GPU-accelerated forward pass
- [ ] Sound design
- [ ] Multiplayer

### 9.3 Implementation Order (Do Not Deviate)

1. `WorldState` struct + grid init + resource seeding
2. `ResourceSystem` — logistic growth, no consumers yet
3. `Entity` struct + spawn + age + metabolic decay + death
4. Fixed-topology NN forward pass — test with random weights, verify output shape
5. `SensorSystem` — 16 raycasts, verify hit detection correctness
6. `BehaviourSystem` + `MovementSystem` — entities move, nothing else
7. `InteractionSystem` — eat only; verify energy transfer and food depletion
8. `InteractionSystem` — attack; verify health/kill mechanics
9. `EvolutionSystem` — asexual reproduction + weight mutation
10. Godot render snapshot + entity sprite pool
11. `MetricsSystem` + population graph UI
12. Entity inspection panel
13. Player tuning panel + SimCommand queue
14. Speciation assignment + species panel
15. Stability test: 10,000 ticks, 200×200 grid, 100 starting entities

---

## 10. Open Questions (Unresolved — Must Decide Before Implementing Affected System)

| #   | Question                                                                                                 | Blocks                            |
| --- | -------------------------------------------------------------------------------------------------------- | --------------------------------- |
| OQ1 | What is max_energy and max_health — fixed constants or per-entity evolved traits?                        | Entity struct, mutation scope     |
| OQ2 | Can entities die from attack alone, or only when health + starvation combine?                            | InteractionSystem                 |
| OQ3 | What does entity_type encoding 0.5 / 0.75 mean — is it based on observed behaviour or genome similarity? | SensorSystem, SpeciationSystem    |
| OQ4 | Does water exist as a resource in MVP?                                                                   | ResourceSystem, Cell struct       |
| OQ5 | What triggers a natural disaster event and what is its mechanical effect?                                | EnvironmentSystem (post-MVP)      |
| OQ6 | Is dead matter (corpses) a food resource entities can consume?                                           | ResourceSystem, InteractionSystem |
| OQ7 | What is the spatial hashing strategy for ray queries?                                                    | SensorSystem performance          |