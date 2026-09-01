
struct UniformStruct {
	iResolution: vec2f,
	iMouse: vec2f,
	iTime: f32,
};
@group(0) @binding(0) 
var<uniform> In: UniformStruct;

@group(0) @binding(1) 
var grayNoiseTexture: texture_2d<f32>;

@group(0) @binding(2)
var textureSampler: sampler; 

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
	//@location(0) color: vec3f,
    @location(0) coneDist: f32,
    @location(1) coneIter: f32,
};

///////////////////////////////////////////////////////////////////////////////
////// Conemarch
///////////////////////////////////////////////////////////////////////////////
fn Conemarch(ro: vec3f,rd: vec3f, fov: f32, layers: i32) -> vec2f{
    var p: vec3f = vec3f(0.0);
    var t: f32 = 0.;
    var d: f32 = 0.;
    var stepsize: f32 = 0.;
    var cone_tan: f32 = tan(fov*0.5);
    var cone_radius: f32 = 0.;
    var old_t: f32;
    
    // itteration count should be same as normal raymarch 1500
    var i: i32=0;
    for(; i < 2000; i++){ 
        p = ro + rd*t;
        
        d = sdPlane_enviornment(p, 0.0, layers);
        
        cone_radius = (t*cone_tan) * 2.0*(In.iResolution.x/In.iResolution.y);// (In.iResolution.y / 32.0);
        /*
        if(d <= 0.){
          return vec2f(old_t, f32(i));
        }*/
            
        if( d < cone_radius || (t >= 9000.)) {
            return vec2f(t, f32(i)); // max(t*0.3, 0.05)
        }
        stepsize = d*0.5;// max(d*0.5,cone_radius*0.75);//== relaxation//*0.4; 
        //old_t = t;
        t += stepsize;//max(stepsize, 0.02);
    }
    // if no hit by conemarch then we return the maximum distance t can travel
    return vec2f(t, f32(i));
}

///////////////////////////////////////////////////////////////////////////////
////// vertex shader
///////////////////////////////////////////////////////////////////////////////
@vertex
fn vs_main(vertIn: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let ratio: f32 = (In.iResolution.x/In.iResolution.y);
	out.position = vec4f(vertIn.position.x, vertIn.position.y, 0.0, 1.0); 
    //out.position = vec4f(vertIn.position.x, vertIn.position.y * ratio, 0.0, 1.0); 

    var m: vec2f = In.iMouse;
 
    var base_ro: vec3f = vec3f(0.0, 1500.0, 0.0);
    var forward: vec3f = vec3f(0.0, 0.0, 1.0);

    var forw_yz: vec2f = forward.yz *  rot2D(-m.y);
    var forw_xz: vec2f = forward.xz *  rot2D(-m.x);
    forward.y = -forw_yz.x;
    //forward.z = forw_yz.y;
    forward.x = forw_xz.x;
    forward.z = forw_xz.y;

    var ro: vec3f = base_ro - forward * 1000.0;
    
    //var rd: vec3f = raydir(out.position.xy, m);
    var rd: vec3f = raydir(vec2f(vertIn.position.x, vertIn.position.y * ratio), m);

    rd.y *= -1.0; // flip y coordinate because it's fliped in wgsl

    let layers: i32 = 10; 
    let fov: f32 = (In.iResolution.x/In.iResolution.y);//radians(90);//radians(30);//radians(45);//radians(90);//3.14159265/100.;//*0.5;//*2.0;//3.14159265 * 0.5;//3.14159265;//3.14159265 * 0.5;//1.5708;//90degree// 0.7854;//PI/4.0;

    let coneVal: vec2f = Conemarch( ro, rd, fov, layers);
    out.coneDist = coneVal.x;
    out.coneIter = coneVal.y;
	//out.color = vertIn.color;//pow(in.color,vec3f(2.2));
	return out;
}



///////////////////////////////////////////////////////////////////////////////
////// General helper functions
///////////////////////////////////////////////////////////////////////////////
fn rot2D(angle: f32) -> mat2x2<f32>{
    let s: f32 = sin(angle);
    let c: f32 = cos(angle);
    return mat2x2<f32>(c, -s, s, c);
}

fn raydir(uv: vec2f, m: vec2f) -> vec3f {
    var rd: vec3f = normalize(vec3f(uv, 1.0));
    
    // Vertical camera rotation
    let yz: vec2f = rd.yz * rot2D(-m.y);
    rd.y = yz.x;
    rd.z = yz.y;
    
    // Horizontal camera rotation
    let xz: vec2f = rd.xz * rot2D(-m.x);
    rd.x = xz.x;
    rd.z = xz.y;
    
    return rd;
}

fn S(lambd: f32) -> f32{
    //return 3.*pow(lambd,2.) - 2.*pow(lambd,3.);
    return (lambd * lambd) *(3. - (2.*lambd) );
}

fn d_S(lambd: f32) -> f32{
    return 6.*lambd *(1. - lambd);
}

fn smoothStep( a: f32, b: f32, x: f32) -> f32{
    return S( min(1.0,max(0.0, (x-a)/(b-a))) );
}

