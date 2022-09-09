/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @mainpage Naev
 *
 * Doxygen documentation for the Naev project.
 */
/**
 * @file naev.c
 *
 * @brief Controls the overall game flow: data loading/unloading and game loop.
 */

/** @cond */
#include "linebreak.h"
#include "physfsrwops.h"
#include "SDL.h"
#include "SDL_error.h"
#include "SDL_image.h"

#include "naev.h"

#if HAS_POSIX
#include <time.h>
#include <unistd.h>
#endif /* HAS_POSIX */
/** @endcond */

#include "ai.h"
#include "background.h"
#include "camera.h"
#include "cond.h"
#include "conf.h"
#include "console.h"
#include "damagetype.h"
#include "debug.h"
#include "dialogue.h"
#include "economy.h"
#include "env.h"
#include "event.h"
#include "faction.h"
#include "fleet.h"
#include "font.h"
#include "gui.h"
#include "hook.h"
#include "input.h"
#include "joystick.h"
#include "land.h"
#include "load.h"
#include "log.h"
#include "map.h"
#include "map_overlay.h"
#include "map_system.h"
#include "menu.h"
#include "mission.h"
#include "music.h"
#include "ndata.h"
#include "nebula.h"
#include "news.h"
#include "nfile.h"
#include "nlua_misn.h"
#include "nlua_var.h"
#include "npc.h"
#include "nstring.h"
#include "nxml.h"
#include "opengl.h"
#include "options.h"
#include "outfit.h"
#include "pause.h"
#include "physics.h"
#include "pilot.h"
#include "player.h"
#include "render.h"
#include "rng.h"
#include "semver.h"
#include "ship.h"
#include "slots.h"
#include "sound.h"
#include "space.h"
#include "spfx.h"
#include "start.h"
#include "tech.h"
#include "threadpool.h"
#include "toolkit.h"
#include "unidiff.h"
#include "weapon.h"

#define CONF_FILE       "conf.lua" /**< Configuration file by default. */
#define VERSION_FILE    "VERSION" /**< Version file by default. */

static int quit               = 0; /**< For primary loop */
static unsigned int time_ms   = 0; /**< used to calculate FPS and movement. */
static double loading_r       = 0.; /**< Just to provide some randomness. */
static glTexture *loading     = NULL; /**< Loading screen. */
static glFont loading_font; /**< Loading font. */
static char *loading_txt = NULL; /**< Loading text to display. */
static SDL_Surface *naev_icon = NULL; /**< Icon. */
static int fps_skipped        = 0; /**< Skipped last frame? */
/* Version stuff. */
static semver_t version_binary; /**< Naev binary version. */
//static semver_t version_data; /**< Naev data version. */
static char version_human[STRMAX_SHORT]; /**< Human readable version. */

/*
 * FPS stuff.
 */
static double fps_dt    = 1.; /**< Display fps accumulator. */
static double game_dt   = 0.; /**< Current game deltatick (uses dt_mod). */
static double real_dt   = 0.; /**< Real deltatick. */
static double fps     = 0.; /**< FPS to finally display. */
static double fps_cur = 0.; /**< FPS accumulator to trigger change. */
static double fps_x     =  15.; /**< FPS X position. */
static double fps_y     = -15.; /**< FPS Y position. */
const double fps_min    = 1./30.; /**< Minimum fps to run at. */

/*
 * prototypes
 */
/* Loading. */
static void print_SDLversion (void);
static void loadscreen_load (void);
static void loadscreen_unload (void);
static void load_all (void);
static void unload_all (void);
static void window_caption (void);
/* update */
static void fps_init (void);
static double fps_elapsed (void);
static void fps_control (void);
static void update_all (void);
/* Misc. */
static void loadscreen_render( double done, const char *msg );
void main_loop( int update ); /* dialogue.c */


/**
 * @brief Flags naev to quit.
 */
void naev_quit (void)
{
   quit = 1;
}


/**
 * @brief Get if Naev is trying to quit.
 */
int naev_isQuit (void)
{
   return quit;
}


/**
 * @brief The entry point of Naev.
 *
 *    @param[in] argc Number of arguments.
 *    @param[in] argv Array of argc arguments.
 *    @return EXIT_SUCCESS on success.
 */
