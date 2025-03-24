// basic_compute_example.cpp
#include "basic_compute_example.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void BasicComputeExample::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize_shader", "shader_path"), &BasicComputeExample::initialize_shader);
    ClassDB::bind_method(D_METHOD("process_data", "input_data"), &BasicComputeExample::process_data);
    
    // Keep original methods from template
    ClassDB::bind_method(D_METHOD("set_value", "value"), &BasicComputeExample::set_value);
    ClassDB::bind_method(D_METHOD("get_value"), &BasicComputeExample::get_value);
    ClassDB::bind_method(D_METHOD("double_value"), &BasicComputeExample::double_value);

    // Register property
    ClassDB::add_property(
        "BasicComputeExample",
        PropertyInfo(Variant::INT, "value"),
        "set_value",
        "get_value"
    );
}

BasicComputeExample::BasicComputeExample() {
    _value = 0;
    rendering_device = nullptr;
    shader_initialized = false;
}

BasicComputeExample::~BasicComputeExample() {
    // Clean up shader resources if they exist
    if (rendering_device && shader_initialized) {
        if (pipeline.is_valid()) rendering_device->free_rid(pipeline);
        if (shader.is_valid()) rendering_device->free_rid(shader);
    }
}

bool BasicComputeExample::initialize_shader(const String& shader_path) {
    // Get the rendering device
    RenderingServer* rendering_server = RenderingServer::get_singleton();
    rendering_device = rendering_server->create_local_rendering_device();
    
    if (!rendering_device) {
        ERR_PRINT("Failed to get rendering device");
        return false;
    }
    
    // Load shader file - fixed by removing the &err parameter
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
    
    Ref<RDShaderSPIRV> shader_spirv = rendering_device->shader_compile_spirv_from_source(shader_src);
    if (shader_spirv.is_null()) {
        ERR_PRINT("Failed to compile shader");
        return false;
    }
    
    // Create shader from SPIRV
    shader = rendering_device->shader_create_from_spirv(shader_spirv);
    if (!shader.is_valid()) {
        ERR_PRINT("Failed to create shader");
        return false;
    }
    
    // Create compute pipeline
    pipeline = rendering_device->compute_pipeline_create(shader);
    if (!pipeline.is_valid()) {
        ERR_PRINT("Failed to create compute pipeline");
        if (shader.is_valid()) rendering_device->free_rid(shader);
        return false;
    }
    
    shader_initialized = true;
    return true;
}

void BasicComputeExample::set_value(int p_value) {
    _value = p_value;
}

int BasicComputeExample::get_value() const {
    return _value;
}

int BasicComputeExample::double_value() const {
    return _value * 2;
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
    
    // Dispatch compute shader
    uint32_t x_groups = (input_data.size() / 4 + 63) / 64;
    if (x_groups == 0) x_groups = 1;
    
    rendering_device->compute_list_dispatch(compute_list, x_groups, 1, 1);
    
    // End compute list without passing the compute_list parameter
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