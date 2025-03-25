#version 450

// Define the workgroup size
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// Input buffer (read-only)
layout(set = 0, binding = 0, std430) readonly buffer InputBuffer {
    float data[];
} input_buffer;

// Output buffer
layout(set = 0, binding = 1, std430) buffer OutputBuffer {
    float data[];
} output_buffer;

void main() {
    // Get the global invocation ID
    uint index = gl_GlobalInvocationID.x;
    
    // Make sure we don't go out of bounds
    if (index >= input_buffer.data.length()) {
        return;
    }
    
    // A simple operation: multiply each element by 2
    output_buffer.data[index] = input_buffer.data[index] * 2.0;
}