int main( int argc, char** argv )
{
   char conf_file_path[PATH_MAX], **search_path, **p;

#ifdef DEBUGGING
   /* Set Debugging flags. */
   memset( debug_flags , 0, DEBUG_FLAGS_MAX );
#endif /* DEBUGGING */

   env_detect( argc, argv );

   log_init();

   /* Set up PhysicsFS. */
   if( PHYSFS_init( env.argv0 ) == 0 ) {
      ERR( "PhysicsFS initialization failed: %s",
            PHYSFS_getErrorByCode( PHYSFS_getLastErrorCode() ) );
      return -1;
   }

   /* Set up locales. */
   gettext_init();
   init_linebreak();

   /* Parse version. */
   if (semver_parse( VERSION, &version_binary ))
      WARN( _("Failed to parse version string '%s'!"), VERSION );

   /* Print the version */
   LOG( " %s v%s (%s)", APPNAME, naev_version(0), HOST );

   if ( env.isAppImage )
      LOG( "AppImage detected. Running from: %s", env.appdir );
   else
      DEBUG( "AppImage not detected." );

   /* Initializes SDL for possible warnings. */
   if ( SDL_Init( 0 ) ) {
      ERR( _( "Unable to initialize SDL: %s" ), SDL_GetError() );
      return -1;
   }

   /* Initialize the threadpool */
   threadpool_init();

   /* Set up debug signal handlers. */
   debug_sigInit();

#if HAS_UNIX
   /* Set window class and name. */
   nsetenv("SDL_VIDEO_X11_WMCLASS", APPNAME, 0);
#endif /* HAS_UNIX */

   /* Must be initialized before input_init is called. */
   if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
      WARN( _("Unable to initialize SDL Video: %s"), SDL_GetError());
      return -1;
   }

   /* We'll be parsing XML. */
   LIBXML_TEST_VERSION
   xmlInitParser();

   /* Input must be initialized for config to work. */
   input_init();

   lua_init(); /* initializes lua */

   conf_setDefaults(); /* set the default config values */

   /*
    * Attempts to load the data path from datapath.lua
    * At this early point in the load process, the binary path
    * is the only place likely to be checked.
    */
   conf_loadConfigPath();

   /* Create the home directory if needed. */
   if ( nfile_dirMakeExist( nfile_configPath() ) )
      WARN( _("Unable to create config directory '%s'"), nfile_configPath());

   /* Set the configuration. */
   snprintf(conf_file_path, sizeof(conf_file_path), "%s"CONF_FILE, nfile_configPath());

   conf_loadConfig(conf_file_path); /* Lua to parse the configuration file */
   conf_parseCLI( argc, argv ); /* parse CLI arguments */

   /* Set up I/O. */
   ndata_setupWriteDir();
   log_redirect();
   ndata_setupReadDirs();
   gettext_setLanguage( conf.language ); /* now that we can find translations */
   LOG( _("Loaded configuration: %s"), conf_file_path );
   search_path = PHYSFS_getSearchPath();
   LOG( "%s", _("Read locations, searched in order:") );
   for (p = search_path; *p != NULL; p++)
      LOG( "    %s", *p );
   PHYSFS_freeList( search_path );
   /* Logging the cache path is noisy, noisy is good at the DEBUG level. */
   DEBUG( _("Cache location: %s"), nfile_cachePath() );
   LOG( _("Write location: %s\n"), PHYSFS_getWriteDir() );

   /* Enable FPU exceptions. */
   if (conf.fpu_except)
      debug_enableFPUExcept();

   /* Load the start info. */
   if (start_load())
      ERR( _("Failed to load module start data.") );
   LOG(" %s", start_name());
   DEBUG_BLANK();

   /* Display the SDL Version. */
   print_SDLversion();
   DEBUG_BLANK();

   /* random numbers */
   rng_init();

   /*
    * OpenGL
    */
   if (gl_init()) { /* initializes video output */
      ERR( _("Initializing video output failed, exiting...") );
      SDL_Quit();
      exit(EXIT_FAILURE);
   }
   window_caption();

   /* Have to set up fonts before rendering anything. */
   //DEBUG("Using '%s' as main font and '%s' as monospace font.", _(FONT_DEFAULT_PATH), _(FONT_MONOSPACE_PATH));
   gl_fontInit( &gl_defFont, _(FONT_DEFAULT_PATH), conf.font_size_def, FONT_PATH_PREFIX, 0 ); /* initializes default font to size */
   gl_fontInit( &gl_smallFont, _(FONT_DEFAULT_PATH), conf.font_size_small, FONT_PATH_PREFIX, 0 ); /* small font */
   gl_fontInit( &gl_defFontMono, _(FONT_MONOSPACE_PATH), conf.font_size_def, FONT_PATH_PREFIX, 0 );

   /* Detect size changes that occurred after window creation. */
   naev_resize();

   /* Display the load screen. */
   loadscreen_load();
   loadscreen_render( 0., _("Initializing subsystems...") );
   time_ms = SDL_GetTicks();

   /*
    * Input
    */
   if ((conf.joystick_ind >= 0) || (conf.joystick_nam != NULL)) {
      if (joystick_init())
         WARN( _("Error initializing joystick input") );
      if (conf.joystick_nam != NULL) { /* use the joystick name to find a joystick */
         if (joystick_use(joystick_get(conf.joystick_nam))) {
            WARN( _("Failure to open any joystick, falling back to default keybinds") );
            input_setDefault(LAYOUT_WASD);
         }
         free(conf.joystick_nam);
      }
      else if (conf.joystick_ind >= 0) /* use a joystick id instead */
         if (joystick_use(conf.joystick_ind)) {
            WARN( _("Failure to open any joystick, falling back to default keybinds") );
            input_setDefault(LAYOUT_WASD);
         }
   }

   /*
    * OpenAL - Sound
    */
   if (conf.nosound) {
      LOG( _("Sound is disabled!") );
      sound_disabled = 1;
      music_disabled = 1;
   }
   if (sound_init())
      WARN( _("Problem setting up sound!") );
   music_choose("load");

   /* FPS stuff. */
   fps_setPos( 15., (double)(gl_screen.h-15-gl_defFont.h) );

   /* Misc graphics init */
   render_init();
   if (nebu_init() != 0) { /* Initializes the nebula */
      /* An error has happened */
      ERR( _("Unable to initialize the Nebula subsystem!") );
      /* Weirdness will occur... */
   }
   gui_init(); /* initializes the GUI graphics */
   toolkit_init(); /* initializes the toolkit */
   map_init(); /* initializes the map. */
   map_system_init(); /* Initialise the solar system map */
   cond_init(); /* Initialize conditional subsystem. */
   cli_init(); /* Initialize console. */

   /* Data loading */
   load_all();

   /* Detect size changes that occurred during load. */
   naev_resize();

   /* Unload load screen. */
   loadscreen_unload();

   /* Start menu. */
   menu_main();

   LOG( _( "Reached main menu" ) );

   fps_init(); /* initializes the time_ms */

   /*
    * main loop
    */
   SDL_Event event;
   /* flushes the event loop since I noticed that when the joystick is loaded it
    * creates button events that results in the player starting out acceling */
   while (SDL_PollEvent(&event));

   /* primary loop */
   while (!quit) {
      while (!quit && SDL_PollEvent(&event)) { /* event loop */
         if (event.type == SDL_QUIT) {
            if (quit || menu_askQuit()) {
               quit = 1; /* quit is handled here */
               break;
            }
         }
         else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_RESIZED) {
            naev_resize();
            continue;
         }
         input_handle(&event); /* handles all the events and player keybinds */
      }

      main_loop( 1 );
   }

   /* Save configuration. */
   conf_saveConfig(conf_file_path);

   /* data unloading */
   unload_all();

   /* cleanup opengl fonts */
   gl_freeFont(NULL);
   gl_freeFont(&gl_smallFont);
   gl_freeFont(&gl_defFontMono);

   start_cleanup(); /* Cleanup from start.c, not the first cleanup step. :) */

   /* exit subsystems */
   cli_exit(); /* Clean up the console. */
   map_system_exit(); /* Destroys the solar system map. */
   map_exit(); /* Destroys the map. */
   ovr_mrkFree(); /* Clear markers. */
   toolkit_exit(); /* Kills the toolkit */
   ai_exit(); /* Stops the Lua AI magic */
   joystick_exit(); /* Releases joystick */
   input_exit(); /* Cleans up keybindings */
   nebu_exit(); /* Destroys the nebula */
   lua_exit(); /* Closes Lua state. */
   render_exit(); /* Cleans up post-processing. */
   gl_exit(); /* Kills video output */
   sound_exit(); /* Kills the sound */
   news_exit(); /* Destroys the news. */

   /* Has to be run last or it will mess up sound settings. */
   conf_cleanup(); /* Free some memory the configuration allocated. */

   /* Free the icon. */
   if (naev_icon)
      SDL_FreeSurface(naev_icon);

   IMG_Quit(); /* quits SDL_image */
   SDL_Quit(); /* quits SDL */

   /* Clean up parser. */
   xmlCleanupParser();

   /* Clean up signal handler. */
   debug_sigClose();

   /* Delete logs if empty. */
   log_clean();

   PHYSFS_deinit();

   /* all is well */
   debug_enableLeakSanitizer();
   return 0;
}


