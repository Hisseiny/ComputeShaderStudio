/* GRAPH PATHFINDING
    Compute Shader - Godot 4.x
    March 2025
*/

// Define max nodes
#define MAX_NODES 256
#define PI 3.141592
#define TWO_PI (2.0 * 3.141592)

// User parameters (can be set externally)
float u_zoom = 1.0f;
float u_pos_x = 0.0f;
float u_pos_y = 0.0f;

// Colors
#define COLOR_BACKGROUND vec3(0.1, 0.1, 0.1)
#define COLOR_NODE vec3(0.5, 0.5, 0.5)
#define COLOR_START vec3(1.0, 0.0, 0.0)
#define COLOR_TARGET vec3(1.0, 1.0, 1.0)
#define COLOR_PATH vec3(0.0, 1.0, 0.0)

// Node structure
struct Node {
    vec2 position;
    int connections[8]; // up to 8 connections
    int connection_count;
    bool visited;
    float distance;
    int parent;
};

// Global states
int last_mouse_button = 0;
int selection_state = 0; // 0: select start, 1: select target
int click_step = -100;

Node nodes[MAX_NODES];
int node_count = 0;
int start_node = -1;
int target_node = -1;
int path[MAX_NODES];
int path_length = 0;
bool path_found = false;

// Convert vec3 color to int32 for buffer
int rgb_to_int(vec3 c) {
    return 0xFF000000
        | int(255.0 * c.x)
        | int(255.0 * c.y) << 8
        | int(255.0 * c.z) << 16;
}

// Convert screen space to world space
vec2 screen_to_world(vec2 screen) {
    vec2 wsize = vec2(WSX, WSY);
    vec2 center = vec2(u_pos_x, u_pos_y);
    return center + (screen / wsize - 0.5) * 4.0 / u_zoom;
}

// Convert world space to screen space
vec2 world_to_screen(vec2 world) {
    vec2 wsize = vec2(WSX, WSY);
    vec2 center = vec2(u_pos_x, u_pos_y);
    return ((world - center) * u_zoom / 4.0 + 0.5) * wsize;
}

// Find closest node to a screen position
int find_closest_node(vec2 screen_pos) {
    float min_dist = 30.0;
    int closest = -1;

    for (int i = 0; i < node_count; i++) {
        vec2 node_screen_pos = world_to_screen(nodes[i].position);
        float dist = length(node_screen_pos - screen_pos);
        if (dist < min_dist) {
            min_dist = dist;
            closest = i;
        }
    }

    return closest;
}

// Find unvisited node with minimum distance
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

// Reconstruct the path from target to start
void reconstruct_path() {
    path_length = 0;
    if (start_node >= 0 && target_node >= 0 && nodes[target_node].parent != -1) {
        int current = target_node;
        while (current != -1 && path_length < MAX_NODES) {
            path[path_length++] = current;
            current = nodes[current].parent;
        }
        path_found = true;
    } else {
        path_found = false;
    }
}

// Dijkstra pathfinding algorithm
void calculate_path() {
    for (int i = 0; i < node_count; i++) {
        nodes[i].visited = false;
        nodes[i].distance = 1e9;
        nodes[i].parent = -1;
    }

    if (start_node < 0 || target_node < 0) {
        path_found = false;
        path_length = 0;
        return;
    }

    nodes[start_node].distance = 0;

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
            vec2 diff = nodes[neighbor].position - nodes[current].position;
            float new_dist = nodes[current].distance + length(diff);

            if (new_dist < nodes[neighbor].distance) {
                nodes[neighbor].distance = new_dist;
                nodes[neighbor].parent = current;
            }
        }
    }

    reconstruct_path();
}

// Initialize a simple grid graph
void init_graph() {
    float grid_size = 0.5;
    int grid_width = 8;
    int grid_height = 6;

    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            int idx = y * grid_width + x;
            if (idx >= MAX_NODES) break;

            nodes[idx].position = vec2(
                (float(x) - float(grid_width - 1) / 2.0) * grid_size,
                (float(y) - float(grid_height - 1) / 2.0) * grid_size
            );
            nodes[idx].connection_count = 0;
            nodes[idx].visited = false;
            nodes[idx].distance = 1e9;
            nodes[idx].parent = -1;

            if (x < grid_width - 1)
                nodes[idx].connections[nodes[idx].connection_count++] = idx + 1;
            if (y < grid_height - 1)
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width;
            if (x < grid_width - 1 && y < grid_height - 1)
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width + 1;
            if (x > 0 && y < grid_height - 1)
                nodes[idx].connections[nodes[idx].connection_count++] = idx + grid_width - 1;
        }
    }

    node_count = min(grid_width * grid_height, MAX_NODES);

    // Initialize with default nodes
    start_node = 0;
    target_node = node_count - 1;

    calculate_path();
}

