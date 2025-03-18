/* GRAPH PATHFINDING
    Based on example code
    March 2025
*/

#define MAX_NODES 256
#define PI 3.141592
#define TWO_PI (2.0*3.141592)

// Color definitions
#define COLOR_BACKGROUND vec3(0.1, 0.1, 0.1)
#define COLOR_NODE vec3(0.5, 0.5, 0.5)
#define COLOR_START vec3(1.0, 0.0, 0.0) // Red point for start
#define COLOR_TARGET vec3(1.0, 1.0, 1.0) // White point for target
#define COLOR_PATH vec3(0.0, 1.0, 0.0) // Green path
#define COLOR_VISITED vec3(0.0, 0.6, 0.9) // Blue for visited nodes

// Node structure
struct Node {
    vec2 position;
    int connections[8]; // Maximum 8 connections per node
    int connection_count;
    bool visited;
    float distance;
    int parent;
};

// Node data
Node nodes[MAX_NODES];
int node_count = 0;
int start_node = -1;
int target_node = -1;
int path[MAX_NODES];
int path_length = 0;
bool path_found = false;

// Converts a vec3 to int for buffer storage
int rgb_to_int(vec3 c) {
    return 0xFF000000
        | int(255.0*c.x)
        | int(255.0*c.y) << 8
        | int(255.0*c.z) << 16;
}

// Converts screen coordinates to world coordinates
vec2 screen_to_world(vec2 screen) {
    vec2 wsize = vec2(WSX, WSY);
    vec2 center = vec2(0.0, 0.0);
    return center + (screen / wsize - 0.5) * 4.0;
}

// Converts world coordinates to screen coordinates
vec2 world_to_screen(vec2 world) {
    vec2 wsize = vec2(WSX, WSY);
    vec2 center = vec2(0.0, 0.0);
    return ((world - center) / 4.0 + 0.5) * wsize;
}

// Finds the unvisited node with minimum distance
int find_min_distance_node() {
    float min_dist = 1e9;
    int min_node = -1;
    
    for (int i = 0; i < node_count; i++) {
        if (!nodes[i].visited && nodes[i].distance < min_dist) {
            min_dist = nodes[i].distance;
            min_node = i;
        }
    }
    
    return min_node;
}

// Reconstructs the path from start to target
void reconstruct_path() {
    path_length = 0;
    if (nodes[target_node].parent != -1) {
        int current = target_node;
        
        while (current != -1 && path_length < MAX_NODES) {
            path[path_length++] = current;
            current = nodes[current].parent;
        }
        
        path_found = true;
    }
}

// Initializes the graph
void init_graph() {
    // Create a simple grid graph as an example
    float grid_size = 0.5;
    int grid_width = 8;
    int grid_height = 6;
    
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            int idx = y * grid_width + x;
            if (idx >= MAX_NODES) break;
            
            // Node position
            nodes[idx].position = vec2(
                (float(x) - float(grid_width-1)/2.0) * grid_size,
                (float(y) - float(grid_height-1)/2.0) * grid_size
            );
            nodes[idx].connection_count = 0;
            nodes[idx].visited = false;
            nodes[idx].distance = 1e9; // Infinity
            nodes[idx].parent = -1;
            
            // Connect to neighbors (right, down, diagonal)
            if (x < grid_width - 1) {
                // Horizontal connection
                nodes[idx].connections[nodes[idx].connection_count++] = idx + 1;
            }
            if (y < grid_height - 1) {
                // Vertical connection
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width;
            }
            if (x < grid_width - 1 && y < grid_height - 1) {
                // Diagonal connection
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width + 1;
            }
            if (x > 0 && y < grid_height - 1) {
                // Diagonal connection
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width - 1;
            }
        }
    }
    
    node_count = min(grid_width * grid_height, MAX_NODES);
    
    // Set start and target nodes
    start_node = 0;  // Top left
    target_node = node_count - 1;  // Bottom right
    
    // Initialize pathfinding algorithm
    nodes[start_node].distance = 0;
    
    // Run pathfinding algorithm directly
    bool done = false;
    while (!done) {
        int current = find_min_distance_node();
        if (current == -1 || current == target_node) {
            done = true;
            continue;
        }
        
        nodes[current].visited = true;
        
        for (int i = 0; i < nodes[current].connection_count; i++) {
            int neighbor = nodes[current].connections[i];
            
            // Calculate distance between nodes
            vec2 diff = nodes[neighbor].position - nodes[current].position;
            float new_dist = nodes[current].distance + length(diff);
            
            if (new_dist < nodes[neighbor].distance) {
                nodes[neighbor].distance = new_dist;
                nodes[neighbor].parent = current;
            }
        }
    }
    
    // Reconstruct the path
    reconstruct_path();
}