fn smoothStepVec3( a: f32, b: f32, v: vec3f) -> vec3f{
    let x: f32 = S( min(1.0,max(0., (v.x-a)/(b-a))) );
    let y: f32 = S( min(1.0,max(0., (v.y-a)/(b-a))) );
    let z: f32 = S( min(1.0,max(0., (v.z-a)/(b-a))) );
    return vec3f(x,y,z);
}

///////////////////////////////////////////////////////////////////////////////
////// General SDF's
///////////////////////////////////////////////////////////////////////////////
const DIFFUSE_MATERIAL_TYPE: u32 = 0u;
const GRASS_MOUNTAIN_TYPE: u32 = 1u;
const SUN_MATERIAL_TYPE: u32 = 2u;

struct Body {
    sd      : f32,
    deriv   : vec3f,
    col     : vec4f,
    material: u32,
};

fn sdSphere( p: vec3f, cord: vec3f) -> f32{
    let r: f32 = 500.0;
    return length(p - cord) - r;
}

// should have the material type as a imput
fn bdSphere(p: vec3f, cord: vec3f, color: vec4f) -> Body{
    let r: f32 = 500.0;
    var dir: vec3f = p - cord;
    return Body(length(dir) - r, normalize(dir), color, DIFFUSE_MATERIAL_TYPE);
}

fn bdSun( p: vec3f, cord: vec3f, color: vec4f)-> Body{
    let r: f32 = 500.0;
    var dir: vec3f = p - cord;
    return Body(length(dir) - r, normalize(dir), color, SUN_MATERIAL_TYPE);
}

fn bdGround(p: vec3f, offset: f32, color: vec4f)-> Body{
    return Body(p.y - offset,vec3f(0.0,1.0,0.0),color,DIFFUSE_MATERIAL_TYPE);
}

fn bdPlane(p: vec3f, offset: f32, color: vec4f)-> Body{
  return Body(dot(p,vec3f(0.0,-1.0,0.0)) + offset,vec3f(0.0,-1.0,0.0),color,DIFFUSE_MATERIAL_TYPE);
}

///////////////////////////////////////////////////////////////////////////////
////// Mountain noise Hightmap 
///////////////////////////////////////////////////////////////////////////////
// "randomy" assing hight to a corner based on xz-coordinates descreatized by earlyer floor.  

/*
fn get_random_noize_corner(ij: vec2f) -> f32{
    //let PI: f32 = 3.14159265359;
    let uv: vec2f = 50.* fract(ij/3.14159265359);
    //float test = (uv.x*uv.y)*(uv.x + uv.y);
    let overflowTestFix: vec2f = uv*(uv.x + uv.y);
    let n_ij: f32 =  (2.*fract((overflowTestFix.x*overflowTestFix.y)) - 1.);
    return n_ij;
}
*/

// Numerically deterministic pseudo-random hash across GPU vendors, Gemini
/*
fn get_random_noize_corner(ij: vec2f) -> f32 {
    let p = fract(ij * vec2f(123.34, 456.21));
    let p2 = p + dot(p, p + 45.32);
    return fract(p2.x * p2.y) * 2.0 - 1.0;
}
*/

fn get_random_noize_corner(ij: vec2f) -> f32 {
    let noiseTextureSize = 256.0;
    let rayPosInUV = (ij + 0.5) / noiseTextureSize;// + 0.5 centers in the center of the texel
    return textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUV, 0.0).r;//* 2.0 - 1.0;
    //return textureSample(grayNoiseTexture, textureSampler, rayPosInUV, 0.0).r;
    /*
    textureSample() uses implicit derivatives (dpdx/dpdy) to determine the appropriate mip level. Those derivatives are available in the fragment stage, but not in the vertex stage.
    By adding 0.0 we can use it in vertex shader.
    */
}


/*
fn get_random_noize_corner(ij: vec2i) -> f32 {
    let rayPosInUV = vec2i(ij.x&255,ij.y&255);/// noiseTextureSize; // + 0.5 centers in the center of the texteö
    return textureLoad(grayNoiseTexture, rayPosInUV, 0).r* 2.0 - 1.0;
}
*/

/*
// this didnot seam to give boost in ms...
fn get_random_noize_corner(ij: vec2f) -> f32{
    return fract(sin(dot(ij, vec2f(16.9675,70.5433))*533442.63442321));
}
*/


// generate the four coners depending on xz-coordinates and then SmoothStep between them


