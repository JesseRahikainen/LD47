#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL_main.h>
#include <SDL.h>
#include <assert.h>

#include <time.h>

#include <float.h>
#include <math.h>

#include "Math/vector2.h"
#include "Graphics/camera.h"
#include "Graphics/graphics.h"
#include "Math/mathUtil.h"
#include "sound.h"
#include "Utils/cfgFile.h"
#include "IMGUI/nuklearWrapper.h"
#include "world.h"

#include "UI/text.h"
#include "Input/input.h"

#include "System/gameTime.h"

#include "gameState.h"
#include "Game/gameScreen.h"

#include "System/memory.h"
#include "System/systems.h"
#include "System/platformLog.h"
#include "System/random.h"

#include "Graphics/debugRendering.h"
#include "Graphics/glPlatform.h"

#include "System/jobQueue.h"

#define DESIRED_WORLD_WIDTH 800
#define DESIRED_WORLD_HEIGHT 600

#define DESIRED_RENDER_WIDTH 800
#define DESIRED_RENDER_HEIGHT 600
#ifdef __EMSCRIPTEN__
	#define DESIRED_WINDOW_WIDTH DESIRED_RENDER_WIDTH
	#define DESIRED_WINDOW_HEIGHT DESIRED_RENDER_HEIGHT
#else
	#define DESIRED_WINDOW_WIDTH 800
	#define DESIRED_WINDOW_HEIGHT 600
#endif

#define DEFAULT_REFRESH_RATE 30

static bool running;
static bool focused;
static Uint64 lastTicks;
static Uint64 physicsTickAcc;
static SDL_Window* window;
static SDL_RWops* logFile;
static const char* windowName = "Toil of Pnamos";
int getWindowRefreshRate( SDL_Window* w )
{
	SDL_DisplayMode mode;
	if( SDL_GetWindowDisplayMode( w, &mode ) != 0 ) {
		return DEFAULT_REFRESH_RATE;
	}

	if( mode.refresh_rate == 0 ) {
		return DEFAULT_REFRESH_RATE;
	}

	llog( LOG_DEBUG, "w: %i  h: %i  r: %i", mode.w, mode.h, mode.refresh_rate );

	int ww, wh;
	SDL_GetWindowSize( w, &ww, &wh );
	llog( LOG_DEBUG, "ww: %i  wh: %i", ww, wh );

	return mode.refresh_rate;
}

void cleanUp( void )
{
	jq_ShutDown( );

	SDL_DestroyWindow( window );
	window = NULL;

	snd_CleanUp( );

	SDL_Quit( );

	if( logFile != NULL ) {
		SDL_RWclose( logFile );
	}

	mem_CleanUp( );

	atexit( NULL );
}

// TODO: Move to logPlatform
void logOutput( void* userData, int category, SDL_LogPriority priority, const char* message )
{
	size_t strLen = SDL_strlen( message );
	SDL_RWwrite( logFile, message, 1, strLen );
#if defined( WIN32 )
	SDL_RWwrite( logFile, "\r\n", 1, 2 );
#elif defined( __ANDROID__ )
	SDL_RWwrite( logFile, "\n", 1, 1 );
#elif defined( __EMSCRIPTEN__ )
	SDL_RWwrite( logFile, "\n", 1, 1 );
#else
	#warning "NO END OF LINE DEFINED FOR THIS PLATFORM!"
#endif
}

static void initIMGUI( NuklearWrapper* imgui, bool useRelativeMousePos, int width, int height )
{
	nk_xu_init( imgui, window, useRelativeMousePos, width, height );

	struct nk_font_atlas* fontAtlas;
	nk_xu_fontStashBegin( imgui, &fontAtlas );
	// load fonts
	struct nk_font *font = nk_font_atlas_add_from_file( fontAtlas, "./Fonts/kenpixel.ttf", 12, 0 );
	nk_xu_fontStashEnd( imgui );
	nk_style_set_font( &( imgui->ctx ), &( font->handle ) );
}

