float4x4 g_mWorld;                  // World matrix for object
float4x4 g_mWorldViewProjection;    // World * View * Projection matrix

// permutation table
static int permutation[] = 
{
151,160,137,91,90,15,
131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};
// gradients for 3D noise
static float3 g[] = {
    1,1,0,
    -1,1,0,
    1,-1,0,
    -1,-1,0,
    1,0,1,
    -1,0,1,
    1,0,-1,
    -1,0,-1, 
    0,1,1,
    0,-1,1,
    0,1,-1,
    0,-1,-1,
    1,1,0,
    0,-1,1,
    -1,1,0,
    0,-1,-1,
};

texture permTexture;
texture gradTexture;

// Generate permutation and gradient textures using CPU runtime
//texture permTexture
//<
  //string texturetype = "2D";
  //string format = "l8";
  //string function = "GeneratePermTexture";
  //int width = 256, height = 1;
//>;
//
//texture gradTexture
//<
  //string texturetype = "2D";
  //string format = "q8w8v8u8";
  //string function = "GenerateGradTexture";
  //int width = 16, height = 1;
//>;
//

float4 GeneratePermTexture(float p : POSITION) : COLOR
{
  return permutation[p * 256] / 255.0;
}

float3 GenerateGradTexture(float p : POSITION) : COLOR
{
  return g[p * 16];
}

sampler permSampler = sampler_state
{
  texture = <permTexture>;

  AddressU  = Wrap;

  AddressV  = Clamp;

  MAGFILTER = POINT;

  MINFILTER = POINT;

  MIPFILTER = NONE;
};




sampler gradSampler = sampler_state
{
  texture = <gradTexture>;

  AddressU  = Wrap;

  AddressV  = Clamp;

  MAGFILTER = POINT;

  MINFILTER = POINT;

  MIPFILTER = NONE;
};


float3 fade(float3 t)
{
  return t * t * t * (t * (t * 6 - 15) + 10); // new curve
//  return t * t * (3 - 2 * t); // old curve
}

float perm(float x)
{
  return tex1D(permSampler, x / 256.0) * 256;
}

float grad(float x, float3 p)
{
  return dot(tex1D(gradSampler, x), p);
}


float inoise(float3 p)
{
  float3 P = fmod(floor(p), 256.0);

  p -= floor(p);

  float3 f = fade(p);

  // HASH COORDINATES FOR 6 OF THE 8 CUBE CORNERS
  float A = perm(P.x) + P.y;

  float AA = perm(A) + P.z;

  float AB = perm(A + 1) + P.z;

  float B =  perm(P.x + 1) + P.y;

  float BA = perm(B) + P.z;

  float BB = perm(B + 1) + P.z;



  // AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
  return lerp(
    lerp(lerp(grad(perm(AA), p),

              grad(perm(BA), p + float3(-1, 0, 0)), f.x),

         lerp(grad(perm(AB), p + float3(0, -1, 0)),

              grad(perm(BB), p + float3(-1, -1, 0)), f.x), f.y),

    lerp(lerp(grad(perm(AA + 1), p + float3(0, 0, -1)),

              grad(perm(BA + 1), p + float3(-1, 0, -1)), f.x),

         lerp(grad(perm(AB + 1), p + float3(0, -1, -1)),

              grad(perm(BB + 1), p + float3(-1, -1, -1)), f.x), f.y),

    f.z);
}

// fractal sum
float fBm(float3 p, int octaves, float lacunarity = 2.0, float gain = 0.5)
{
	float freq = 1.0, amp = 0.5;
	float sum = 0;	
	for(int i=0; i<octaves; i++) {
		sum += inoise(p*freq)*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 pos   : POSITION;   // vertex position 
	float4 Diffuse    : COLOR0; 
    float2 tex  : TEXCOORD0;  // vertex texture coords 
};


//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT VS2( float4 vPos : POSITION, float3 vNormal : NORMAL, float2 vTexCoord0 : TEXCOORD0 )
{
    VS_OUTPUT Output;
   //float3 vNormalWorldSpace;
   // float4 vAnimatedPos = vPos;
    
    // Transform the position from object space to homogeneous projection space
    Output.pos = mul(vPos, g_mWorldViewProjection);
   
    // Transform the normal from object space to world space    
    Output.Diffuse.xyz = mul(vPos, g_mWorld);
   //Output.Diffuse.xyz = normalize( mul(vNormal, (float3x3)g_mWorld) );

	Output.Diffuse.w = 1;
	Output.tex = vTexCoord0;

    return Output;    
}
//-----------------------------------------------------------------------------
// Pixel Shader: Combine
// Desc: Combine the source image with the original image.
//-----------------------------------------------------------------------------
float4 PS2( VS_OUTPUT In ) : COLOR0
{
	//return GeneratePermTexture(In.tex.x) + GeneratePermTexture(In.tex.y);
	return fBm(float3(In.tex.x, In.tex.y,0),4);
}




//-----------------------------------------------------------------------------
// Technique: PostProcess
// Desc: Performs post-processing effect that down-filters.
//-----------------------------------------------------------------------------
technique PostProcess
{
    pass p0
    {
        VertexShader = compile vs_3_0 VS2();
        PixelShader = compile ps_3_0 PS2();
    }
}
