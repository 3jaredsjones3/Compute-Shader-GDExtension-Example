// basic_compute_example.cpp
#include "basic_compute_example.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

// Modified compute shader with correct syntax for Godot 4.4
const char* BasicComputeExample::DEFAULT_COMPUTE_SHADER = R"(#version 450

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
)";

void BasicComputeExample::_bind_methods() {
    // Bind with different method names to avoid overloading issues
    ClassDB::bind_method(D_METHOD("initialize_shader"), &BasicComputeExample::initialize_shader);
    ClassDB::bind_method(D_METHOD("initialize_shader_from_path", "shader_path"), &BasicComputeExample::initialize_shader_from_path);
    ClassDB::bind_method(D_METHOD("process_data", "input_data"), &BasicComputeExample::process_data);
    
    // Keep original methods from template
    ClassDB::bind_method(D_METHOD("set_value", "value"), &BasicComputeExample::set_value);
    ClassDB::bind_method(D_METHOD("get_value"), &BasicComputeExample::get_value);
    
    // Add compute mode methods
    ClassDB::bind_method(D_METHOD("set_compute_mode", "mode"), &BasicComputeExample::set_compute_mode);
    ClassDB::bind_method(D_METHOD("get_compute_mode"), &BasicComputeExample::get_compute_mode);

    // Register properties
    ClassDB::add_property(
        "BasicComputeExample",
        PropertyInfo(Variant::INT, "value"),
        "set_value",
        "get_value"
    );
    
    ClassDB::add_property(
        "BasicComputeExample",
        PropertyInfo(Variant::INT, "compute_mode", PROPERTY_HINT_ENUM, "Built-in:0,External:1"),
        "set_compute_mode",
        "get_compute_mode"
    );
}

BasicComputeExample::BasicComputeExample() {
    _value = 0;
    rendering_device = nullptr;
    shader_initialized = false;
    compute_mode = 0; // Default to built-in shader
}

BasicComputeExample::~BasicComputeExample() {
    // Clean up shader resources if they exist
    if (rendering_device && shader_initialized) {
        if (pipeline.is_valid()) rendering_device->free_rid(pipeline);
        if (shader.is_valid()) rendering_device->free_rid(shader);
    }
}

// New method that uses the built-in shader
bool BasicComputeExample::initialize_shader() {
    // Get the rendering device
    print_line("Getting RenderingServer singleton...");
    RenderingServer* rendering_server = RenderingServer::get_singleton();
    // Avoid string concatenation with ternary operator
    if (rendering_server) {
        print_line("RenderingServer: valid");
    } else {
        print_line("RenderingServer: null");
    }
    
    print_line("Creating local rendering device...");
    rendering_device = rendering_server->create_local_rendering_device();
    // Avoid string concatenation with ternary operator
    if (rendering_device) {
        print_line("RenderingDevice: valid");
    } else {
        print_line("RenderingDevice: null");
    }
    
    if (!rendering_device) {
        ERR_PRINT("Failed to get rendering device");
        return false;
    }
    
    // Use the hard-coded shader
    String shader_code = String(DEFAULT_COMPUTE_SHADER);
    print_line("Using built-in compute shader");
    
    // Create shader source and compile
    Ref<RDShaderSource> shader_src;
    shader_src.instantiate();
    shader_src->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, shader_code);
    
    print_line("Compiling shader...");
    Ref<RDShaderSPIRV> shader_spirv = rendering_device->shader_compile_spirv_from_source(shader_src);
    if (shader_spirv.is_null()) {
        ERR_PRINT("Failed to compile shader: shader_spirv is null");
        return false;
    }
    
    // Check for compile errors in the SPIRV output
    if (shader_spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE) != "") {
        String error = shader_spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE);
        ERR_PRINT("Compute shader compile error: " + error);
        return false;
    }
    
    // Create shader from SPIRV
    print_line("Creating shader from SPIRV...");
    shader = rendering_device->shader_create_from_spirv(shader_spirv);
    if (!shader.is_valid()) {
        ERR_PRINT("Failed to create shader: RID is invalid");
        return false;
    }
    
    // Create compute pipeline
    print_line("Creating compute pipeline...");
    pipeline = rendering_device->compute_pipeline_create(shader);
    if (!pipeline.is_valid()) {
        ERR_PRINT("Failed to create compute pipeline: RID is invalid");
        if (shader.is_valid()) rendering_device->free_rid(shader);
        return false;
    }
    
    print_line("Shader initialized successfully");
    shader_initialized = true;
    return true;
}

