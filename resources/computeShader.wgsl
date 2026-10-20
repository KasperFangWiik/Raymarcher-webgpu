

@group(0) @binding(0) 
var grayNoiseTexture: texture_storage_2d<r32float,write>;

fn S(lambd: f32) -> f32{
    //return 3.*pow(lambd,2.) - 2.*pow(lambd,3.);
    return (lambd * lambd) *(3. - (2.*lambd) );
}

fn get_random_noize_corner(ij: vec2f) -> f32 {
    let p = fract(ij * vec2f(123.34, 456.21));
    let p2 = p + dot(p, p + 45.32);
    return fract(p2.x * p2.y) * 2.0 - 1.0;
}

fn f(p: vec2f)-> f32{

    var fr: vec2f = fract(p);
    var ij: vec2f = floor(p); // (i,j) = floor(x,z)

    // b_i-1,j = c_i,j-1 = d_i-1,j-1 = a_i,j
    // b_i-1,j = a_i,j => b_i,j = a_i+1,j
    let a_ij: f32 = get_random_noize_corner(ij);
    let b_ij: f32 = get_random_noize_corner(ij+vec2f(0., 1.));
    let c_ij: f32 = get_random_noize_corner(ij+vec2f(1., 0.));
    let d_ij: f32 = get_random_noize_corner(ij+vec2f(1., 1.));

    let s_x: f32 = S(fr.x);
    let s_z: f32 = S(fr.y);
    let ba_difz: f32 = (b_ij - a_ij)*s_z;
    let ca_difx: f32 = (c_ij - a_ij)*s_x;
    let abcd_dif: f32 = (a_ij - b_ij - c_ij + d_ij);

    return (a_ij + ba_difz + ca_difx + abcd_dif*s_x*s_z);
}

fn N_func(p: vec2f, coord_scale: f32, layers: i32) -> f32{

    //var rot: mat2x2<f32> = mat2x2<f32>(0.8, -0.6, 0.6, 0.8);//mat2x2<f32>(4./5., -3./5., 3/5, 4/5);
    
    var rot: mat2x2<f32> = mat2x2<f32>(0.8, 0.6, -0.6, 0.8);

    var pr: vec2f = p * coord_scale;// * 0.002; // 1/2000
    
    var N_val: f32 = f(pr);
    //skipping one rotation seams to give a more intresting look
    //pr = 2. * (rot * pr);  
    var a: f32 = 0.5;
    
    //  f + 1/2 * f1 + (1/4) * f2 + (1/8) * f3 + (1/16) * f4
    for(var i: i32 = 1; i < layers; i++){
        N_val += f(pr) * a;
        a *= 0.5;
        pr = 2.0 * (pr * rot);
   }
   return N_val;
}


@compute @workgroup_size(4,4)
fn cs_main(@builtin(global_invocation_id) id: vec3<u32>){
    
    let xy: vec2f = vec2f(id.xy);
    let p = fract(xy * vec2f(123.34, 456.21));
    let p2 = p + dot(p, p + 45.32);
    textureStore(grayNoiseTexture, id.xy, vec4f(fract(p2.x * p2.y) * 2.0 - 1.0, 0.0, 0.0, 0.0) );
    
    //textureStore(grayNoiseTexture, id.xy, vec4f(f(vec2f(id.xy)), 0.0, 0.0, 0.0) );

    //textureStore(grayNoiseTexture, id.xy, vec4f(N_func(vec2f(id.xy), 0.002, 10), 0.0, 0.0, 0.0) );
}