extends Node

func _ready():
	test_compute_shader()

func test_compute_shader():
	print("Testing BasicComputeExample GDExtension")
	
	# Debug rendering info
	print("Godot version: " + Engine.get_version_info().string)
	print("Renderer: " + RenderingServer.get_video_adapter_name())
	
	# Create an instance of our custom class
	var compute_example = BasicComputeExample.new()
	
	# First test the original template functionality
	compute_example.set_value(21)
	print("Value: ", compute_example.get_value())
	
	# Select compute mode (0 = built-in, 1 = external)
	var use_external_shader = false
	compute_example.set_compute_mode(1 if use_external_shader else 0)
	print("Using compute mode: ", "External" if compute_example.get_compute_mode() == 1 else "Built-in")
	
	# Try to initialize the shader
	print("Attempting to initialize shader...")
	var success = false
	
	if compute_example.get_compute_mode() == 0:
		# Use built-in shader
		success = compute_example.initialize_shader()
	else:
		# Use external shader file
		var shader_path = "res://compute.glsl"
		if not FileAccess.file_exists(shader_path):
			print("ERROR: Compute shader file not found at: " + shader_path)
			return
		print("Found shader file at: " + shader_path)
		success = compute_example.initialize_shader_from_path(shader_path)
	
	if not success:
		print("Shader initialization failed!")
		print("Possible reasons:")
		print("1. RenderingDevice may not be accessible (This is a common issue)")
		print("2. Your GPU might not support the required Vulkan features")
		print("3. There could be a shader compilation error")
		print("\nTry adding print debugging in the C++ code to see where it's failing")
		return
	
	print("Shader initialized successfully")
	
	# Create a small test array 
	var input_data = PackedFloat32Array([1.0, 2.0, 3.0, 4.0, 5.0])
	
	# Convert to bytes
	var input_bytes = PackedByteArray()
	input_bytes.resize(input_data.size() * 4)  # 4 bytes per float
	
	for i in range(input_data.size()):
		input_bytes.encode_float(i * 4, input_data[i])
	
	# Process the data
	print("Processing data with compute shader...")
	var output_bytes = compute_example.process_data(input_bytes)
	
	# Convert back to floats
	var output_data = PackedFloat32Array()
	output_data.resize(output_bytes.size() / 4)
	
	for i in range(output_data.size()):
		output_data[i] = output_bytes.decode_float(i * 4)
	
	# Print results
	print("Input:  ", input_data)
	print("Output: ", output_data)
	print("Expected: [2.0, 4.0, 6.0, 8.0, 10.0]")
	
	# Verify results
	var all_correct = true
	for i in range(input_data.size()):
		if abs(output_data[i] - input_data[i] * 2.0) > 0.001:
			print("Error at index ", i, ": expected ", input_data[i] * 2.0, " but got ", output_data[i])
			all_correct = false
	
	if all_correct:
		print("✅ All results match! The compute shader is working correctly!")
	else:
		print("❌ Some results were incorrect.")