// Original method that loads shader from file
bool BasicComputeExample::initialize_shader_from_path(const String& shader_path) {
    // Get the rendering device
    print_line("Getting RenderingServer singleton...");
    RenderingServer* rendering_server = RenderingServer::get_singleton();
    // Avoid string concatenation with ternary operator
    if (rendering_server) {
        print_line("RenderingServer: valid");
    } else {
        print_line("RenderingServer: null");
    }
    
    print_line("Creating local rendering device...");
    rendering_device = rendering_server->create_local_rendering_device();
    // Avoid string concatenation with ternary operator
    if (rendering_device) {
        print_line("RenderingDevice: valid");
    } else {
        print_line("RenderingDevice: null");
    }
    
    if (!rendering_device) {
        ERR_PRINT("Failed to get rendering device");
        return false;
    }
    
    // Load shader file
    print_line("Loading shader from: " + shader_path);
    Ref<FileAccess> file = FileAccess::open(shader_path, FileAccess::READ);
    if (file.is_null()) {
        ERR_PRINT("Failed to open shader file: " + shader_path);
        return false;
    }
    
    String shader_code = file->get_as_text();
    file->close();
    
    // Create shader source and compile
    Ref<RDShaderSource> shader_src;
    shader_src.instantiate();
    shader_src->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, shader_code);
    
    print_line("Compiling shader...");
    Ref<RDShaderSPIRV> shader_spirv = rendering_device->shader_compile_spirv_from_source(shader_src);
    if (shader_spirv.is_null()) {
        ERR_PRINT("Failed to compile shader");
        return false;
    }
    
    // Create shader from SPIRV
    print_line("Creating shader from SPIRV...");
    shader = rendering_device->shader_create_from_spirv(shader_spirv);
    if (!shader.is_valid()) {
        ERR_PRINT("Failed to create shader");
        return false;
    }
    
    // Create compute pipeline
    print_line("Creating compute pipeline...");
    pipeline = rendering_device->compute_pipeline_create(shader);
    if (!pipeline.is_valid()) {
        ERR_PRINT("Failed to create compute pipeline");
        if (shader.is_valid()) rendering_device->free_rid(shader);
        return false;
    }
    
    print_line("Shader initialized successfully");
    shader_initialized = true;
    return true;
}

void BasicComputeExample::set_value(int p_value) {
    _value = p_value;
}

int BasicComputeExample::get_value() const {
    return _value;
}

void BasicComputeExample::set_compute_mode(int p_mode) {
    compute_mode = p_mode;
}

int BasicComputeExample::get_compute_mode() const {
    return compute_mode;
}



PackedByteArray BasicComputeExample::process_data(const PackedByteArray& input_data) {
    if (!shader_initialized || !rendering_device) {
        ERR_PRINT("Shader not initialized");
        return PackedByteArray();
    }
    
    // Create input and output buffers
    RID input_buffer = rendering_device->storage_buffer_create(input_data.size(), input_data);
    
    PackedByteArray output_data;
    output_data.resize(input_data.size());
    RID output_buffer = rendering_device->storage_buffer_create(output_data.size(), output_data);
    
    // Create uniform for input buffer
    Ref<RDUniform> input_uniform;
    input_uniform.instantiate();
    input_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    input_uniform->set_binding(0);
    input_uniform->add_id(input_buffer);
    
    // Create uniform for output buffer  
    Ref<RDUniform> output_uniform;
    output_uniform.instantiate();
    output_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    output_uniform->set_binding(1);
    output_uniform->add_id(output_buffer);
    
    // Create uniform set
    Array uniforms;
    uniforms.push_back(input_uniform);
    uniforms.push_back(output_uniform);
    
    RID uniform_set = rendering_device->uniform_set_create(uniforms, shader, 0);
    
    // In Godot 4.x, the RenderingDevice API changed how compute lists are handled. 
    // Instead of using RID objects for compute lists, it now uses integer IDs directly.
    int64_t compute_list = rendering_device->compute_list_begin();
    
    // Bind pipeline and uniform set
    rendering_device->compute_list_bind_compute_pipeline(compute_list, pipeline);
    rendering_device->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
    
    // Dispatch compute shader - Fix type issues here
    int x_groups = static_cast<int>((input_data.size() / 4 + 63) / 64);
    if (x_groups == 0) x_groups = 1;
    
    print_line(String("Dispatching compute shader with x_groups = ") + String::num_int64(x_groups));
    rendering_device->compute_list_dispatch(compute_list, x_groups, 1, 1);
    
    // End compute list
    rendering_device->compute_list_end();
    
    // Submit and sync
    rendering_device->submit();
    rendering_device->sync();
    
    // Read back the data
    PackedByteArray result_data = rendering_device->buffer_get_data(output_buffer);
    
    // Clean up resources
    rendering_device->free_rid(uniform_set);
    rendering_device->free_rid(input_buffer);
    rendering_device->free_rid(output_buffer);
    
    return result_data;
}