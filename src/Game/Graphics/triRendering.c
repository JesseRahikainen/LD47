#include "triRendering.h"

#include <stdlib.h>

#include "glPlatform.h"

#include "../Math/matrix4.h"
#include "camera.h"
#include "shaderManager.h"
#include "glDebugging.h"
#include "../System/platformLog.h"
#include "../Math/mathUtil.h"

typedef struct {
	Vector3 pos;
	Color col;
	Vector2 uv;
} Vertex;

typedef struct {
	GLuint vertexIndices[3];
	float zPos;
	uint32_t camFlags;

	GLuint texture;
	float floatVal0;

	ShaderType shaderType;

	//int scissorID;
	int stencilGroup; // valid values are 0-7, anything else will cause it to be ignored
} Triangle;

/*
Ok, so what do we want to optimize for?
I'd think transferring memory.
So we have the vertices we transfer at the beginning of the rendering
Once that is done we generate index buffers to represent what each camera can see
*/
#define MAX_SOLID_TRIS 2048
#define MAX_TRANSPARENT_TRIS 2048
#define MAX_STENCIL_TRIS 256

#define MAX_TRIS ( MAX( MAX_SOLID_TRIS, MAX( MAX_TRANSPARENT_TRIS, MAX_STENCIL_TRIS ) ) )

typedef struct {
	//Vertex startVertices[MAX_VERTS];
	//Vertex endVertices[MAX_VERTS];

	int triCount;
	int vertCount;

	Triangle* triangles;
	Vertex* vertices;
	GLuint* indices;

	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	int lastTriIndex;
	int lastIndexBufferIndex;
} TriangleList;

TriangleList solidTriangles;
TriangleList transparentTriangles;
TriangleList stencilTriangles;


// don't want to use a full sized triangle list, maybe have at most 4 tris
//  so first we'd like to make the TrianglList structure have dynamic sized
//  arrays
// what if we chain them together somehow, we can define a render as to the
//  stencil buffer
//
// create stencil group
// draw things, assigning the stencil group, optionally make them render to the stencil buffer
// when we sort triangles we then group things together by the stencil group, with those that
// render to the stencil put first
// when we're rendering a triangle that is to the stencil group we'll have to change the current
// state of the rendering, when we switch back the state will have to be changed back
// instead of creating a stencil group, we can just assign a stencil group (have it be an uint,
// a value of 0 means no associated stencil group, any other number means there is a stencil
// group)
// but then, how do we do transparent and non-transparent triangles needing the same stencil
// group? for stencil triangles we can assume there is no transparency, but we can put them
// into the transparent array, that might not work too well
// it's almost like we want to split up the rendering so we have a transparent and non-transparent
// list per stencil group, then we can render to the stencil buffer, then just draw all the
// triangles in the group
// given the static array sizes, i would prefer not to do that (with what we have right now)
// we could sort inside the arrays, so they're grouped interally by stencil group, but that
// will break the transparency rendering
// we could say it only works on non-transparent images, but that seems like laziness
// if we could use order independent transparency rendering then we could use the masking
// as is, we could also remove one of the triangle groups completely as well
// but nothing about OIT is easy
// 
// could we have the stencil groups rendered first, each writing a different value to the
// stencil buffer, then if the triangle has a stencil group associated with it we can
// only render if it's that value?
// before rendering any of the transparent or non-transparent triangles we render the stencil
// triangles, with each being set to to a value based on the stencil group (an unsigned byte)
// then we can sort the triangles as before, but there's another state to swtich between,
// based on the stencil group id for each triangle
// we can't do it with just values, as we would then overwrite things, so we'd have to use
// a bit mask, with an 8 bit stencil buffer we could support up to 8 different masks at once
// 
// so we then have three sets of triangles instead of two, the stencil triangles, the opaque
// triangles, and the transparent triangles.
// the first step of rendering is to to create the stencil buffer, we render all the stencil
// triangles
// glDepthFunc( GL_ALWAYS );
// glStencilFunc( GL_ALWAYS, 0xff, 0xff );
// glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
//
// glStencilMask( 1 << stencilGroup );
//
// what this should do, is make it so the rendering for that group will set the appropriate
// bit in the stencil buffer for that pixel
//
// then when rendering the triangles we either make it so the stencil always passes, or
// only passes when it equals a certain value
//
// glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
// glDepthFunc( GL_LESS );
//
// if( !usesStencil ) {
//   glStencilFunc( GL_ALWAYS, 0xff, 0xff ); // always pass
// } else {
//   glStencilFunc( GL_EQUAL, 0xff, ( 1 << stencilGroup ) );
// }
//
/*
if( isStencilTriangles ) {
  glStencilMask( 1 << stencilGroup );
} else {
  if( usesStencil ) {
    glStencilFunc( GL_EQUAL, 0xff, ( 1 << stencilGroup ) );
  } else {
    glStencilFunc( GL_ALWAYS, 0xff, 0xff );
  }
}
*/




