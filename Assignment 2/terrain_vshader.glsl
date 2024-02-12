R"(
#version 330 core

in vec3 vposition;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform vec3 viewPos; 

// clip plane
uniform vec3 clipPlaneNormal;
uniform float clipPlaneHeight;

out vec2 uv;
out vec3 fragPos;
out vec3 normal;
out float height;
out float slope;
out vec3 distanceFromCamera;

//Perlin noise on GPU
float Perlin2D( vec2 P ) {
    //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin2D.glsl

    // establish our grid cell and unit position
    vec2 Pi = floor(P);
    vec4 Pf_Pfmin1 = P.xyxy - vec4( Pi, Pi + 1.0 );

    // calculate the hash
    vec4 Pt = vec4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += vec2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    vec4 hash_x = fract( Pt * ( 1.0 / 951.135664 ) );
    vec4 hash_y = fract( Pt * ( 1.0 / 642.949883 ) );

    // calculate the gradient results
    vec4 grad_x = hash_x - 0.49999;
    vec4 grad_y = hash_y - 0.49999;
    vec4 grad_results = inversesqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

    // Classic Perlin Interpolation
    grad_results *= 1.4142135623730950488016887242097;  // scale things to a strict -1.0->1.0 range  *= 1.0/sqrt(0.5)
    // Quintic interpolation
    vec2 blend = Pf_Pfmin1.xy * Pf_Pfmin1.xy * Pf_Pfmin1.xy * (Pf_Pfmin1.xy * (Pf_Pfmin1.xy * 6.0 - 15.0) + 10.0);
    vec4 blend2 = vec4( blend, vec2( 1.0 - blend ) );
    return dot( grad_results, blend2.zxzx * blend2.wwyy );
}


//FBM, adapted from MusgraveTerrain00 and noise.h in Terrains
float fBm( vec2 P ) {
    // fBm parameters - Summer
    float H = 0.9f;
    float lacunarity = 2.0f;
    float offset = 0.2f;
    const int octaves = 5;
    float value = 0.0;
    float signal = 0.0f;
    float f = 1.0f;
    float w = 0.175f;

    // Spectral construction
    for (int i=0; i<octaves; i++) {
        if (w > 1.0){
            w = 1.0;
        } 
        // Arrays are not supported by openGL, so the exponent is not calculated ahead of time
        signal += (Perlin2D( P )+w) * pow(f, -H);
        value += w*signal;
        w *= signal;
        P.x *= lacunarity;
        P.y *= lacunarity;
        f *= lacunarity;
    }
    return(value);
}



void main() {
    // displace the position so that we get an infinite world
	vec3 position = vposition + (vec3(viewPos.x, viewPos.y, 0)); 

    // tex coords
    uv = (position.xy + 20/2) / 20; 

    // tile the textures to improve the resolution => instead of [0, 1], we use [0, 40]
    int num_tiles = 40; 
    uv = (uv * num_tiles);

    // Calculate height. 
    float h = fBm(position.xy);
    height = h*1.5;
    // give new disturbed position
    fragPos = vposition.xyz + vec3(0,0,h);
    gl_Position = P*V*M*vec4(fragPos, 1.0f);

    // Calculate the vertex normals
    vec3 A = vec3(position.x + 1.0f, position.y, Perlin2D(position.xy + (1, 0)));
    vec3 B = vec3(position.x - 1.0f, position.y, Perlin2D(position.xy + (-1, 0)));
    vec3 C = vec3(position.x, position.y + 1.0f, Perlin2D(position.xy + (0, 1)));
    vec3 D = vec3(position.x, position.y - 1.0f, Perlin2D(position.xy + (0, -1)));
    vec3 n = normalize( cross(A - B , C - D) );
    normal = n;

    // the up vector is (0, 0, 1) so the dot of the top and the normal vector is normal.z
    // we take acos to get the actual gradient
    slope = acos(normal.z);

    // set distance from camera to scale the visibility
    distanceFromCamera = fragPos - viewPos;

    // set clipping plane
    vec4 plane = vec4(clipPlaneNormal, clipPlaneHeight); 
    float distance_of_this_vertex = dot(plane, vec4(fragPos, 1.0));      
    gl_ClipDistance[0] = distance_of_this_vertex;
}
)"