/**
 * @brief Loads a loading screen.
 */
void loadscreen_load (void)
{
   char file_path[PATH_MAX];
   char **loadpaths, **loadscreens, *load;
   size_t nload, nreal, nbuf;
   const char *loading_prefix = "webp";

   /* Count the loading screens */
   loadpaths = PHYSFS_enumerateFiles( GFX_PATH"loading/" );

   for (nload=0; loadpaths[nload]!=NULL; nload++) {}
   loadscreens = calloc( nload, sizeof(char*) );
   nreal = 0;
   for (nload=0; loadpaths[nload]!=NULL; nload++) {
      if (!ndata_matchExt( loadpaths[nload], loading_prefix ))
         continue;
      loadscreens[nreal++] = loadpaths[nload];
   }

   /* Must have loading screens */
   if (nreal==0) {
      WARN( _("No loading screens found!") );
      PHYSFS_freeList( loadpaths );
      free( loadscreens );
      loading = NULL;
      return;
   }

   /* Load the loading font. */
   gl_fontInit( &loading_font, _(FONT_DEFAULT_PATH), 24, FONT_PATH_PREFIX, 0 ); /* initializes default font to size */

   /* Set the zoom. */
   cam_setZoom( conf.zoom_far );

   /* Choose the screen. */
   load = loadscreens[ RNG_BASE(0,nreal-1) ];

   /* Load the texture */
   snprintf( file_path, sizeof(file_path), GFX_PATH"loading/%s", load );
   loading = gl_newImage( file_path, 0 );
   loading_r = RNGF();

   /* Load the metadata. */
   snprintf( file_path, sizeof(file_path), GFX_PATH"loading/%s.txt", load );
   free( loading_txt );
   loading_txt = ndata_read( file_path, &nbuf );

   /* Create the stars. */
   background_initStars(1000);
   background_initDust();

   /* Clean up. */
   PHYSFS_freeList( loadpaths );
   free( loadscreens );
}