// glStencilMask
// glStencilFunc
// glStencilOp




#define Z_ORDER_OFFSET ( 1.0f / (float)( 2 * ( MAX_TRIS + 1 ) ) )

static ShaderProgram shaderPrograms[NUM_SHADERS];

TriVert triVert( Vector2 pos, Vector2 uv, Color col )
{
	TriVert v;
	v.pos = pos;
	v.uv = uv;
	v.col = col;
	return v;
}

int triRenderer_LoadShaders( void )
{
	llog( LOG_INFO, "Loading triangle renderer shaders." );
	ShaderDefinition shaderDefs[5];
	ShaderProgramDefinition progDefs[NUM_SHADERS];

	llog( LOG_INFO, "  Destroying shaders." );
	shaders_Destroy( shaderPrograms, NUM_SHADERS );

	// Sprite shader
	shaderDefs[0].fileName = NULL;
	shaderDefs[0].type = GL_VERTEX_SHADER;
	shaderDefs[0].shaderText = DEFAULT_VERTEX_SHADER;

	shaderDefs[1].fileName = NULL;
	shaderDefs[1].type = GL_FRAGMENT_SHADER;
	shaderDefs[1].shaderText = DEFAULT_FRAG_SHADER;

	// for rendering fonts
	shaderDefs[2].fileName = NULL;
	shaderDefs[2].type = GL_FRAGMENT_SHADER;
	shaderDefs[2].shaderText = FONT_FRAG_SHADER;

	// simple sdf rendering
	shaderDefs[3].fileName = NULL;
	shaderDefs[3].type = GL_FRAGMENT_SHADER;
	shaderDefs[3].shaderText = SIMPLE_SDF_FRAG_SHADER;

	// image sdf rendering
	shaderDefs[4].fileName = NULL;
	shaderDefs[4].type = GL_FRAGMENT_SHADER;
	shaderDefs[4].shaderText = IMAGE_SDF_FRAG_SHADER;


	progDefs[0].fragmentShader = 1;
	progDefs[0].vertexShader = 0;
	progDefs[0].geometryShader = -1;

	progDefs[1].fragmentShader = 2;
	progDefs[1].vertexShader = 0;
	progDefs[1].geometryShader = -1;

	progDefs[2].fragmentShader = 3;
	progDefs[2].vertexShader = 0;
	progDefs[2].geometryShader = -1;

	progDefs[3].fragmentShader = 4;
	progDefs[3].vertexShader = 0;
	progDefs[3].geometryShader = -1;

	llog( LOG_INFO, "  Loading shaders." );
	if( shaders_Load( &( shaderDefs[0] ), sizeof( shaderDefs ) / sizeof( ShaderDefinition ),
		progDefs, shaderPrograms, NUM_SHADERS ) <= 0 ) {
		llog( LOG_ERROR, "Error compiling image shaders.\n" );
		return -1;
	}

	return 0;
}