// Draw an animated line with a flowing effect
vec3 draw_animated_line(vec2 screen_pos, vec2 start_pos, vec2 end_pos, float thickness) {
    vec2 line_dir = normalize(end_pos - start_pos);
    float t = dot(screen_pos - start_pos, line_dir);
    float line_length = length(end_pos - start_pos);
    vec2 projected = start_pos + clamp(t, 0.0, line_length) * line_dir;
    float dist = length(screen_pos - projected);

    if (dist < thickness) {
        float time = float(step) * 0.05;
        float flow_pos = fract((t / line_length) - time);
        float intensity = smoothstep(0.0, 0.2, flow_pos) * smoothstep(1.0, 0.8, flow_pos) * 1.2;

        vec3 base_color = COLOR_PATH;
        vec3 highlight_color = vec3(0.5, 1.0, 0.0);

        return mix(base_color, highlight_color, intensity);
    }

    return vec3(0.0);
}

// Draw a node (circle) at a given position
vec3 draw_node(vec2 frag_coord, vec2 center, float radius, vec3 color) {
    float dist = length(frag_coord - center);
    float alpha = smoothstep(radius, radius - 1.0, dist);
    return color * alpha;
}

// Main compute shader
void main() {
    uint p = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * WSX;
    vec2 screen_pos = vec2(gl_GlobalInvocationID.xy);
    vec2 world_pos = screen_to_world(screen_pos);

    if (node_count == 0) {
        init_graph();
    }

    bool is_mouse_click = (mouse_button == 1 && last_mouse_button == 0);

    if (is_mouse_click && mousex >= 0 && mousey >= 0 && step > click_step + 5) {
        click_step = step;

        int clicked_node = find_closest_node(vec2(mousex, mousey));

        if (clicked_node >= 0) {
            if (selection_state == 0) {
                start_node = clicked_node;
                selection_state = 1;
                path_found = false;
            } else if (selection_state == 1) {
                if (clicked_node != start_node) {
                    target_node = clicked_node;
                    calculate_path();
                }
                selection_state = 0;
            }
        }
    }

    last_mouse_button = mouse_button;

    vec3 color = COLOR_BACKGROUND;

    // Draw connections between nodes
    for (int i = 0; i < node_count; i++) {
        vec2 node_screen_pos = world_to_screen(nodes[i].position);

        for (int j = 0; j < nodes[i].connection_count; j++) {
            int neighbor = nodes[i].connections[j];
            vec2 neighbor_screen_pos = world_to_screen(nodes[neighbor].position);

            vec2 line_dir = normalize(neighbor_screen_pos - node_screen_pos);
            float t = dot(screen_pos - node_screen_pos, line_dir);
            float line_length = length(neighbor_screen_pos - node_screen_pos);
            vec2 projected = node_screen_pos + clamp(t, 0.0, line_length) * line_dir;
            float dist = length(screen_pos - projected);

            if (dist < 0.5) {
                color = max(color, vec3(0.3, 0.3, 0.3));
            }
        }
    }

    // Draw the path found
    if (path_found) {
        for (int i = 0; i < path_length - 1; i++) {
            vec2 path_screen_pos = world_to_screen(nodes[path[i]].position);
            vec2 next_screen_pos = world_to_screen(nodes[path[i + 1]].position);

            vec3 path_color = draw_animated_line(screen_pos, path_screen_pos, next_screen_pos, 2.0);
            color = max(color, path_color);
        }
    }

    // Draw nodes
    for (int i = 0; i < node_count; i++) {
        vec2 node_screen_pos = world_to_screen(nodes[i].position);

        vec3 node_color = COLOR_NODE;
        if (i == start_node)
            node_color = COLOR_START;
        else if (i == target_node)
            node_color = COLOR_TARGET;

        vec3 node_drawn = draw_node(screen_pos, node_screen_pos, 5.0, node_color);
        color = max(color, node_drawn);
    }

    // Highlight start and target nodes
    if (start_node >= 0) {
        vec2 start_screen_pos = world_to_screen(nodes[start_node].position);
        vec3 start_drawn = draw_node(screen_pos, start_screen_pos, 8.0, COLOR_START);
        color = max(color, start_drawn);
    }

    if (target_node >= 0) {
        vec2 target_screen_pos = world_to_screen(nodes[target_node].position);
        vec3 target_drawn = draw_node(screen_pos, target_screen_pos, 8.0, COLOR_TARGET);
        color = max(color, target_drawn);
    }

    // Show mouse cursor position
    if ((mousex >= 0 && gl_GlobalInvocationID.x == mousex) ||
        (mousey >= 0 && gl_GlobalInvocationID.y == mousey)) {
        color = vec3(0.7, 0.7, 0.7);
    }

    // Hint node under mouse cursor
    int hint_node = find_closest_node(vec2(mousex, mousey));
    if (hint_node >= 0 && hint_node != start_node && hint_node != target_node) {
        vec2 hint_pos = world_to_screen(nodes[hint_node].position);
        vec3 hint_color = (selection_state == 0) ? COLOR_START : COLOR_TARGET;
        hint_color = mix(hint_color, vec3(1.0), 0.3);

        float pulse = sin(float(step) * 0.1) * 0.5 + 0.5;
        vec3 hint_drawn = draw_node(screen_pos, hint_pos, 6.0 + pulse * 2.0, hint_color * 0.3);
        color = max(color, hint_drawn);
    }

    data_0[p] = rgb_to_int(color);
}