int unalignedAccess( void )
{
	llog( LOG_INFO, "Forcing unaligned access" );
#pragma warning( push )
#pragma warning( disable : 6011) // this is for testing behavior of a platform, never used in the rest of the program so it should cause no issues
	int* is = (int*)malloc( sizeof( int ) * 100 );

	for( int i = 0; i < 100; ++i ) {
		is[i] = i;
	}

	char* p = (char*)is;
	int* intData = (int*)((char*)&( p[1] ) );
	int unalignedInt = *intData;
	llog( LOG_INFO, "data: %i", *intData );

	intData = (int*)( (char*)&( p[2] ) );
	llog( LOG_INFO, "data: %i", *intData );

	intData = (int*)( (char*)&( p[3] ) );
	llog( LOG_INFO, "data: %i", *intData );

	return unalignedInt;
#pragma warning( pop )
}

int initEverything( void )
{
#ifndef _DEBUG
	logFile = SDL_RWFromFile( "log.txt", "w" );
	if( logFile != NULL ) {
		SDL_LogSetOutputFunction( logOutput, NULL );
	}
#endif

	//unalignedAccess( );

	llog( LOG_INFO, "Initializing memory." );
	// memory first, won't be used everywhere at first so lets keep the initial allocation low, 64 MB
	mem_Init( 64 * 1024 * 1024 );

	// then SDL
	SDL_SetMainReady( );
	if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 ) {
		llog( LOG_ERROR, "%s", SDL_GetError( ) );
		return -1;
	}
	llog( LOG_INFO, "SDL successfully initialized." );
	atexit( cleanUp );

	// set up opengl
	//  try opening and parsing the config file
	int majorVersion;
	int minorVersion;
	int redSize;
	int greenSize;
	int blueSize;
	int depthSize;
	int stencilSize;

	void* oglCFGFile;
#if defined( __EMSCRIPTEN__ )
	// NOTE: If you run into an errors with opening this file check to see if you're using FireFox, if
	//  you are then switch to a different browser for testing.
	oglCFGFile = cfg_OpenFile( "webgl.cfg" );
#elif defined( __ANDROID__ )
	oglCFGFile = cfg_OpenFile( "androidgl.cfg" );
#else
	oglCFGFile = cfg_OpenFile( "opengl.cfg" );
#endif

	cfg_GetInt( oglCFGFile, "MAJOR", 3, &majorVersion );
	cfg_GetInt( oglCFGFile, "MINOR", 3, &minorVersion );
	cfg_GetInt( oglCFGFile, "RED_SIZE", 8, &redSize );
	cfg_GetInt( oglCFGFile, "GREEN_SIZE", 8, &greenSize );
	cfg_GetInt( oglCFGFile, "BLUE_SIZE", 8, &blueSize );
	cfg_GetInt( oglCFGFile, "DEPTH_SIZE", 16, &depthSize );
	cfg_GetInt( oglCFGFile, "STENCIL_SIZE", 8, &stencilSize );
	cfg_CloseFile( oglCFGFile );

	// todo: commenting these out breaks the font rendering, something wrong with the texture that's created
	//  note: without these it uses the values loaded in from the .cfg file, which is 3.3
//#if defined( __ANDROID__ ) //|| defined( __IOS__ )
	// setup using OpenGLES
//#else
	// setup using OpenGL
	//majorVersion = 2;
	//minorVersion = 0;
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, PROFILE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, minorVersion );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, redSize );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, greenSize );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, blueSize );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, depthSize );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, stencilSize );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
//#endif

	llog( LOG_INFO, "Desired GL version: %i.%i", majorVersion, minorVersion );

#if defined( __ANDROID__ )
	Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode( 0, &mode );
	int windowWidth = mode.w;
	int windowHeight = mode.h;