/**
 * @brief Renders the load screen with message.
 *
 *    @param done Amount done (1. == completed).
 *    @param msg Loading screen message.
 */
void loadscreen_render( double done, const char *msg )
{
   const double SHIP_IMAGE_WIDTH    = 512.;  /**< Loadscreen Ship Image Width */
   const double SHIP_IMAGE_HEIGHT   = 512.; /**< Loadscreen Ship Image Height */
   double bx;  /**<  Blit Image X Coord */
   double by;  /**<  Blit Image Y Coord */
   double x;   /**<  Progress Bar X Coord */
   double y;   /**<  Progress Bar Y Coord */
   double w;   /**<  Progress Bar Width Basis */
   double h;   /**<  Progress Bar Height Basis */
   double rh;  /**<  Loading Progress Text Relative Height */
   SDL_Event event;

   /* Clear background. */
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* Draw stars. */
   background_renderStars(0., 1);

   /*
    * Dimensions.
    */
   /* Image. */
   bx = (SCREEN_W-SHIP_IMAGE_WIDTH)/2.;
   by = (SCREEN_H-SHIP_IMAGE_HEIGHT)/2.;
   /* Loading bar. */
   w  = SCREEN_W * 0.4;
   h  = SCREEN_H * 0.025;
   rh = h + gl_defFont.h + 4.;
   x  = (SCREEN_W-w)/2.;
   y  = (SCREEN_H-SHIP_IMAGE_HEIGHT)/2. - rh - 5.;

   /* Draw loading screen image. */
   if (loading != NULL)
      gl_blitScale( loading, bx, by, SHIP_IMAGE_WIDTH, SHIP_IMAGE_HEIGHT, NULL );
   if (loading_txt != NULL) {
      int tw = gl_printWidthRaw( &loading_font, loading_txt );
      gl_printRaw( &loading_font, bx+SHIP_IMAGE_WIDTH-tw, by+20, &cFontWhite, 1, loading_txt );
   }

   /* Draw progress bar. */
   glUseProgram(shaders.progressbar.program);
   glUniform1f( shaders.progressbar.paramf, loading_r );
   glUniform1f( shaders.progressbar.dt, done );
   gl_renderShader( x, y, w, h, 0., &shaders.progressbar, NULL, 0 );

   /* Draw text. */
   gl_printRaw( &gl_defFont, x, y + h + 3., &cFontWhite, -1., msg );

   /* Get rid of events again. */
   while (SDL_PollEvent(&event));

   /* Flip buffers. HACK: Also try to catch a late-breaking resize from the WM. */
   SDL_GL_SwapWindow( gl_screen.window );
   naev_resize();
}