fn f(p: vec2f)-> f32{
    let noiseTextureSize = 256.0;
    let rayPosInUV = (p + 0.5) / noiseTextureSize;// + 0.5 centers in the center of the texel
    return textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUV, 0.0).r;
}
/*
fn f(p: vec2f)-> f32{

    //float i = ij.x;
    //float j = ij.y; // is z cordinate
    var fr: vec2f = fract(p);

    // creat a descreat grid
    var ij: vec2f = floor(p); // (i,j) = floor(x,z)

    // b_i-1,j = c_i,j-1 = d_i-1,j-1 = a_i,j
    // b_i-1,j = a_i,j => b_i,j = a_i+1,j
    let a_ij: f32 = get_random_noize_corner(ij);
    let b_ij: f32 = get_random_noize_corner(ij+vec2f(0., 1.));
    let c_ij: f32 = get_random_noize_corner(ij+vec2f(1., 0.));
    let d_ij: f32 = get_random_noize_corner(ij+vec2f(1., 1.));
    /*
    // creat a descreat grid
    var ij: vec2i = vec2i(floor(p)); // (i,j) = floor(x,z)

    // b_i-1,j = c_i,j-1 = d_i-1,j-1 = a_i,j
    // b_i-1,j = a_i,j => b_i,j = a_i+1,j
    let a_ij: f32 = get_random_noize_corner(ij);
    let b_ij: f32 = get_random_noize_corner(ij+vec2i(0, 1));
    let c_ij: f32 = get_random_noize_corner(ij+vec2i(1, 0));
    let d_ij: f32 = get_random_noize_corner(ij+vec2i(1, 1));
    */

    let s_x: f32 = S(fr.x);
    let s_z: f32 = S(fr.y);
    let ba_difz: f32 = (b_ij - a_ij)*s_z;
    let ca_difx: f32 = (c_ij - a_ij)*s_x;
    let abcd_dif: f32 = (a_ij - b_ij - c_ij + d_ij);

    return (a_ij + ba_difz + ca_difx + abcd_dif*s_x*s_z);
}


fn mountainNormal(p: vec3f, normls_layers: i32) -> vec3f{
    //const float EPS = 0.000001;
    let EPS: f32 = 0.005;//0.0005;
    
    /*
    n = Cross_product( (1., df/dz ,0.) , (0., df/dx ,1.) =>
    n = ( -df/dx , 1. , -df/dz )
    */
    
    // vec3(x, y, z)
    
    // Sample the ground sdf in x-axis and z-axis
    let x0_sampl: f32 = SDF_scene(p + vec3f(EPS, 0.0, 0.0),normls_layers,false).sd;
    let x1_sampl: f32 = SDF_scene(p - vec3f(EPS, 0.0, 0.0),normls_layers,false).sd;
    
    let z0_sampl: f32 = SDF_scene(p + vec3f(0.0, 0.0, EPS),normls_layers,false).sd;
    let z1_sampl: f32 = SDF_scene(p - vec3f(0.0, 0.0, EPS),normls_layers,false).sd;
    
    // getting the difference/change in x-axis and z-axis
    let neg_dx: f32 = x0_sampl - x1_sampl ; // = -( x0_sampl - x1_sampl)
    let neg_dz: f32 = z0_sampl - z1_sampl; // = -( z0_sampl - z1_sampl)
    
    return normalize(vec3f(neg_dx, EPS, neg_dz));
}
*/
fn f_df(p: vec2f) -> vec3f{
    let noiseTextureSize = 256.0;
    let rayPosInUV = (p ) / noiseTextureSize;// + 0.5 centers in the center of the texel
    let f_val:f32 = textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUV, 0.0).r;
    let offset: vec2f = vec2f(1.0, 0.0);

    let rayPosInUVX = ((p+offset.xy)) / noiseTextureSize;
    let px0:f32 = textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUVX, 0.0).r;
    let rayPosInUVX_min = ((p-offset.xy)) / noiseTextureSize;
    let px1:f32 = textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUVX_min, 0.0).r;


    let rayPosInUVY = ((p+offset.yx)) / noiseTextureSize;
    let py0:f32 = textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUVY, 0.0).r;
    let rayPosInUVY_min = ((p-offset.yx)) / noiseTextureSize;
    let py1:f32 = textureSampleLevel(grayNoiseTexture, textureSampler, rayPosInUVY_min, 0.0).r;

    let dx = px0 - px1;
    let dy = py0 - py1;
    let df = vec2f(dx,dy);

    return vec3f(f_val,df);
}
/*
fn f_df(p: vec2f) -> vec3f{

    // creat a descreat grid

    //float i = ij.x;
    //float j = ij.y; // is z cordinate
    var fr: vec2f = fract(p);

    // creat a descreat grid
    var ij: vec2f = floor(p); // (i,j) = floor(x,z)

    // b_i-1,j = c_i,j-1 = d_i-1,j-1 = a_i,j
    // b_i-1,j = a_i,j => b_i,j = a_i+1,j
    let a_ij: f32 = get_random_noize_corner(ij);
    let b_ij: f32 = get_random_noize_corner(ij+vec2f(0., 1.));
    let c_ij: f32 = get_random_noize_corner(ij+vec2f(1., 0.));
    let d_ij: f32 = get_random_noize_corner(ij+vec2f(1., 1.));
    /*
    // creat a descreat grid
    var ij: vec2i = vec2i(floor(p)); // (i,j) = floor(x,z)

    // b_i-1,j = c_i,j-1 = d_i-1,j-1 = a_i,j
    // b_i-1,j = a_i,j => b_i,j = a_i+1,j
    let a_ij: f32 = get_random_noize_corner(ij);
    let b_ij: f32 = get_random_noize_corner(ij+vec2i(0, 1));
    let c_ij: f32 = get_random_noize_corner(ij+vec2i(1, 0));
    let d_ij: f32 = get_random_noize_corner(ij+vec2i(1, 1));
    */

    let s_x: f32 = S(fr.x);
    let s_z: f32 = S(fr.y);
    let ba_difz: f32 = (b_ij - a_ij)*s_z;
    let ca_difx: f32 = (c_ij - a_ij)*s_x;
    let abcd_dif: f32 = (a_ij - b_ij - c_ij + d_ij);

    let f_val: f32 =( a_ij + ba_difz + ca_difx + abcd_dif*s_x*s_z);

    let s_dx: f32 = d_S(fr.x);
    let s_dz: f32 = d_S(fr.y);
    
    var df: vec2f = vec2f(0.0);
    df.x = (c_ij - a_ij)* s_dx + abcd_dif* s_dx * s_z;
    df.y = (b_ij - a_ij)* s_dz + abcd_dif*s_x*s_dz;
     
    return vec3f(f_val,df);
}*/