#else
	int windowHeight = DESIRED_WINDOW_HEIGHT;
	int windowWidth = DESIRED_WINDOW_WIDTH;

	Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif

	//int renderHeight = DESIRED_RENDER_HEIGHT;
	//int renderWidth = (int)( renderHeight * (float)windowWidth / (float)windowHeight );
	int renderHeight = windowHeight;
	int renderWidth = windowWidth;

	int worldHeight = DESIRED_WORLD_HEIGHT;
	int worldWidth = (int)( worldHeight * (float)windowWidth / (float)windowHeight );

	world_SetSize( worldWidth, worldHeight );

	llog( LOG_INFO, "Window size: %i x %i    Render size: %i x %i", windowWidth, windowHeight, renderWidth, renderHeight );

	window = SDL_CreateWindow( windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		windowWidth, windowHeight, windowFlags );

	if( window == NULL ) {
		llog( LOG_ERROR, "%s", SDL_GetError( ) );
		return -1;
	}
	llog( LOG_INFO, "SDL OpenGL window successfully created" );

	// Create rendering
	if( gfx_Init( window, renderWidth, renderHeight ) < 0 ) {
		return -1;
	}
	llog( LOG_INFO, "Rendering successfully initialized" );

	// Create sound mixer
	if( snd_Init( 2 ) < 0 ) {
		return -1;
	}
	llog( LOG_INFO, "Mixer successfully initialized" );

	cam_Init( );
	cam_SetProjectionMatrices( worldWidth, worldHeight, false );
	llog( LOG_INFO, "Cameras successfully initialized" );

	input_InitMouseInputArea( worldWidth, worldHeight );

	// on mobile devices we can't assume the width and height we've used to create the window
	//  are the current width and height
	int winWidth;
	int winHeight;
	SDL_GetWindowSize( window, &winWidth, &winHeight );
	input_UpdateMouseWindow( winWidth, winHeight );
	llog( LOG_INFO, "Input successfully initialized" );

	initIMGUI( &inGameIMGUI, true, renderWidth, renderHeight );
	initIMGUI( &editorIMGUI, false, winWidth, winHeight );
	llog( LOG_INFO, "IMGUI successfully initialized" );

	txt_Init( );

	rand_Seed( NULL, (uint32_t)time( NULL ) );

	return 0;
}

/* input processing */
void processEvents( int windowsEventsOnly )
{
	SDL_Event e;
	nk_input_begin( &( editorIMGUI.ctx ) );
	nk_input_begin( &( inGameIMGUI.ctx ) );
	while( SDL_PollEvent( &e ) != 0 ) {
		if( e.type == SDL_WINDOWEVENT ) {
			switch( e.window.event ) {
			case SDL_WINDOWEVENT_RESIZED:
				// data1 == width, data2 == height
				gfx_SetWindowSize( e.window.data1, e.window.data2 );
				input_UpdateMouseWindow( e.window.data1, e.window.data2 );
				break;

			// will want to handle these messages for pausing and unpausing the game when they lose focus
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				llog( LOG_DEBUG, "Gained focus" );
				focused = true;
				snd_SetFocus( true );
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				llog( LOG_DEBUG, "Lost focus" );
				focused = false;
				snd_SetFocus( false );
				break;
			}
		}

		if( e.type == SDL_QUIT ) {
			llog( LOG_DEBUG, "Quitting" );
			running = false;
		}

		if( windowsEventsOnly ) { 
			continue;
		}

		sys_ProcessEvents( &e );
		input_ProcessEvents( &e );
		gsm_ProcessEvents( &globalFSM, &e );
		//imgui_ProcessEvents( &e ); // just for TextInput events when text input is enabled
		nk_xu_handleEvent( &editorIMGUI, &e );
		nk_xu_handleEvent( &inGameIMGUI, &e );
	}

	nk_input_end( &( editorIMGUI.ctx ) );
	nk_input_end( &( inGameIMGUI.ctx ) );
}