#include "../System/memory.h"
static int createTriListGLObjects( TriangleList* triList )
{
	if( ( triList->triangles = mem_Allocate( sizeof( Triangle ) * triList->triCount ) ) == NULL ) {
		llog( LOG_ERROR, "Unable to allocate triangles array." );
		return -1;
	}

	if( ( triList->vertices = mem_Allocate( sizeof( Vertex ) * triList->vertCount ) ) == NULL ) {
		llog( LOG_ERROR, "Unable to allocate vertex array." );
		return -1;
	}

	if( ( triList->indices = mem_Allocate( sizeof( GLuint ) * triList->vertCount ) ) == NULL ) {
		llog( LOG_ERROR, "Unable to allocate index array." );
		return -1;
	}

	GL( glGenVertexArrays( 1, &( triList->VAO ) ) );
	GL( glGenBuffers( 1, &( triList->VBO ) ) );
	GL( glGenBuffers( 1, &( triList->IBO ) ) );
	if( ( triList->VAO == 0 ) || ( triList->VBO == 0 ) || ( triList->IBO == 0 ) ) {
		llog( LOG_ERROR, "Unable to create one or more storage objects for triangle rendering." );
		return -1;
	}

	GL( glBindVertexArray( triList->VAO ) );

	GL( glBindBuffer( GL_ARRAY_BUFFER, triList->VBO ) );
	GL( glBufferData( GL_ARRAY_BUFFER, sizeof( triList->vertices[0] ) * triList->vertCount, NULL, GL_DYNAMIC_DRAW ) );

	GL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, triList->IBO ) );
	GL( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( triList->indices[0] ) * triList->vertCount, triList->indices, GL_DYNAMIC_DRAW ) );

	GL( glEnableVertexAttribArray( 0 ) );
	GL( glEnableVertexAttribArray( 1 ) );
	GL( glEnableVertexAttribArray( 2 ) );

	GL( glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (const GLvoid*)offsetof( Vertex, pos ) ) );
	GL( glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (const GLvoid*)offsetof( Vertex, uv ) ) );
	GL( glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (const GLvoid*)offsetof( Vertex, col ) ) );

	GL( glBindVertexArray( 0 ) );

	GL( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

	triList->lastIndexBufferIndex = -1;
	triList->lastTriIndex = -1;

	return 0;
}

/*
Initializes all the stuff needed for rendering the triangles.
 Returns a value < 0 if there's a problem.
*/
int triRenderer_Init( int renderAreaWidth, int renderAreaHeight )
{
	for( int i = 0; i < NUM_SHADERS; ++i ) {
		shaderPrograms[i].programID = 0;
	}

	if( triRenderer_LoadShaders( ) < 0 ) {
		return -1;
	}

	solidTriangles.triCount = MAX_SOLID_TRIS;
	solidTriangles.vertCount = MAX_SOLID_TRIS * 3;

	transparentTriangles.triCount = MAX_TRANSPARENT_TRIS;
	transparentTriangles.vertCount = MAX_TRANSPARENT_TRIS * 3;

	stencilTriangles.triCount = MAX_STENCIL_TRIS;
	stencilTriangles.vertCount = MAX_STENCIL_TRIS * 3;

	llog( LOG_INFO, "Creating triangle lists." );
	if( ( createTriListGLObjects( &solidTriangles ) < 0 ) ||
		( createTriListGLObjects( &transparentTriangles ) < 0 ) ||
		( createTriListGLObjects( &stencilTriangles ) < 0 ) ) {
		return -1;
	}

	return 0;
}