/**
 * @brief Frees the loading screen.
 */
static void loadscreen_unload (void)
{
   gl_freeTexture(loading);
   loading = NULL;
   gl_freeFont( &loading_font );
   free( loading_txt );
}


/**
 * @brief Loads all the data, makes main() simpler.
 */
#define LOADING_STAGES     16. /**< Amount of loading stages. */
void load_all (void)
{
   /* We can do fast stuff here. */
   sp_load();

   /* order is very important as they're interdependent */
   loadscreen_render( 1./LOADING_STAGES, _("Loading Commodities...") );
   commodity_load(); /* dep for space */

   loadscreen_render( 2./LOADING_STAGES, _("Loading Special Effects...") );
   spfx_load(); /* no dep */

   loadscreen_render( 3./LOADING_STAGES, _("Loading Damage Types...") );
   dtype_load(); /* dep for outfits */

   loadscreen_render( 4./LOADING_STAGES, _("Loading Outfits...") );
   outfit_load(); /* dep for ships, factions */

   loadscreen_render( 5./LOADING_STAGES, _("Loading Ships...") );
   ships_load(); /* dep for fleet */

   loadscreen_render( 6./LOADING_STAGES, _("Loading Factions...") );
   factions_load(); /* dep for fleet, space, missions, AI */

   loadscreen_render( 7./LOADING_STAGES, _("Loading Events...") );
   events_load(); /* no dep */

   loadscreen_render( 8./LOADING_STAGES, _("Loading Missions...") );
   missions_load(); /* no dep */

   loadscreen_render( 9./LOADING_STAGES, _("Loading AI...") );
   ai_load(); /* dep for fleets */

   loadscreen_render( 10./LOADING_STAGES, _("Loading Fleets...") );
   fleet_load(); /* dep for space */

   loadscreen_render( 11./LOADING_STAGES, _("Loading Techs...") );
   tech_load(); /* dep for space */

   loadscreen_render( 12./LOADING_STAGES, _("Loading the Universe...") );
   space_load();

   loadscreen_render( 13./LOADING_STAGES, _("Loading the UniDiffs...") );
   diff_loadAvailable();

   loadscreen_render( 14./LOADING_STAGES, _("Populating Maps...") );
   outfit_mapParse();

   loadscreen_render( 15./LOADING_STAGES, _("Initializing Details..") );
   background_init();
   map_load();
   map_system_load();
   pilots_init();
   weapon_init();
   player_init(); /* Initialize player stuff. */
   loadscreen_render( 1., _("Loading Completed!") );
}
/**
 * @brief Unloads all data, simplifies main().
 */
void unload_all (void)
{
   /* cleanup some stuff */
   player_cleanup(); /* cleans up the player stuff */
   gui_free(); /* cleans up the player's GUI */
   weapon_exit(); /* destroys all active weapons */
   pilots_free(); /* frees the pilots, they were locked up :( */
   cond_exit(); /* destroy conditional subsystem. */
   land_exit(); /* Destroys landing vbo and friends. */
   npc_clear(); /* In case exiting while landed. */
   background_free(); /* Destroy backgrounds. */
   load_free(); /* Clean up loading game stuff stuff. */
   diff_free();
   economy_destroy(); /* must be called before space_exit */
   space_exit(); /* cleans up the universe itself */
   tech_free(); /* Frees tech stuff. */
   fleet_free();
   ships_free();
   outfit_free();
   spfx_free(); /* gets rid of the special effect */
   dtype_free(); /* gets rid of the damage types */
   missions_free();
   events_exit(); /* Clean up events. */
   factions_free();
   commodity_free();
   var_cleanup(); /* cleans up mission variables */
   sp_cleanup();
}