// needed to be able to work in javascript
void mainLoop( void* v )
{
	Uint64 tickDelta;
	Uint64 currTicks;
	int numPhysicsProcesses;
	float renderDelta;

	Uint64 mainTimer = gt_StartTimer( );

#if defined( __EMSCRIPTEN__ )
	if( !running ) {
		emscripten_cancel_main_loop( );
	}
#endif

	currTicks = SDL_GetPerformanceCounter( );
	tickDelta = currTicks - lastTicks;
	lastTicks = currTicks;

	Uint64 procTimer = gt_StartTimer( );
	if( !focused ) {
		processEvents( 1 );
		return;
	}

	physicsTickAcc += tickDelta;

	// process input
	processEvents( 0 );

	// handle per frame update
	sys_Process( );
	gsm_Process( &globalFSM );
	float procTimerSec = gt_StopTimer( procTimer );

	Uint64 physicsTimer = gt_StartTimer( );
	// process movement, collision, and other things that require a delta time
	numPhysicsProcesses = 0;
	while( physicsTickAcc > PHYSICS_TICK ) {
		sys_PhysicsTick( PHYSICS_DT );
		gsm_PhysicsTick( &globalFSM, PHYSICS_DT );
		physicsTickAcc -= PHYSICS_TICK;
		++numPhysicsProcesses;
	}
	float physicsTimerSec = gt_StopTimer( physicsTimer );

	Uint64 drawTimer = gt_StartTimer( );
	// drawing
	if( numPhysicsProcesses > 0 ) {
		// set the new draw positions
		renderDelta = PHYSICS_DT * (float)numPhysicsProcesses;
		gfx_ClearDrawCommands( renderDelta );
		cam_FinalizeStates( renderDelta );

		// set up drawing for everything
		sys_Draw( );
		gsm_Draw( &globalFSM );
	}
	float drawTimerSec = gt_StopTimer( drawTimer );

	Uint64 mainJobsTimer = gt_StartTimer( );
	// process all the jobs we need the main thread for, using this reduces the need for synchronization
	jq_ProcessMainThreadJobs( );
	float mainJobsTimerSec = gt_StopTimer( mainJobsTimer );

	Uint64 renderTimer = gt_StartTimer( );
	// do the actual rendering for this frame
	float dt = (float)tickDelta / (float)SDL_GetPerformanceFrequency( ); //(float)tickDelta / 1000.0f;
	gt_SetRenderTimeDelta( dt );
	cam_Update( dt );
	gfx_Render( dt );
	float renderTimerSec = gt_StopTimer( renderTimer );

	Uint64 flipTimer = gt_StartTimer( );
	SDL_GL_SwapWindow( window );
	float flipTimerSec = gt_StopTimer( flipTimer );

/*	if( dt >= 0.02f ) {
		llog( LOG_INFO, "!!! dt: %f", dt );
	} else {
		llog( LOG_INFO, "dt: %f", dt );
	}//*/

	float mainTimerSec = gt_StopTimer( mainTimer );

	int priority = ( mainTimerSec >= 0.15f ) ? LOG_WARN : LOG_INFO;
	//llog( priority, "%smain: %.4f - proc: %.4f  phys: %.4f  draw: %.4f  jobs: %.4f  rndr: %.4f  flip: %.4f", ( mainTimerSec >= 0.02f ) ? "!!! " : "", mainTimerSec, procTimerSec, physicsTimerSec, drawTimerSec, mainJobsTimerSec, renderTimerSec, flipTimerSec );
	//llog( priority, "%smain: %.4f", ( mainTimerSec >= 0.02f ) ? "!!! " : "", mainTimerSec );
}

#include "Utils/hashMap.h"
int main( int argc, char** argv )
{
/*#ifdef _DEBUG
	SDL_LogSetAllPriority( SDL_LOG_PRIORITY_VERBOSE );
#else
	SDL_LogSetAllPriority( SDL_LOG_PRIORITY_WARN );
#endif//*/

	SDL_LogSetAllPriority( SDL_LOG_PRIORITY_VERBOSE );

	if( initEverything( ) < 0 ) {
		return 1;
	}

	srand( (unsigned int)time( NULL ) );

	//***** main loop *****
	running = true;
	lastTicks = SDL_GetPerformanceCounter( );
	physicsTickAcc = 0;
#if defined( __ANDROID__ )
	focused = true;
#endif

	gsm_EnterState( &globalFSM, &gameScreenState );
	//gsm_EnterState( &globalFSM, &testAStarScreenState );
	//gsm_EnterState( &globalFSM, &testJobQueueScreenState );
	//gsm_EnterState( &globalFSM, &testSoundsScreenState );
	//gsm_EnterState( &globalFSM, &testPointerResponseScreenState );
	//gsm_EnterState( &globalFSM, &testSteeringScreenState );
	//gsm_EnterState( &globalFSM, &bordersTestScreenState );
	//gsm_EnterState( &globalFSM, &hexTestScreenState );

#if defined( __EMSCRIPTEN__ )
	emscripten_set_main_loop_arg( mainLoop, NULL, -1, 1 );
#else
	while( running ) {
		mainLoop( NULL );
	}
#endif

	return 0;
}