static bool isTriAxisSeparating( Vector2* p0, Vector2* p1, Vector2* p2 )
{
	float temp;
	Vector2 projTriPt0, projTriPt2;
	float triPt0Dot, triPt2Dot;
	float triMin, triMax;
	float quadMin, quadMax;

	Vector2 orthoAxis;
	vec2_Subtract( p0, p1, &orthoAxis );
	temp = orthoAxis.x;
	orthoAxis.x = -orthoAxis.y;
	orthoAxis.y = temp;
	vec2_Normalize( &orthoAxis ); // is this necessary?

	// since we're generating it from p0 and p1 that means they should both project to the same
	//  value on the axis, so we only need to project p0 and p2
	vec2_ProjOnto( p0, &orthoAxis, &projTriPt0 );
	vec2_ProjOnto( p2, &orthoAxis, &projTriPt2 );
	triPt0Dot = vec2_DotProduct( &orthoAxis, &projTriPt0 );
	triPt2Dot = vec2_DotProduct( &orthoAxis, &projTriPt2 );

	if( triPt0Dot > triPt2Dot ) {
		triMin = triPt2Dot;
		triMax = triPt0Dot;
	} else {
		triMin = triPt0Dot;
		triMax = triPt2Dot;
	}

	// now find min and max for AABB on this axis, we know all points will be <+/-1,+/-1>
	float dotPP = orthoAxis.x + orthoAxis.y;
	float dotPN = orthoAxis.x - orthoAxis.y;
	float dotNN = -orthoAxis.x - orthoAxis.y;
	float dotNP = -orthoAxis.x + orthoAxis.y;

	quadMin = MIN( dotPP, MIN( dotPN, MIN( dotNN, dotNP ) ) );
	quadMax = MAX( dotPP, MAX( dotPN, MAX( dotNN, dotNP ) ) );

	return ( FLT_LT( quadMax, triMin ) || FLT_GT( quadMin, triMax ) );
}

// test to see if the triangle intersects the AABB centered on (0,0) with sides of length 2
static bool testTriangle( Vector2* p0, Vector2* p1, Vector2* p2 )
{
	// we'll need an optimized version of the SAT test, since we always know the position and size
	//  of the AABB we can precompute things

	// first test to see if there is a horizontal or vertical axis that separates
	if( FLT_LT( p0->x, -1.0f ) && FLT_LT( p1->x, -1.0f ) && FLT_LT( p2->x, -1.0f ) ) return false;
	if( FLT_GT( p0->x, 1.0f ) && FLT_GT( p1->x, 1.0f ) && FLT_GT( p2->x, 1.0f ) ) return false;

	if( FLT_LT( p0->y, -1.0f ) && FLT_LT( p1->y, -1.0f ) && FLT_LT( p2->y, -1.0f ) ) return false;
	if( FLT_GT( p0->y, 1.0f ) && FLT_GT( p1->y, 1.0f ) && FLT_GT( p2->y, 1.0f ) ) return false;

	// now test to see if the segments of the triangle generate a separating axis
	if( isTriAxisSeparating( p0, p1, p2 ) ) return false;
	if( isTriAxisSeparating( p1, p2, p0 ) ) return false;
	if( isTriAxisSeparating( p2, p0, p1 ) ) return false;

	return true;
}

