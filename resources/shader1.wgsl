
// The memory location of the uniform is given by a pair of a *bind group* and a *binding*
// Specify which binding index the uniform is attached to
//@group(0) @binding(0) var<uniform> uTime: f32;


struct UniformStruct {
	iResolution: vec2f,
	iMouse: vec2f,
	iTime: f32,
};
@group(0) @binding(0) var<uniform> In: UniformStruct;


struct VertexInput {
	@location(0) position: vec2f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	// The location here does not refer to a vertex attribute, it just means
	// that this field must be handled by the rasterizer.
	// (It can also refer to another field of another struct that would be used
	// as input to the fragment shader.)
	@location(0) color: vec3f,
};


@vertex
	fn vs_main(in: VertexInput) -> VertexOutput {
	    var out: VertexOutput;
		let ratio: f32 = (In.iResolution.x/In.iResolution.y);
		out.position = vec4f(in.position.x, in.position.y * ratio, 0.0, 1.0);
		out.color = in.color;//pow(in.color,vec3f(2.2));
		return out;
	}

@fragment
	// fn fs_main(@location(0) color: vec3f) -> @location(0) vec4f { // could use this
	// Or we can use a custom struct whose fields are labeled aka  VertexOutput
	fn fs_main(in: VertexOutput) -> @location(0) vec4f {
		return vec4f(in.color, 1.0);//vec4f(in.color * sin(In.iTime), 1.0);
	} 