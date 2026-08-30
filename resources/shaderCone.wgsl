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
    @location(0) coneDist: f32,
    @location(1) coneIter: f32,
};

///////////////////////////////////////////////////////////////////////////////
////// Helper Functions & Camera Logic
///////////////////////////////////////////////////////////////////////////////
fn rot2D(angle: f32) -> mat2x2<f32>{
    let s: f32 = sin(angle);
    let c: f32 = cos(angle);
    return mat2x2<f32>(c, -s, s, c);
}

// Uniform UV calculation for both VS and FS
fn getAspectUV(position: vec2f) -> vec2f {
    return vec2f(position.x * (In.iResolution.x / In.iResolution.y), position.y);
}

// Consistent Camera Setup
fn getCameraVectors(m: vec2f, uv: vec2f, ro_out: ptr<function, vec3f>, rd_out: ptr<function, vec3f>) {
    var base_ro: vec3f = vec3f(-2.5, 2.5, 0.0);
    var forward: vec3f = vec3f(0.0, 0.0, 1.0);

    // Apply Pitch then Yaw
    let yz: vec2f = forward.yz * rot2D(-m.y);
    forward.y = yz.x;
    forward.z = yz.y;

    let xz: vec2f = forward.xz * rot2D(-m.x);
    forward.x = xz.x;
    forward.z = xz.y;

    *ro_out = base_ro - forward * 1.0;

    // Ray Direction
    var rd: vec3f = normalize(vec3f(uv, 1.0));
    let rd_yz: vec2f = rd.yz * rot2D(-m.y);
    rd.y = rd_yz.x;
    rd.z = rd_yz.y;

    let rd_xz: vec2f = rd.xz * rot2D(-m.x);
    rd.x = rd_xz.x;
    rd.z = rd_xz.y;

    rd.y *= -1.0; // Flip Y for WebGPU coordinate space
    *rd_out = rd;
}

fn CalculateNormal(p: vec3f) -> vec3f {
    let EPS: f32 = 0.005;
    let v1: vec3f = vec3f(
        scene(p + vec3f(EPS, 0.0, 0.0)),
        scene(p + vec3f(0.0, EPS, 0.0)),
        scene(p + vec3f(0.0, 0.0, EPS))
    );
    let v2: vec3f = vec3f(
        scene(p - vec3f(EPS, 0.0, 0.0)),
        scene(p - vec3f(0.0, EPS, 0.0)),
        scene(p - vec3f(0.0, 0.0, EPS))
    );
    return normalize(v1 - v2);
}

///////////////////////////////////////////////////////////////////////////////
////// SDF
///////////////////////////////////////////////////////////////////////////////
fn sphereFold(z: vec3f, minR: f32, maxR: f32, bloatFactor: f32) -> f32 {
    let r2: f32 = dot(z, z);
    return max(maxR / max(minR, r2), bloatFactor);
}

fn boxFold(z: vec4f, r: vec3f) -> vec4f {
    return vec4f(clamp(z.xyz, -r, r) * 2.0 - z.xyz, z.w);
}

fn de_box(p: vec4f, s: vec3f) -> f32 {
    let a: vec3f = abs(p.xyz) - s;
    let aM: vec3f = vec3f(max(a.x, 0.0), max(a.y, 0.0), max(a.z, 0.0));
    return (min(max(max(a.x, a.y), a.z), 0.0) + length(aM)) / p.w;
}

fn mandleBox(pos: vec3f) -> f32 {
    var p: vec4f = vec4f(pos, 1.0);
    p *= 4.0;
    let o: vec4f = p;
    for (var i: i32 = 0; i < 9; i++) {
        p = boxFold(p, vec3f(1.0, 1.0, 1.0));
        p *= sphereFold(p.xyz, 0.0, 1.0, 1.0) * 2.0;
        p += o;
    }
    return de_box(p, vec3f(10.0, 10.0, 10.0));
}

fn scene(p: vec3f) -> f32 {
    return mandleBox(p);
}