/**
 * @brief Split main loop from main() for secondary loop hack in toolkit.c.
 */
void main_loop( int update )
{
   /*
    * Control FPS.
    */
   fps_control(); /* everyone loves fps control */

   /*
    * Handle update.
    */
   input_update( real_dt ); /* handle key repeats. */
   sound_update( real_dt ); /* Update sounds. */
   if (toolkit_isOpen())
      toolkit_update(); /* to simulate key repetition */
   if (!paused && update) {
      /* Important that we pass real_dt here otherwise we get a dt feedback loop which isn't pretty. */
      player_updateAutonav( real_dt );
      update_all(); /* update game */
   }
   else if (!dialogue_isOpen()) {
      /* We run the exclusion end here to handle any hooks that are potentially manually triggered by hook.trigger. */
      hook_exclusionEnd( 0. );
   }

   /* Safe hook should be run every frame regardless of whether game is paused or not. */
   hooks_run( "safe" );

   /* Checks to see if we want to land. */
   space_checkLand();

   /*
    * Handle render.
    */
   if (!quit) {
      /* Clear buffer. */
      render_all( game_dt, real_dt );
      /* Draw buffer. */
      SDL_GL_SwapWindow( gl_screen.window );
   }
}


/**
 * @brief Wrapper for gl_resize that handles non-GL reinitialization.
 */
void naev_resize (void)
{
   /* Auto-detect window size. */
   int w, h;
   SDL_GL_GetDrawableSize( gl_screen.window, &w, &h );

   /* Update options menu, if open. (Never skip, in case the fullscreen mode alone changed.) */
   opt_resize();

   /* Nothing to do. */
   if ((w == gl_screen.rw) && (h == gl_screen.rh))
      return;

   /* Resize the GL context, etc. */
   gl_resize();

   /* Regenerate the background stars. */
   if (cur_system != NULL)
      background_initStars( cur_system->stars );
   else
      background_initStars( 1000. ); /* from loadscreen_load */

   /* Must be before gui_reload */
   fps_setPos( 15., (double)(SCREEN_H-15-gl_defFont.h) );

   /* Reload the GUI (may regenerate land window) */
   gui_reload();

   /* Resets dimensions in other components which care. */
   ovr_refresh();
   toolkit_reposition();
   menu_main_resize();
   nebu_resize();
}

/*
 * @brief Toggles between windowed and fullscreen mode.
 */
void naev_toggleFullscreen (void)
{
   opt_setVideoMode( conf.width, conf.height, !conf.fullscreen, 0 );
}


#if HAS_POSIX && defined(CLOCK_MONOTONIC)
static struct timespec global_time; /**< Global timestamp for calculating delta ticks. */
static int use_posix_time; /**< Whether or not to use POSIX time. */
#endif /* HAS_POSIX && defined(CLOCK_MONOTONIC) */
/**
 * @brief Initializes the fps engine.
 */
static void fps_init (void)
{
#if HAS_POSIX && defined(CLOCK_MONOTONIC)
   use_posix_time = 1;
   /* We must use clock_gettime here instead of gettimeofday mainly because this
    * way we are not influenced by changes to the time source like say ntp which
    * could skew up the dt calculations. */
   if (clock_gettime(CLOCK_MONOTONIC, &global_time)==0)
      return;
   WARN( _("clock_gettime failed, disabling POSIX time.") );
   use_posix_time = 0;
#endif /* HAS_POSIX && defined(CLOCK_MONOTONIC) */
   time_ms  = SDL_GetTicks();
}
/**
 * @brief Gets the elapsed time.
 *
 *    @return The elapsed time from the last frame.
 */
static double fps_elapsed (void)
{
   double dt;
   unsigned int t;

#if HAS_POSIX && defined(CLOCK_MONOTONIC)
   struct timespec ts;

   if (use_posix_time) {
      if (clock_gettime(CLOCK_MONOTONIC, &ts)==0) {
         dt  = ts.tv_sec - global_time.tv_sec;
         dt += (ts.tv_nsec - global_time.tv_nsec) / 1e9;
         global_time = ts;
         return dt;
      }
      WARN( _("clock_gettime failed!") );
   }
#endif /* HAS_POSIX && defined(CLOCK_MONOTONIC) */

   t        = SDL_GetTicks();
   dt       = (double)(t - time_ms); /* Get the elapsed ms. */
   dt      /= 1000.; /* Convert to seconds. */
   time_ms  = t;

   return dt;
}