static int addTriangle( TriangleList* triList, TriVert vert0, TriVert vert1, TriVert vert2,
	ShaderType shader, GLuint texture, float floatVal0, int clippingID, uint32_t camFlags, int8_t depth )
{
	//#define SCALE ( 3000.0f / 540.0f )/*
	//#define SCALE ( 3000.0f / 960.0f )//*/

	/*vert0.pos.x *= SCALE;
	vert0.pos.y *= SCALE;

	vert1.pos.x *= SCALE;
	vert1.pos.y *= SCALE;

	vert2.pos.x *= SCALE;
	vert2.pos.y *= SCALE;//*/

	// test to see if the triangle can be culled
	bool anyInside = false;
	for( int currCamera = cam_StartIteration( ); ( currCamera != -1 ) && !anyInside; currCamera = cam_GetNextActiveCam( ) ) {
		if( cam_GetFlags( currCamera ) & camFlags ) {
			Matrix4 vpMat;
			cam_GetVPMatrix( currCamera, &vpMat );

			// issue! if the triangle is too large, and all it's vertices are off screen while the triangle still intersects the screen it won't draw
			//  so we'll also have to test the sign of each access, if it's different between
			Vector2 p0, p1, p2;
			mat4_TransformVec2Pos( &vpMat, &( vert0.pos ), &p0 );
			mat4_TransformVec2Pos( &vpMat, &( vert1.pos ), &p1 );
			mat4_TransformVec2Pos( &vpMat, &( vert2.pos ), &p2 );
			anyInside = testTriangle( &p0, &p1, &p2 );
/*#define TEST_VERT( v )	mat4_TransformVec2Pos( &vpMat, &( v ), &test ); \
						anyInside = anyInside || ( FLT_GE( test.x, -1.0f ) && FLT_LE( test.x, 1.0f ) && FLT_GE( test.y, -1.0f ) && FLT_LE( test.y, 1.0f ) );
			TEST_VERT( vert0.pos );
			TEST_VERT( vert1.pos );
			TEST_VERT( vert2.pos );
#undef TEST_VERT//*/
		}
	}
	if( !anyInside ) return 0;

	if( triList->lastTriIndex >= ( triList->triCount - 1 ) ) {
		llog( LOG_VERBOSE, "Triangle list full." );
		return -1;
	}

	float z = (float)depth + ( Z_ORDER_OFFSET * ( solidTriangles.lastTriIndex + transparentTriangles.lastTriIndex + 2 ) );

	int idx = triList->lastTriIndex + 1;
	triList->lastTriIndex = idx;
	triList->triangles[idx].camFlags = camFlags;
	triList->triangles[idx].texture = texture;
	triList->triangles[idx].zPos = z;
	triList->triangles[idx].shaderType = shader;
	triList->triangles[idx].stencilGroup = clippingID;
	triList->triangles[idx].floatVal0 = floatVal0;
	int baseIdx = idx * 3;

#define ADD_VERT( v, offset ) \
	vec2ToVec3( &( (v).pos ), z, &( triList->vertices[baseIdx + (offset)].pos ) ); \
	triList->vertices[baseIdx + (offset)].col = (v).col; \
	triList->vertices[baseIdx + (offset)].uv = (v).uv; \
	triList->triangles[idx].vertexIndices[(offset)] = baseIdx + (offset);

	ADD_VERT( vert0, 0 );
	ADD_VERT( vert1, 1 );
	ADD_VERT( vert2, 2 );

#undef ADD_VERT

	return 0;
}

/*
We'll assume the array has three vertices in it.
 Return a value < 0 if there's a problem.
*/
int triRenderer_AddVertices( TriVert* verts, ShaderType shader, GLuint texture, float floatVal0,
	int clippingID, uint32_t camFlags, int8_t depth, TriType type )
{
	return triRenderer_Add( verts[0], verts[1], verts[2], shader, texture, floatVal0, clippingID, camFlags, depth, type );
}

int triRenderer_Add( TriVert vert0, TriVert vert1, TriVert vert2, ShaderType shader, GLuint texture, float floatVal0,
	int clippingID, uint32_t camFlags, int8_t depth, TriType type )
{
	switch( type ) {
	case TT_SOLID:
		return addTriangle( &solidTriangles, vert0, vert1, vert2, shader, texture, floatVal0, clippingID, camFlags, depth );
	case TT_TRANSPARENT:
		return addTriangle( &transparentTriangles, vert0, vert1, vert2, shader, texture, floatVal0, clippingID, camFlags, depth );
	case TT_STENCIL:
		return addTriangle( &stencilTriangles, vert0, vert1, vert2, shader, texture, floatVal0, clippingID, camFlags, depth );
	}
	return 0;
}

/*
Clears out all the triangles currently stored.
*/
void triRenderer_Clear( void )
{
	transparentTriangles.lastTriIndex = -1;
	solidTriangles.lastTriIndex = -1;
	stencilTriangles.lastTriIndex = -1;
}

static int sortByRenderState( const void* p1, const void* p2 )
{
	Triangle* tri1 = (Triangle*)p1;
	Triangle* tri2 = (Triangle*)p2;

	int stDiff = ( (int)tri1->shaderType - (int)tri2->shaderType );
	if( stDiff != 0 ) {
		return stDiff;
	}

	if( tri1->texture < tri2->texture ) {
		return -1;
	} else if( tri1->texture > tri2->texture ) {
		return 1;
	}

	return ( tri1->stencilGroup - tri2->stencilGroup );
}