fn N_func(p: vec2f, coord_scale: f32, layers: i32) -> f32{
    //return f(p);
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

fn N_deriv(p: vec2f, coord_scale: f32, layers: i32) -> vec3f{
    //return f_df(p);
    //var rot: mat2x2<f32> = mat2x2<f32>(0.8, -0.6, 0.6, 0.8);//mat2(4./5., -3./5., 3/5, 4/5);
    
    var rot: mat2x2<f32> = mat2x2<f32>(0.8, 0.6, -0.6, 0.8);
  
    var pr: vec2f = p * coord_scale;// * 0.002; // 1/2000
    
    var N_val: vec3f = f_df(pr);
    var val: vec3f = vec3f(0.0);
    
    //mat2 cumlativ_rot = mat2(0.1, 0.0, 0.0, 0.1);
    var cumlativ_rot: mat2x2<f32> = rot; // mat2(0.1, 0.0, 0.0, 0.1);
    
    //pr = 2. * (rot *pr);
    //pr = cumlativ_rot * pr;
    
    var a: f32 = 0.5;
    var tmp_df_val: vec2f;

    for(var i: i32 = 1; i < layers; i++){
        val = f_df(pr);
        
        tmp_df_val = val.yz * cumlativ_rot;  // need to rotate derivatives
        val.y = tmp_df_val.x;
        val.z = tmp_df_val.y;

        N_val += (val * a);///(1.0+dot(val.yz,val.yz));
        
        // decreas amplitue
        a *= 0.5;
        // rotate and double frequency
        pr = 2.0 * (pr * rot);// we don't want to cumulativly totate because pr saves state cumlativ_rot * pr;
        cumlativ_rot = 2.0 * (rot * cumlativ_rot);
   }
   
   // scale xz-derivatives to fit cordinate scales
   
   N_val.y *= coord_scale;
   N_val.z *= coord_scale;
   
   return vec3f(N_val.x, N_val.yz);
   
}

fn sdPlane_enviornment(p: vec3f, offset: f32, layers: i32) -> f32{
    
    var pr: vec2f = p.xz;
    var coord_scale: f32 = 0.002;
    
    var N: f32 = N_func(p.xz, coord_scale, layers);
   
    let height_multiplier: f32 = 600.0*0.7;//600.0*0.6;//* 0.3;
    
    // Explicit Distance Calculation
    //return (p.y - (N * height_multiplier + 600.0*0.6) - offset);//180.0); 
    return (p.y - (N * height_multiplier + height_multiplier) - offset);//180.0); 
}

/////// JAG ÄR HÄR!!!!!!!!!!!!!!!!!!!!
fn sd_derivPlane_enviornment(p: vec3f, offset: f32, layers: i32) -> vec4f{

    let coord_scale: f32 = 0.002;

    var val: vec3f = N_deriv(p.xz,coord_scale,layers);
   
    var h: f32 = val.x;
    var deriv: vec2f = val.yz; 
    
    var height_multiplier: f32 = 600.0*0.7;// * 0.3;
    
    // Explicit Distance Calculation
    var d: f32 = p.y - (h * height_multiplier + height_multiplier);//180.0); 
    
    // Account for global multipliers and initial coordinate scaling
    deriv = height_multiplier * deriv;
    
    // The surface normal for y - H(x,z) = 0 is defined as (-dH/dx, 1.0, -dH/dz)
    var normal: vec3f = normalize(vec3(-deriv.x, 1.0, -deriv.y));
    
    return vec4f(d, normal);
}

fn bdPlane_enviornment(p: vec3f, offset: f32,layers: i32,is_analytic: bool,color: vec4f) -> Body{
    
    if (is_analytic){
        var N:vec4f = sd_derivPlane_enviornment(p,offset,layers);
        return Body( N.x, N.yzw, color, GRASS_MOUNTAIN_TYPE);
    }
    
    return Body( sdPlane_enviornment(p,offset,layers),vec3f(0.0), color, GRASS_MOUNTAIN_TYPE);
}

fn grass_col(b: Body, n: vec3f, lightdir: vec3f, green: vec3f, light_green: vec3f) -> vec3f{
          // having light brown with this smooth step might be a nize sun?
          // float nv_sdot = smoothStep(1.,0.,dot(n,-lightdir));

    var nv_sdot: f32 = max(dot(n*0.65,-lightdir),0.); // -s
          // mountain_diff = vec3(5.,-8.,-31.)
          //vec3 light_mountain = vec3(250./256., 210./256., 164./256.);
    var light_mountain: vec3f = vec3f(255./256., 202./256., 133./256.);
    var Mlight_bounce: vec3f = min(nv_sdot*0.5,0.25) * light_mountain;
          
    var green_col: vec3f = vec3f(155./256., 163./256., 0.);
    var rgb_gr_bounce: vec3f = vec3f(160./256.,155./256.,0.);
    var Glight_bounce: vec3f = min(nv_sdot*10.,1.0) * green;//0.75*nv_sdot*rgb_gr_bounce;
          
          //"this creats snow"
          //vec3 Glight_bounce = min(nv_sdot*10.,1.0) * color+0.5;
          
    // rgb_green(178, 186, 0), darker green rgb(155, 163, 0)
    
    // rgb_snow(255, 250, 250)
    //vec3 green = vec3(256./256., 256./256., 256./256.);//snow
    
    //float gr_const = smoothStep(0.15,0.4, n.y);// when mountainNormal on
    let gr_const: f32 = smoothStep(0.5,0.6, n.y);
    
    return ( (b.col.rgb+Mlight_bounce)*(1.- gr_const) + (green + Glight_bounce)*gr_const);
}

fn fog_col(col: vec3f, t: f32) -> vec3f{

    // vec4(226./256., 226./256., 226./256.)
    //vec3 gray_col = vec3(0.8828125, 0.8828125, 0.8828125);
    var gray_col: vec3f = vec3f(0.7828125, 0.7828125, 0.7828125);
    let fog_const: f32 = -0.000050;
    
    // add Fog color
    var fog_rgb: vec3f = vec3(exp(fog_const*t),exp(fog_const*t*2.), exp(fog_const*t*4.));
    
    return fog_rgb*(col) + (1. - fog_rgb)*gray_col;
}

fn bdsky(p: vec3f, rd_y: f32) -> Body{
    //float tmp_rdy = normalize(p).y
    //vec3 gray = vec3(0.8828125, 0.8828125, 0.8828125);
    var blue: vec4f = vec4f(145./256., 197./256., 250./256.,1.);
    var color: vec3f = blue.rgb;
    // test first if we hit then if yes then do compilcated calc
    var sky: Body = bdPlane(p, 2500., blue);
    if(sky.sd < 0.0001){
        //N_func(vec2 p, float coord_scale, int layers)
        //smoothStep(float a, float b, float x)
        let cl_const: f32 = smoothStep(-1.,1., N_func( p.xz, 0.001, 4));
        color = (blue.rgb - 0.6*max(rd_y, 0.0))*(cl_const-1.) + cl_const;
    }else{
        color = blue.rgb - 0.6*max(rd_y, 0.0);
    }
    sky.col = vec4f(color,1.);
    //N_func(vec2 p, float coord_scale, int layers)
    //color - 0.6*max(rd.y, 0.0);
    
    return sky;
}

///////////////////////////////////////////////////////////////////////////////
////// SDF scene function
///////////////////////////////////////////////////////////////////////////////
// could this consitute ultra/mega shader and uses more registers than needed?
fn SDF_scene( p:vec3f, layers: i32, is_analytic: bool) -> Body{
    
    //vec3 spherepos = vec3(0., 500., 0.);
    //body sphere = bdSphere(p ,spherepos,vec4(0.541,1.,0.549,1.));
    
    //body sun = bdSun(p, lightPos, suncolor);
    
    // vec3 p,float offset,vec4 color
    //vec4 green = vec4(0.541,1.,0.549,1.);
    
    // "collision" with sky will stall for slow mountain collision???
    //body sky = bdPlane(p, 2500., vec4(0.541,1.,0.549,1.));

    // vec4(250./256., 210./256., 164./256., 1.);
    let mount_col: vec4f = vec4(0.9765625, 0.8203125, 0.640625, 1.);
 
    var mountains: Body = bdPlane_enviornment(p , 0., layers,is_analytic,mount_col);
    
    /*
    if(sky.sd < mountains.sd){
        return sky;
    }else{
        return mountains;
    }
    */
    return mountains;
    /*
    if(sphere.sd < mountains.sd){
        return sphere;
    }else{
        return mountains;
    }
    */
    
    //return min (Sphere.sd, bdGround().sd);

}

///////////////////////////////////////////////////////////////////////////////
////// Normal calculations
///////////////////////////////////////////////////////////////////////////////
// faster mountain normals, (only sample sdf 4 times and not 6 as in general normal calculations)
fn mountainNormal(p: vec3f, normls_layers: i32) -> vec3f{
    //const float EPS = 0.000001;
    let EPS: f32 = 0.005;//0.0005;
    
    /*
    n = Cross_product( (1., df/dz ,0.) , (0., df/dx ,1.) =>
    n = ( -df/dx , 1. , -df/dz )
    */
    
    // vec3(x, y, z)
    
    // Sample the ground sdf in x-axis and z-axis
    let x0_sampl: f32 = SDF_scene(p + vec3f(EPS, 0.0, 0.0),normls_layers,false).sd;
    let x1_sampl: f32 = SDF_scene(p - vec3f(EPS, 0.0, 0.0),normls_layers,false).sd;
    
    let z0_sampl: f32 = SDF_scene(p + vec3f(0.0, 0.0, EPS),normls_layers,false).sd;
    let z1_sampl: f32 = SDF_scene(p - vec3f(0.0, 0.0, EPS),normls_layers,false).sd;
    
    // getting the difference/change in x-axis and z-axis
    let neg_dx: f32 = x0_sampl - x1_sampl ; // = -( x0_sampl - x1_sampl)
    let neg_dz: f32 = z0_sampl - z1_sampl; // = -( z0_sampl - z1_sampl)
    
    return normalize(vec3f(neg_dx, EPS, neg_dz));
}


// General normal calculation
fn CalculateNormal(p: vec3f, normls_layers: i32) -> vec3f{
    let EPS: f32 = 0.005;//0.0005;//0.0005;//0.000001;
    
    let v1: vec3f = vec3f(
        SDF_scene(p + vec3f(EPS, 0.0, 0.0),normls_layers,false).sd,
        SDF_scene(p + vec3f(0.0, EPS, 0.0),normls_layers,false).sd,
        SDF_scene(p + vec3f(0.0, 0.0, EPS),normls_layers,false).sd);
        
    let v2: vec3f = vec3f(
        SDF_scene(p - vec3f(EPS, 0.0, 0.0),normls_layers,false).sd,
        SDF_scene(p - vec3f(0.0, EPS, 0.0),normls_layers,false).sd,
        SDF_scene(p - vec3f(0.0, 0.0, EPS),normls_layers,false).sd);

    return normalize(v1-v2);
}


///////////////////////////////////////////////////////////////////////////////
////// bisection
///////////////////////////////////////////////////////////////////////////////
fn bisection(ro: vec3f,   dr: vec3f,
            old_t: f32,   t: f32,
            MaxIter: i32, EPS: f32,
            layers: i32, is_analytic: bool) -> vec3f{
        
        var c: vec3f;
        var ct: f32 = 0.;
        var oldSign: f32 = 1.;//sign(SDF_scene(ro + dr*old_t, layers, is_analytic).sd);
        

        var tmp_old_t: f32 = old_t;
        var tmp_t: f32 = t;
        for (var i: i32 = 0; i < MaxIter; i++){
        
           ct = (tmp_old_t + tmp_t) * 0.5;
           c = ro + dr*ct;
    
           var c_body: Body = SDF_scene(c,layers, is_analytic);

    		
           //if(abs((ct-old_t)/ct) <= EPS*abs(ct)) break;
           if ((t - tmp_old_t) < EPS){ break;}

           if (oldSign != sign(c_body.sd)){
               tmp_t = ct;
           } else {
               tmp_old_t = ct;
               //a_body = image(ro + dr*old_t);
           }

        }
        
        return c;
}

///////////////////////////////////////////////////////////////////////////////
////// Shadow calculations
///////////////////////////////////////////////////////////////////////////////
// function used for soft shadow approximation of penumbra/ soft shadows
fn Rmin(currentMinDist: f32,h: f32,t: f32) -> f32{
    //return min(currentMinDist,(8. * h/t));
    //return min(currentMinDist,(32. * h/t));
    return min(currentMinDist,(16. * h/t)); // (32. * h/t);
}


// function for if mountain is in shade, aka hard shadow
fn sh(p: vec3f, lightDir: vec3f, EPS: f32, layers: i32) -> f32{
    
    var stepsize: f32= 0.;
    var t: f32 = EPS*10.;//0.5; // shadow acne stepout
    //vec3 lightDir = normalize(l - p);
    var hp: vec3f = vec3f(0.0);
    var d: f32 = 0.0;
    var minDist: f32 = 1.;
    var old_t: f32 = t;
    for(var i: i32 = 0; i < 600; i++){
        hp = p + lightDir*t;
        d = sdPlane_enviornment(hp,0.,layers);
        minDist = Rmin(minDist, d, t); 
        
        if(t > 2500.0){
            break;
        }
            
        if( d <= 0.0005 *min(f32(i*i),1000.) ) {//*min(float(i*i),10.) min(f32(i*i),1000.)
            return 0.;
        }
        old_t = t;
        stepsize = d*0.3;//max(d * 0.4, 0.001);
        t += stepsize;
       
    }
    return smoothStep(0.,1., minDist ); 
}

///////////////////////////////////////////////////////////////////////////////
////// Light calculations
///////////////////////////////////////////////////////////////////////////////
fn DirectLight(b: Body, p: vec3f, n: vec3f, lightPos: vec3f, EPS: f32,
                shadow_layers: i32, normal_layers: i32,
                is_analytic: bool) -> f32{
    
    var ligtVec: vec3f = lightPos - p;
    var lightDir: vec3f = normalize(ligtVec);
    
    // spara maxDiffuse i body...
    var maxDiffuse: f32 = 1.;
    var Shadeing: f32 = 0.;
    if(b.material == 1u) {
        maxDiffuse = mix(1.0, 0.1, clamp(n.y, 0.0, 1.0));/*0.3*/
        Shadeing = sqrt(clamp(dot(n, lightDir), 0., maxDiffuse));
    } else {
        Shadeing = clamp(dot(n, lightDir), 0., maxDiffuse);
    }

    if(Shadeing == 0.){ return 0.0;}//Light*0.1;
    
    var squaredDist: f32 = dot(ligtVec,ligtVec);
      
    var lightAttenuation: f32 = (1./(squaredDist+0.0001));
      
    let Li: f32 = 46000000.;
    let PI: f32 = 3.14159265359;
    let BRDF: f32 = (1./PI);
    
    var Light: f32 = BRDF * (Li * lightAttenuation);
    
    //float Shadeing = clamp(dot(n, lightDir), 0., 1.);
    //if(Shadeing == 0.) return .0;//Light*0.1; // should we even multiply with the light here? If Shadeing clamped to 0.1
    
    // if the light is this small then we don't need to do shading or shadow calculations
    if(Light <= 0.1){ return 0.1;}
    
	var Shadow: f32 = sh(p+n*0.1, lightDir, EPS, shadow_layers); //+n*0.1

	return Light * Shadeing * Shadow;
}


///////////////////////////////////////////////////////////////////////////////
////// Raymarch
///////////////////////////////////////////////////////////////////////////////
fn Raymarch(ro: vec3f, rd: vec3f, start_t_distance: f32, start_index: i32)-> vec4f{
    
    //Sun posistion, using sperical coordinates
    var radius: f32 = 5000.; // radius of a hemishpere
    var sie: f32 = 1.4*sin((In.iTime)*0.3);
    var fi: f32 = 1.0;
    
    // rotation around hemispere
    var sun_angle: vec3f = normalize( vec3f(sin(sie)*sin(fi), cos(sie),sin(sie)*cos(fi)));
    var ligPos: vec3f = radius * sun_angle;
    
    let startDistance: f32 = start_t_distance;
    let startIndex: i32 = start_index;

    var b: Body;
    var p: vec3f;
    var old_t: f32 = 0.;
    var t: f32 = startDistance;
    var stepsize: f32 = 0.;
    var EPS_colid: f32 = 0.;
    var shadow_acne_EPS: f32 = 1.0;
    
    var is_analytic: bool = true;//false;// funkar inte med "normal_layer" (måste beräkna hela)
    var physical_layers: i32 = 10;
    var normal_layers: i32 = 10;
    
    // LOD verkar i
    /*
    bool lod_on = false;
    int low_LOD_layers = 5;
    int high_LOD_layers = 10;
    */
    
    // 145, 197, 250 "rgb sky blue"
    var col: vec4f = vec4f(145./256., 197./256., 250./256.,1.); // vec4(1, 1, 1, 1);                  
    //vec4 brown = vec4(0.9765625, 0.8203125, 0.640625, 1.);
    
    for(var i: i32 =startIndex; i<=2000; i++){ // why does increing these itteration make the background less incaptulate background color...
        
      // calculate a point on the ray 
      p = ro + rd*t;
      // minst vara 5 och max vara 8
      
      /*
      // möjligtvis dåligt för "threads" divergar för ofta
      if(lod_on){
          if(t >= 8000. && t < 4000. ){
               physical_layers = low_LOD_layers;//max(5, min(8, 20 / i));//physical_layers;
          } else if (t >= 4000.){
              physical_layers = (low_LOD_layers + high_LOD_layers) /2;
          }else{
               physical_layers = high_LOD_layers;//max(5, min(8, 20 / i));     
          }
          }
      */ 
      
      b = SDF_scene(p,physical_layers,false);
      var d: f32 = b.sd;
      
      EPS_colid = 0.005;
      // risk att terminera tidigt på längre avstånd och många itterationer
      //if(t >= 1000. || i >= 100) 
        //  {EPS_colid= 0.05;}
      
      if(d < 0.){  
            p = bisection(ro, rd, old_t, t, 5, 0.0005, physical_layers, is_analytic);  
            shadow_acne_EPS *= 4.; // need to look in to the best number for this..
        } 
        
      if(d <= EPS_colid*min(f32(i),10.)){ // *min(float(i),10.)
            p = bisection(ro, rd, old_t, t+EPS_colid, 5, 0.0005, physical_layers, is_analytic);  
            shadow_acne_EPS *= 4.;
            
            var n: vec3f = vec3f(0.0);
            if(is_analytic){
                
                // analytic normal
                n = SDF_scene(p,normal_layers,is_analytic).deriv;
            } else {
                //n = mountainNormal(p,normal_layers);
                n = CalculateNormal(p,normal_layers);
            }
          
            var l: f32 = DirectLight(b, p, n, ligPos, shadow_acne_EPS,
                                physical_layers, normal_layers, is_analytic);
                                
            var skyblue: vec3f = vec3f(145./256., 197./256., 250./256.)- 0.6*max(rd.y, 0.0);
            var sky_ref: vec3f = (1. + n.y)*0.05 * skyblue;//(1. + n.y)
          

            var green_col: vec3f = vec3f(155./256., 163./256., 0.);
            var ligtDir: vec3f = normalize(ligPos - p);
          
            b.col.r *= (l + sky_ref).r;  b.col.g *= (l + sky_ref).g;  b.col.b *= (l + sky_ref).b;
            green_col *= (l + sky_ref);
            var rgb_gr_bounce: vec3f = vec3f(160./256.,155./256.,0.);//* (l + sky_ref);
          
            var gr_col: vec3f = grass_col(b,n,ligtDir,green_col,rgb_gr_bounce);
            var LDir: vec3f = normalize(ligPos - ro);
            var sundot: f32 = clamp(dot(LDir,vec3f(0.,1.,0.)),0.,1.);
          // add fog color
            col = vec4f(fog_col(gr_col,t),1.);
          //col.rg += vec2(0.06,-0.06)* exp(-1./(1.-sundot));
          
            return col;
          //break;
      }
      
        old_t = t;
        
        t += d*0.3;//max(d*0.30, 0.001);// b.sd * 0.4;//max(b.sd, 0.0001);
        
        if (t > 8000.){ break; }
        /*
        if (p.y >= 2500. && p.y <= 2600.){
            float cl_sd = smoothStep(-1.,1., N_func(p.xz,0.001,2) );
            vec3 cloud_col = vec3(cl_sd);//vec3( (2.*(cl_sd+1.))) ;
            col.rgb = cloud_col;
        }
        */
    }
    
    /////// Google AI sun stuff:
    var LDir: vec3f = normalize(ligPos - ro);
    var sundot:f32 = clamp(dot(LDir,rd),0.,1.);
    var sun_const:f32 = pow(sundot,800.)*4.0;//max( dot(LDir,rd), 0.);
    var halo: f32 = pow(sundot,200.)*0.7;
    //*(1.-sun_const)
    // Standard Web/Design "Sun": rgb(252, 229, 112)
    //vec4 sunCol = vec4(252./256., 229./256., 112./256.,1.);
    var suntest:f32 = clamp(dot(LDir, vec3(0.,1.,0.)),0.,1.);
    //float redshift = 205./256. - 10.*exp(-1./(1.- suntest)) ;//min( (205./256 - rdyval) ,256.);
    var redshift: f32 = 205./256.;
    var sunColh: vec4f = vec4f(241./256., redshift , 108./256.,1.);
    var sunCol: vec4f = vec4f(1., 1. , 1.,1.) -exp(-1./(1.- suntest));
    //col -= exp(-1./(1.-max(suntest, 0.0)));
    var sky_dark_const: f32 = 1;// exp(-1./(1.-suntest) );
    return ( col - 0.6*max(rd.y, 0.0) - 0.4*(1.-suntest))*(1.-sun_const)*(1.-halo) + sunCol*sun_const + sunColh*halo;
}


///////////////////////////////////////////////////////////////////////////////
////// Main function
///////////////////////////////////////////////////////////////////////////////
//fn fs_main(@builtin(position) position: vec4f) -> @location(0) vec4f {
@fragment
fn fs_main(intv: VertexOutput) -> @location(0) vec4f {

    let uv: vec2f = ((2.0*intv.position.xy) - In.iResolution) / In.iResolution.y;
    var m: vec2f = In.iMouse;
    /*
    let uv = position.xy / In.iResolution;
    var pixel_pos = vec3f(uv*2.0 - 1.0, 0.0);
    let resolution_ratio = (In.iResolution.x/In.iResolution.y);
    pixel_pos *= resolution_ratio;
    */

    var base_ro: vec3f = vec3f(0.0, 1500.0, 0.0);
    var forward: vec3f = vec3f(0.0, 0.0, 1.0);

    var forw_yz: vec2f = forward.yz *  rot2D(-m.y);
    var forw_xz: vec2f = forward.xz *  rot2D(-m.x);
    forward.y = -forw_yz.x;
    //forward.z = forw_yz.y;
    forward.x = forw_xz.x;
    forward.z = forw_xz.y;

    var ro: vec3f = base_ro - forward * 1000.0;
    
    var rd: vec3f = raydir(uv, m);
    rd.y *= -1.0; // flip y coordinate because it's fliped in wgsl
    var fragColor: vec4f =  Raymarch(ro,rd,intv.coneDist,i32(intv.coneIter));
    //var fragColor: vec4f =  Raymarch(ro,rd,0.0,0);

    return vec4f(smoothStepVec3(0.0, 1.0, fragColor.rgb), 1.0); // does rgb work?
}