/**
 * @brief Controls the FPS.
 */
static void fps_control (void)
{
   double delay;
   double fps_max;
#if HAS_POSIX
   struct timespec ts;
#endif /* HAS_POSIX */

   /* dt in s */
   real_dt  = fps_elapsed();
   game_dt  = real_dt * dt_mod; /* Apply the modifier. */

   /* if fps is limited */
   if (!conf.vsync && conf.fps_max != 0) {
      fps_max = 1./(double)conf.fps_max;
      if (real_dt < fps_max) {
         delay    = fps_max - real_dt;
#if HAS_POSIX
         ts.tv_sec  = floor( delay );
         ts.tv_nsec = fmod( delay, 1. ) * 1e9;
         nanosleep( &ts, NULL );
#else /* HAS_POSIX */
         SDL_Delay( (unsigned int)(delay * 1000) );
#endif /* HAS_POSIX */
         fps_dt  += delay; /* makes sure it displays the proper fps */
      }
   }
}


/**
 * @brief Sets the position to display the FPS.
 */
void fps_setPos( double x, double y )
{
   fps_x = x;
   fps_y = y;
}


/**
 * @brief Displays FPS on the screen.
 *
 *    @param[in] dt Current delta tick.
 */
void display_fps( const double dt )
{
   double x,y;
   double dt_mod_base = 1.;

   fps_dt  += dt;
   fps_cur += 1.;
   if (fps_dt > 1.) { /* recalculate every second */
      fps = fps_cur / fps_dt;
      fps_dt = fps_cur = 0.;
   }

   x = fps_x;
   y = fps_y;
   if (conf.fps_show) {
      gl_print( NULL, x, y, NULL, _("%.2f FPS"), fps );
      y -= gl_defFont.h + 5.;
   }

   if ((player.p != NULL) && !player_isFlag(PLAYER_DESTROYED) &&
         !player_isFlag(PLAYER_CREATING)
         && !player_isFlag(PLAYER_CINEMATICS)) {
      dt_mod_base = player_dt_default();
   }
   if (dt_mod != dt_mod_base)
      /* Translators: "TC" is short for "Time Compression". Please
       * substitute "TC" for a shortened form of "Time Compression" or
       * just "Time" that makes sense for the language being translated
       * to. The number displayed is the rate of time passage as a
       * percentage (e.g. 200% if time is passing twice as fast as
       * normal). */
      gl_print(NULL, x, y, NULL, _("TC %.0f%%"), 100. * dt_mod / dt_mod_base);

   if (!paused || !player_paused || !conf.pause_show)
      return;

   y = SCREEN_H / 3. - gl_defFontMono.h / 2.;
   gl_printMidRaw( &gl_defFontMono, SCREEN_W, 0., y,
         NULL, -1., _("PAUSED") );
}


/**
 * @brief Updates the game itself (player flying around and friends).
 *
 *    @brief Mainly uses game dt.
 */
static void update_all (void)
{
   int i, n;
   double nf, microdt, accumdt;

   if ((real_dt > 0.25) && (fps_skipped==0)) { /* slow timers down and rerun calculations */
      fps_skipped = 1;
      return;
   }
   else if (game_dt > fps_min) { /* we'll force a minimum FPS for physics to work alright. */

      /* Number of frames. */
      nf = ceil( game_dt / fps_min );
      microdt = game_dt / nf;
      n  = (int) nf;

      /* Update as much as needed, evenly. */
      accumdt = 0.;
      for (i=0; i<n; i++) {
         update_routine( microdt, 0 );
         /* OK, so we need a bit of hackish logic here in case we are chopping up a
          * very large dt and it turns out time compression changes so we're now
          * updating in "normal time compression" zone. This amounts to many updates
          * being run when time compression has changed and thus can cause, say, the
          * player to exceed their target position or get mauled by an enemy ship.
          */
         accumdt += microdt;
         if (accumdt > dt_mod*real_dt)
            break;
      }

      /* Note we don't touch game_dt so that fps_display works well */
   }
   else /* Standard, just update with the last dt */
      update_routine( game_dt, 0 );

   fps_skipped = 0;
}