// Draws a line between two positions
vec3 draw_line(vec2 screen_pos, vec2 start_pos, vec2 end_pos, float thickness, vec3 color) {
    vec2 line_dir = normalize(end_pos - start_pos);
    vec2 normal = vec2(-line_dir.y, line_dir.x);
    
    // Project point onto line
    float t = dot(screen_pos - start_pos, line_dir);
    float line_length = length(end_pos - start_pos);
    
    // Distance from point to line
    vec2 projected = start_pos + clamp(t, 0.0, line_length) * line_dir;
    float dist = length(screen_pos - projected);
    
    if (dist < thickness) {
        return color;
    }
    
    return vec3(0.0);
}

// Draws an animated line between two positions with glowing flow effect
vec3 draw_animated_path(vec2 screen_pos, vec2 start_pos, vec2 end_pos, float thickness, float time_offset) {
    vec2 line_dir = normalize(end_pos - start_pos);
    vec2 normal = vec2(-line_dir.y, line_dir.x);
    
    // Project point onto line
    float t = dot(screen_pos - start_pos, line_dir);
    float line_length = length(end_pos - start_pos);
    
    // Distance from point to line
    vec2 projected = start_pos + clamp(t, 0.0, line_length) * line_dir;
    float dist = length(screen_pos - projected);
    
    // Animate path (flow effect) using the step variable
    if (dist < thickness) {
        // Base color of the path
        vec3 base_color = COLOR_PATH;
        
        // Create a moving pulse effect that travels along the path
        float pulse_pos = fract((t / line_length) - time_offset * 0.3);
        float pulse = smoothstep(0.0, 0.1, pulse_pos) * smoothstep(0.4, 0.3, pulse_pos) * 1.5;
        
        // Make the path glow where the pulse is
        return mix(base_color, base_color * 2.5, pulse);
    }
    
    return vec3(0.0);
}

// Draws a node at a given position
vec3 draw_node(vec2 screen_pos, vec2 node_pos, float radius, vec3 color, float pulse) {
    float dist = length(screen_pos - node_pos);
    float r = radius * (1.0 + pulse * 0.3);
    
    if (dist < r) {
        // Smooth edge transition
        float alpha = smoothstep(r, r - 1.0, dist);
        return mix(vec3(0.0), color * (1.0 + pulse * 0.3), alpha);
    }
    
    return vec3(0.0);
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint p = x + y * WSX;
    vec2 screen_pos = vec2(x, y);
    vec2 world_pos = screen_to_world(screen_pos);
    
    // Initialize graph if not already done
    if (node_count == 0) {
        init_graph();
    }
    
    // Use step for animation timing
    float time = float(step) * 0.05;
    
    // Background color
    vec3 color = COLOR_BACKGROUND;
    
    // Draw connections between nodes
    for (int i = 0; i < node_count; i++) {
        vec2 node_screen_pos = world_to_screen(nodes[i].position);
        
        for (int j = 0; j < nodes[i].connection_count; j++) {
            int neighbor = nodes[i].connections[j];
            vec2 neighbor_screen_pos = world_to_screen(nodes[neighbor].position);
            
            // Draw thin line between nodes
            vec3 line_color = draw_line(screen_pos, node_screen_pos, neighbor_screen_pos, 0.5, vec3(0.2, 0.2, 0.2));
            color = max(color, line_color);
        }
    }
    
    // Draw visited nodes with static color (no animation)
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].visited && i != start_node && i != target_node) {
            vec2 node_screen_pos = world_to_screen(nodes[i].position);
            
            // No pulse effect, just static nodes
            vec3 node_drawn = draw_node(screen_pos, node_screen_pos, 4.0, COLOR_VISITED, 0.0);
            color = max(color, node_drawn);
        }
    }
    
    // Draw the path with animated flowing effect
    if (path_found) {
        // We'll draw the path in segments with animation flowing through
        for (int i = 0; i < path_length - 1; i++) {
            vec2 path_screen_pos = world_to_screen(nodes[path[i]].position);
            vec2 next_screen_pos = world_to_screen(nodes[path[i+1]].position);
            
            // Offset the animation for each segment to create a flowing effect
            float segment_offset = time + float(i) * 0.2;
            
            vec3 path_color = draw_animated_path(screen_pos, path_screen_pos, next_screen_pos, 2.0, segment_offset);
            color = max(color, path_color);
        }
    }
    
    // Draw all nodes
    for (int i = 0; i < node_count; i++) {
        vec2 node_screen_pos = world_to_screen(nodes[i].position);
        
        // Node color without pulse effect
        vec3 node_color = COLOR_NODE;
        float pulse = 0.0; // No pulse for any nodes
        float node_size = 5.0;
        
        if (i == start_node) {
            node_color = COLOR_START;
            node_size = 8.0;
        } else if (i == target_node) {
            node_color = COLOR_TARGET;
            node_size = 8.0;
        }
        
        vec3 node_drawn = draw_node(screen_pos, node_screen_pos, node_size, node_color, pulse);
        color = max(color, node_drawn);
    }
    
    // Show cursor position
    if ((mousex >= 0 && x == mousex) || (mousey >= 0 && y == mousey)) {
        color = vec3(0.7, 0.7, 0.7);
    }
    
    // Write result to buffer
    data_0[p] = rgb_to_int(color);
}