static int sortByDepth( const void* p1, const void* p2 )
{
	Triangle* tri1 = (Triangle*)p1;
	Triangle* tri2 = (Triangle*)p2;
	return ( ( ( tri1->zPos ) - ( tri2->zPos ) ) > 0.0f ) ? 1 : -1;
}

static void generateVertexArray( TriangleList* triList )
{
	GL( glBindBuffer( GL_ARRAY_BUFFER, triList->VBO ) );
	GL( glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof( Vertex ) * ( ( triList->lastTriIndex + 1 ) * 3 ), triList->vertices ) );
}

// for when we're rendering to the stencil buffer
static void onStencilSwitch_Stencil( int stencilGroup )
{
	glStencilMask( 1 << stencilGroup );
}

// for when we should be reading from the stencil buffer
static void onStencilSwitch_Standard( int stencilGroup )
{
	if( ( stencilGroup >= 0 ) && ( stencilGroup <= 7 ) ) {
		// enable stencil masking
		glStencilFunc( GL_EQUAL, 0xff, ( 1 << stencilGroup ) );
	} else {
		// disable stencil masking
		glStencilFunc( GL_ALWAYS, 0xff, 0xff );
	}
}

static void drawTriangles( uint32_t currCamera, TriangleList* triList, void(*onStencilSwitch)( int ) )
{
	// create the index buffers to access the vertex buffer
	//  TODO: Test to see if having the index buffer or the vertex buffer in order is faster
	GLuint lastBoundTexture = 0;
	int triIdx = 0;
	ShaderType lastBoundShader = NUM_SHADERS;
	Matrix4 vpMat;
	uint32_t camFlags = 0;
	int lastSetClippingArea = -1;

	// we'll only be accessing the one vertex array
	GL( glBindVertexArray( triList->VAO ) );

	do {
		GLuint texture = triList->triangles[triIdx].texture;
		float floatVal0 = triList->triangles[triIdx].floatVal0;
		triList->lastIndexBufferIndex = -1;

		if( ( triIdx <= triList->lastTriIndex ) && ( triList->triangles[triIdx].shaderType != lastBoundShader ) ) {
			// next shader, bind and set up
			lastBoundShader = triList->triangles[triIdx].shaderType;

			camFlags = cam_GetFlags( currCamera );
			cam_GetVPMatrix( currCamera, &vpMat );

			GL( glUseProgram( shaderPrograms[lastBoundShader].programID ) );
			GL( glUniformMatrix4fv( shaderPrograms[lastBoundShader].uniformLocs[UNIFORM_TF_MAT], 1, GL_FALSE, &( vpMat.m[0] ) ) ); // set view projection matrix
			GL( glUniform1i( shaderPrograms[lastBoundShader].uniformLocs[UNIFORM_TEXTURE], 0 ) ); // use texture 0
		}

		if( ( triIdx <= triList->lastTriIndex ) && ( triList->triangles[triIdx].stencilGroup != lastSetClippingArea ) ) {
			// next clipping area
			lastSetClippingArea = triList->triangles[triIdx].stencilGroup;
			onStencilSwitch( triList->triangles[triIdx].stencilGroup );
		}

		int triCount = 0;
		// gather the list of all the triangles to be drawn
		while( ( triIdx <= triList->lastTriIndex ) &&
				( triList->triangles[triIdx].texture == texture ) &&
				( triList->triangles[triIdx].shaderType == lastBoundShader ) &&
				( triList->triangles[triIdx].stencilGroup == lastSetClippingArea ) &&
				FLT_EQ( triList->triangles[triIdx].floatVal0, floatVal0 ) ) {
			if( ( triList->triangles[triIdx].camFlags & camFlags ) != 0 ) {
				triList->indices[++triList->lastIndexBufferIndex] = triList->triangles[triIdx].vertexIndices[0];
				triList->indices[++triList->lastIndexBufferIndex] = triList->triangles[triIdx].vertexIndices[1];
				triList->indices[++triList->lastIndexBufferIndex] = triList->triangles[triIdx].vertexIndices[2];
			}
			++triIdx;
			++triCount;
		}

		// send the indices of the vertex array to draw, if there is any
		if( triList->lastIndexBufferIndex < 0 ) {
			continue;
		}
		GL( glUniform1f( shaderPrograms[lastBoundShader].uniformLocs[UNIFORM_FLOAT_0], floatVal0 ) );
		GL( glBindTexture( GL_TEXTURE_2D, texture ) );
		GL( glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, sizeof( GLuint ) * ( triList->lastIndexBufferIndex + 1 ), triList->indices ) );
		GL( glDrawElements( GL_TRIANGLES, triList->lastIndexBufferIndex + 1, GL_UNSIGNED_INT, NULL ) );
	} while( triIdx <= triList->lastTriIndex );
}