/**
 * @brief Actually runs the updates
 *
 *    @param[in] dt Current delta tick.
 *    @param[in] enter_sys Whether this is the initial update upon entering the system.
 */
void update_routine( double dt, int enter_sys )
{
   HookParam h[3];

   if (!enter_sys) {
      hook_exclusionStart();

      /* Update time. */
      ntime_update( dt );
   }

   /* Update engine stuff. */
   space_update(dt);
   weapons_update(dt);
   spfx_update(dt, real_dt);
   pilots_update(dt);

   /* Update camera. */
   cam_update( dt );

   if (!enter_sys) {
      hook_exclusionEnd( dt );

      /* Hook set up. */
      h[0].type = HOOK_PARAM_NUMBER;
      h[0].u.num = dt;
      h[1].type = HOOK_PARAM_NUMBER;
      h[1].u.num = real_dt;
      h[2].type = HOOK_PARAM_SENTINEL;
      /* Run the update hook. */
      hooks_runParam( "update", h );
   }
}


/**
 * @brief Sets the window caption.
 */
static void window_caption (void)
{
   char buf[PATH_MAX];
   SDL_RWops *rw;

   /* Load icon. */
   rw = PHYSFSRWOPS_openRead( GFX_PATH"icon.png" );
   if (rw == NULL) {
      WARN( _("Icon (icon.png) not found!") );
      return;
   }
   naev_icon   = IMG_Load_RW( rw, 1 );
   if (naev_icon == NULL) {
      WARN( _("Unable to load icon.png!") );
      return;
   }

   /* Set caption. */
   snprintf(buf, sizeof(buf), APPNAME" – %s", start_name());
   SDL_SetWindowTitle( gl_screen.window, buf );
   SDL_SetWindowIcon(  gl_screen.window, naev_icon );
}


/**
 * @brief Returns the version in a human readable string.
 *
 *    @param long_version Returns the long version if it's long.
 *    @return The human readable version string.
 */
char *naev_version( int long_version )
{
   /* Set up the long version. */
   if (long_version) {
      if (version_human[0] == '\0')
         snprintf( version_human, sizeof(version_human),
               " "APPNAME" v%s%s – %s", VERSION,
#ifdef DEBUGGING
               _(" debug"),
#else /* DEBUGGING */
               "",
#endif /* DEBUGGING */
               start_name() );
      return version_human;
   }

   return VERSION;
}


static int
binary_comparison (int x, int y) {
  if (x == y) return 0;
  if (x > y) return 1;
  return -1;
}
/**
 * @brief Compares the version against the current naev version.
 *
 *    @param version The version string to check.
 *    @return a number: postive if the binary version is newer than the
 *       checked version string, or negative if the binary version is
 *       older than the checked version string; and with a magnitude of
 *       3 if the difference is in the major version number, 2 if the
 *       difference is in the minor version number, and 1 if the
 *       difference is in the bugfix version number.
 */
int naev_versionCompare( const char *version )
{
   int res;
   semver_t sv;

   if (semver_parse( version, &sv ))
   {
      WARN( _("Failed to parse version string '%s'!"), version );
      return -1;
   }

   if ((res = 3*binary_comparison(version_binary.major, sv.major)) == 0) {
      if ((res = 2*binary_comparison(version_binary.minor, sv.minor)) == 0) {
         res = semver_compare( version_binary, sv );
      }
   }
   semver_free( &sv );
   return res;
}


/**
 * @brief Prints the SDL version to console.
 */
static void print_SDLversion (void)
{
   const SDL_version *linked;
   SDL_version compiled;
   unsigned int version_linked, version_compiled;

   /* Extract information. */
   SDL_VERSION(&compiled);
   SDL_version ll;
   SDL_GetVersion( &ll );
   linked = &ll;
   DEBUG( _("SDL: %d.%d.%d [compiled: %d.%d.%d]"),
         linked->major, linked->minor, linked->patch,
         compiled.major, compiled.minor, compiled.patch);

   /* Get version as number. */
   version_linked    = linked->major*100 + linked->minor;
   version_compiled  = compiled.major*100 + compiled.minor;

   /* Check if major/minor version differ. */
   if (version_linked > version_compiled)
      WARN( _("SDL is newer than compiled version") );
   if (version_linked < version_compiled)
      WARN( _("SDL is older than compiled version.") );
}

double naev_getrealdt (void)
{
   return real_dt;
}