///////////////////////////////////////////////////////////////////////////////
////// Conemarch
///////////////////////////////////////////////////////////////////////////////
fn Conemarch(ro: vec3f, rd: vec3f, fov: f32) -> vec2f {
    var t: f32 = 0.0;
    var d: f32 = 0.0;
    let cone_tan: f32 = tan(fov * 0.5);
    var cone_radius: f32 = 0.0;
    var old_t: f32 = 0.0;

    var i: i32 = 0;
    for (; i < 100; i++) {
        let p: vec3f = ro + rd * t;
        d = scene(p);
        cone_radius = (t * cone_tan) * (2.0 * (In.iResolution.x/ In.iResolution.y));

        /*
        if (d <= 0.0) {
            return vec2f(old_t, f32(i));
        }
        */

        if (d < cone_radius * 1.25 || t >= 50.0) {
            return vec2f(t, f32(i));
        }

        //old_t = t;
        // Step multiplier reduced to 0.5 to prevent skipping fractal details
        t += d;// max(d, cone_radius*0.75);
    }
    return vec2f(t, f32(i));
}

////////////
////////
////////////

///////////////////////////////////////////////////////////////////////////////
////// Vertex Shader
///////////////////////////////////////////////////////////////////////////////
@vertex
fn vs_main(vertIn: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(vertIn.position.x, vertIn.position.y, 0.0, 1.0) ;

    let uv: vec2f = getAspectUV(vertIn.position.xy);
    var ro: vec3f;
    var rd: vec3f;
    getCameraVectors(In.iMouse, uv, &ro, &rd);

    let fov: f32 = (In.iResolution.x/ In.iResolution.y);//radians(30.0);

    let coneVal: vec2f = Conemarch(ro, rd, fov);
    out.coneDist = coneVal.x;
    out.coneIter = coneVal.y;

    return out;
}

///////////////////////////////////////////////////////////////////////////////
////// Raymarch
///////////////////////////////////////////////////////////////////////////////
fn Raymarch(startDis: f32, stepsTaken: i32, ro: vec3f, rd: vec3f) -> f32 {
    var d: f32 = startDis;
    var cd: f32;

    for (var i: i32 = stepsTaken; i < 100; i++) {
        let p: vec3f = ro + d * rd;
        cd = scene(p);

        if (cd < 0.0005 || d >= 50.0) { break; }
        // Step multiplier reduced to 0.5 to prevent tunneling through geometry
        d += cd;
    }

    return d;
}

///////////////////////////////////////////////////////////////////////////////
////// Fragment Shader
///////////////////////////////////////////////////////////////////////////////
@fragment
fn fs_main(intv: VertexOutput) -> @location(0) vec4f {
    let safeStartingDistance:f32 = intv.coneDist;//intv.coneDist;
    let safeIterCount : f32 = intv.coneIter; // intv.coneIter
    if (safeStartingDistance >= 50.0) {
        return vec4f(0.0, 0.0, 1.0, 1.0);
    }

    // Convert fragment position to aspect-ratio corrected UV matching the vertex shader
    let norm_pos: vec2f = (intv.position.xy / In.iResolution) * 2.0 - 1.0;
    let uv: vec2f = getAspectUV(norm_pos);

    var ro: vec3f;
    var rd: vec3f;
    getCameraVectors(In.iMouse, uv, &ro, &rd);

    let disTravelled: f32 = Raymarch(safeStartingDistance, i32(safeIterCount), ro, rd);

    var color: vec3f;
    if (disTravelled >= 50.0) {
        color = vec3f(0.0, 0.0, 1.0);
    } else {
        let hp: vec3f = ro + disTravelled * rd;
        let n: vec3f = CalculateNormal(hp);

        let u_lightDir: vec3f = normalize(vec3f(1.0, 2.0, -1.0));
        let dotNL: f32 = dot(n, u_lightDir);
        let diff: f32 = max(dotNL, 0.0) * 0.5;
        let spec: f32 = pow(diff, 16.0) * 3.0;
        let ambient: f32 = 0.15;

        color = vec3f(1.0, 0.5, 0.5) * (spec + ambient + diff);
    }

    return vec4f(color, 1.0);
}