/*static void lerpVertices( TriangleList* triList, float t )
{
	for( int i = 0; i < triList->lastTriIndex; ++i ) {
		int baseIdx = i * 3;
		for( int s = 0; s < 3; ++i ) {
			triList->vertices[baseIdx+s].uv = triList->startVertices[baseIdx+s].uv;
			vec3_Lerp( &( triList->startVertices[baseIdx+s].pos ), &( triList->endVertices[baseIdx+s].pos ), t, &( triList->vertices[baseIdx+s].pos ) );
			clr_Lerp( &( triList->startVertices[baseIdx+s].col ), &( triList->endVertices[baseIdx+s].col ), t, &( triList->vertices[baseIdx+s].col ) );
		}
	}
}//*/

/*
Draws out all the triangles.
*/
void triRenderer_Render( )
{
	// SDL_qsort appears to break some times, so fall back onto the standard library qsort for right now, and implement our own when we have time
	qsort( solidTriangles.triangles, solidTriangles.lastTriIndex + 1, sizeof( Triangle ), sortByRenderState );
	qsort( transparentTriangles.triangles, transparentTriangles.lastTriIndex + 1, sizeof( Triangle ), sortByDepth );
	qsort( stencilTriangles.triangles, stencilTriangles.lastTriIndex + 1, sizeof( Triangle ), sortByRenderState );

	// now that the triangles have been sorted create the vertex arrays
	generateVertexArray( &solidTriangles );
	generateVertexArray( &transparentTriangles );
	generateVertexArray( &stencilTriangles );

	GL( glDisable( GL_CULL_FACE ) );
	GL( glEnable( GL_DEPTH_TEST ) );
	GL( glEnable( GL_STENCIL_TEST ) );
	GL( glClearStencil( 0x0 ) );
	
	// render triangles
	// TODO: We're ignoring any issues with cameras and transparency, probably want to handle this better.
	for( int currCamera = cam_StartIteration( ); currCamera != -1; currCamera = cam_GetNextActiveCam( ) ) {
		GL( glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT ) );

		GL( glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE ) );
		GL( glDepthFunc( GL_ALWAYS ) );
		GL( glDepthMask( GL_FALSE ) );
		GL( glStencilFunc( GL_ALWAYS, 0xff, 0xff ) );
		GL( glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE ) );
		drawTriangles( currCamera, &stencilTriangles, onStencilSwitch_Stencil );

		GL( glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ) );
		GL( glDepthFunc( GL_LESS ) );
		GL( glDepthMask( GL_TRUE ) );
		GL( glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP ) );
		GL( glStencilMask( 0xff ) );

		GL( glDisable( GL_BLEND ) );
		drawTriangles( currCamera, &solidTriangles, onStencilSwitch_Standard );

		GL( glEnable( GL_BLEND ) );
		GL( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
		drawTriangles( currCamera, &transparentTriangles, onStencilSwitch_Standard );
	}

	GL( glBindVertexArray( 0 ) );
	GL( glUseProgram( 0 ) );
}