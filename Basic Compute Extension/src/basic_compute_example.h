// basic_compute_example.h
#ifndef BASIC_COMPUTE_EXAMPLE_H
#define BASIC_COMPUTE_EXAMPLE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {

class BasicComputeExample : public RefCounted {
    GDCLASS(BasicComputeExample, RefCounted)

private:
    // Original value from template
    int _value;
    
    // RenderingDevice and shader resources
    RenderingDevice* rendering_device;
    RID shader;
    RID pipeline;
    bool shader_initialized;

protected:
    static void _bind_methods();

public:
    // Initialize shader
    bool initialize_shader(const String& shader_path);
    
    // Process data with compute shader
    PackedByteArray process_data(const PackedByteArray& input_data);
    
    // Original methods from template
    void set_value(int p_value);
    int get_value() const;
    int double_value() const;

    BasicComputeExample();
    ~BasicComputeExample();
};

}  // namespace godot
#endif // BASIC_COMPUTE_EXAMPLE_H