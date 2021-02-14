/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nebula.c
 *
 * @brief Handles rendering and generating the nebula.
 */

/** @cond */
#include "naev.h"
/** @endcond */

#include "nebula.h"

#include "camera.h"
#include "conf.h"
#include "gui.h"
#include "log.h"
#include "menu.h"
#include "opengl.h"
#include "perlin.h"
#include "player.h"
#include "rng.h"
#include "spfx.h"


#define NEBULA_PUFFS         32 /**< Amount of puffs to generate */
#define NEBULA_PUFF_BUFFER   300 /**< Nebula buffer */


/* Nebula properties */
static double nebu_density = 0.; /**< The density. */
static double nebu_view = 0.; /**< How far player can see. */
static double nebu_dt   = 0.; /**< How fast nebula changes. */
static double nebu_time = 0.; /**< Timer since last render. */

/* Nebula scaling stuff. */
static double nebu_scale = 4.; /**< How much to scale nebula. */
static int nebu_dofbo    = 0;
static GLuint nebu_fbo   = GL_INVALID_VALUE;
static GLuint nebu_tex   = GL_INVALID_VALUE;
static GLfloat nebu_render_w= 0.;
static GLfloat nebu_render_h= 0.;
static gl_Matrix4 nebu_render_P;

/* puff textures */
static glTexture *nebu_pufftexs[NEBULA_PUFFS]; /**< Nebula puffs. */


/**
 * @struct NebulaPuff
 *
 * @brief Represents a nebula puff.
 */
typedef struct NebulaPuff_ {
   double x; /**< X position. */
   double y; /**< Y position */
   double height; /**< height vs player */
   int tex; /**< Texture */
} NebulaPuff;
static NebulaPuff *nebu_puffs = NULL; /**< Stack of puffs. */
static int nebu_npuffs        = 0; /**< Number of puffs. */
static double puff_x          = 0.;
static double puff_y          = 0.;


/*
 * prototypes
 */
static SDL_Surface* nebu_surfaceFromNebulaMap( float* map, const int w, const int h );
/* Puffs. */
static void nebu_generatePuffs (void);
static void nebu_renderPuffs( int below_player );
/* Nebula render methods. */
static void nebu_renderBackground( const double dt );
static void nebu_blitFBO (void);


/**
 * @brief Initializes the nebula.
 *
 *    @return 0 on success.
 */
int nebu_init (void)
{
   nebu_generatePuffs();
   return nebu_resize();
}


/**
 * @brief Handles a screen s
 *
 *    @return 0 on success.
 */
int nebu_resize (void)
{
   double scale;
   GLfloat fbo_w, fbo_h;
   GLenum status;

   scale = conf.nebu_scale * gl_screen.scale;
   fbo_w = round(gl_screen.nw/scale);
   fbo_h = round(gl_screen.nh/scale);
   if (scale == nebu_scale && fbo_w == nebu_render_w && fbo_h == nebu_render_h)
      return 0;

   nebu_scale = scale;
   nebu_render_w = fbo_w;
   nebu_render_h = fbo_h;
   nebu_dofbo = (nebu_scale != 1.);
   glDeleteTextures( 1, &nebu_tex );
   glDeleteFramebuffers( 1, &nebu_fbo );

   if (nebu_dofbo) {
      /* Create the render buffer. */
      glGenTextures(1, &nebu_tex);
      glBindTexture(GL_TEXTURE_2D, nebu_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nebu_render_w, nebu_render_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_2D, 0);

      /* Create the frame buffer. */
      glGenFramebuffers( 1, &nebu_fbo );
      glBindFramebuffer(GL_FRAMEBUFFER, nebu_fbo);

      /* Attach the colour buffer. */
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nebu_tex, 0);

      /* Check status. */
      status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (status != GL_FRAMEBUFFER_COMPLETE)
         WARN(_("Error setting up nebula framebuffer!"));

      /* Restore state. */
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      gl_checkErr();
   }

   /* Set up the matrices. */
   nebu_render_P = gl_Matrix4_Identity();
   nebu_render_P = gl_Matrix4_Translate(nebu_render_P, -nebu_render_w/2., -nebu_render_h/2., 0. );
   nebu_render_P = gl_Matrix4_Scale(nebu_render_P, nebu_render_w, nebu_render_h, 1);

   return 0;
}


/**
 * @brief Gets the nebula view radius.
 *
 *    @return The nebula view radius.
 */
double nebu_getSightRadius (void)
{
   return nebu_view;
}


/**
 * @brief Cleans up the nebu subsystem.
 */
void nebu_exit (void)
{
   int i;

   /* Free the puffs. */
   for (i=0; i<NEBULA_PUFFS; i++)
      gl_freeTexture( nebu_pufftexs[i] );

   if (nebu_dofbo) {
      glDeleteFramebuffers( 1, &nebu_fbo );
      glDeleteTextures( 1, &nebu_tex );
   }
}


/**
 * @brief Renders the nebula.
 *
 *    @param dt Current delta tick.
 */
void nebu_render( const double dt )
{
   nebu_renderBackground(dt);
   nebu_renderPuffs( 1 );
}


/**
 * @brief Renders the nebula using the multitexture approach.
 *
 *    @param dt Current delta tick.
 */
static void nebu_renderBackground( const double dt )
{
   /* calculate frame to draw */
   nebu_time += dt * nebu_dt;

   if (nebu_dofbo) {
      glBindFramebuffer(GL_FRAMEBUFFER, nebu_fbo);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }

   /* Start the program. */
   glUseProgram(shaders.nebula_background.program);

   /* Set shader uniforms. */
   gl_uniformColor(shaders.nebula_background.color, &cBlue);
   gl_Matrix4_Uniform(shaders.nebula_background.projection, nebu_render_P);
   glUniform2f(shaders.nebula_background.center, -nebu_render_P.m[3][0], -nebu_render_P.m[3][1]);
   glUniform1f(shaders.nebula_background.radius, nebu_view * cam_getZoom() / nebu_scale);
   glUniform1f(shaders.nebula_background.time, nebu_time);

   /* Draw. */
   glEnableVertexAttribArray( shaders.nebula_background.vertex );
   gl_vboActivateAttribOffset( gl_squareVBO, shaders.nebula_background.vertex, 0, 2, GL_FLOAT, 0 );
   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
   nebu_blitFBO();

   /* Clean up. */
   glDisableVertexAttribArray( shaders.nebula_background.vertex );
   glUseProgram(0);
   gl_checkErr();
}


/**
 * @brief If we're drawing the nebula buffered, copy to the screen.
 */
static void nebu_blitFBO (void)
{
   if (nebu_dofbo) {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      glUseProgram(shaders.texture.program);

      glBindTexture( GL_TEXTURE_2D, nebu_tex );

      glEnableVertexAttribArray( shaders.texture.vertex );
      gl_vboActivateAttribOffset( gl_squareVBO, shaders.texture.vertex,
            0, 2, GL_FLOAT, 0 );

      /* Set shader uniforms. */
      gl_uniformColor(shaders.texture.color, &cWhite);
      gl_Matrix4_Uniform(shaders.texture.projection, gl_Matrix4_Ortho(0, 1, 0, 1, 1, -1));
      gl_Matrix4_Uniform(shaders.texture.tex_mat, gl_Matrix4_Identity());

      /* Draw. */
      glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

      /* Clear state. */
      glDisableVertexAttribArray( shaders.texture.vertex );
   }
}


/**
 * @brief Updates visibility and stuff.
 */
void nebu_update( double dt )
{
   (void) dt;
   double mod = 1.;

   if (player.p != NULL)
      mod = player.p->ew_detect;

   /* At density 1000 you have zero visibility. */
   nebu_view = (1000. - nebu_density) * mod;
}


/**
 * @brief Renders the nebula overlay (hides what player can't see).
 *
 *    @param dt Current delta tick.
 */
void nebu_renderOverlay( const double dt )
{
   (void) dt;
   double gx, gy;
   double z;

   /* Get GUI offsets. */
   gui_getOffset( &gx, &gy );

   /* Get zoom. */
   z = cam_getZoom();

   /*
    * Renders the puffs
    */
   nebu_renderPuffs( 0 );

   /* Prepare the matrix */
   /* TODO: translate according to spfx_getShake()? */
   if (nebu_dofbo) {
      glBindFramebuffer(GL_FRAMEBUFFER, nebu_fbo);
      glClearColor( 0., 0., 0., 0. );
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glDisable( GL_DEPTH_TEST );
   }

   /* Start the program. */
   glUseProgram(shaders.nebula.program);

   /* Set shader uniforms. */
   gl_uniformColor(shaders.nebula.color, &cDarkBlue);
   gl_Matrix4_Uniform(shaders.nebula.projection, nebu_render_P);
   glUniform2f(shaders.nebula.center, -nebu_render_P.m[3][0], -nebu_render_P.m[3][1]);
   glUniform1f(shaders.nebula.radius, nebu_view * z / nebu_scale);
   glUniform1f(shaders.nebula.time, nebu_time);

   /* Draw. */
   glEnableVertexAttribArray(shaders.nebula.vertex);
   gl_vboActivateAttribOffset( gl_squareVBO, shaders.nebula.vertex, 0, 2, GL_FLOAT, 0 );
   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
   nebu_blitFBO();

   /* Clean up. */
   glDisableVertexAttribArray( shaders.nebula.vertex );
   glClearColor( 0., 0., 0., 1. );
   glUseProgram(0);
   gl_checkErr();

   /* Reset puff movement. */
   puff_x = 0.;
   puff_y = 0.;
}


/**
 * @brief Renders the puffs.
 *
 *    @param below_player Render the puffs below player or above player?
 */
static void nebu_renderPuffs( int below_player )
{
   int i;

   /* Main menu shouldn't have puffs */
   if (menu_isOpen(MENU_MAIN)) return;

   for (i=0; i<nebu_npuffs; i++) {

      /* Separate by layers */
      if ((below_player && (nebu_puffs[i].height < 1.)) ||
            (!below_player && (nebu_puffs[i].height > 1.))) {

         /* calculate new position */
         nebu_puffs[i].x += puff_x * nebu_puffs[i].height;
         nebu_puffs[i].y += puff_y * nebu_puffs[i].height;

         /* Check boundaries */
         if (nebu_puffs[i].x > SCREEN_W + NEBULA_PUFF_BUFFER)
            nebu_puffs[i].x -= SCREEN_W + 2*NEBULA_PUFF_BUFFER;
         else if (nebu_puffs[i].y > SCREEN_H + NEBULA_PUFF_BUFFER)
            nebu_puffs[i].y -= SCREEN_H + 2*NEBULA_PUFF_BUFFER;
         else if (nebu_puffs[i].x < -NEBULA_PUFF_BUFFER)
            nebu_puffs[i].x += SCREEN_W + 2*NEBULA_PUFF_BUFFER;
         else if (nebu_puffs[i].y < -NEBULA_PUFF_BUFFER)
            nebu_puffs[i].y += SCREEN_H + 2*NEBULA_PUFF_BUFFER;

         /* Render */
         gl_blitStatic( nebu_pufftexs[nebu_puffs[i].tex],
               nebu_puffs[i].x, nebu_puffs[i].y, &cLightBlue );
      }
   }
}


/**
 * @brief Moves the nebula puffs.
 */
void nebu_movePuffs( double x, double y )
{
   puff_x += x;
   puff_y += y;
}


/**
 * @brief Prepares the nebualae to be rendered.
 *
 *    @param density Density of the nebula (0-1000).
 *    @param volatility Volatility of the nebula (0-1000).
 */
void nebu_prep( double density, double volatility )
{
   (void)volatility;
   int i;

   nebu_density = density;
   nebu_update( 0. );
   nebu_dt   = (2.*density + 200.) / 10000.; /* Faster at higher density */
   nebu_time = 0.;

   nebu_npuffs = density/2.;
   nebu_puffs = realloc(nebu_puffs, sizeof(NebulaPuff)*nebu_npuffs);
   for (i=0; i<nebu_npuffs; i++) {
      /* Position */
      nebu_puffs[i].x = (double)RNG(-NEBULA_PUFF_BUFFER,
            SCREEN_W + NEBULA_PUFF_BUFFER);
      nebu_puffs[i].y = (double)RNG(-NEBULA_PUFF_BUFFER,
            SCREEN_H + NEBULA_PUFF_BUFFER);

      /* Maybe make size related? */
      nebu_puffs[i].tex = RNG(0,NEBULA_PUFFS-1);
      nebu_puffs[i].height = RNGF() + 0.2;
   }
}


/**
 * @brief Generates nebula puffs.
 */
static void nebu_generatePuffs (void)
{
   int i;
   int w,h;
   SDL_Surface *sur;
   float *nebu;

   /* Generate the nebula puffs */
   for (i=0; i<NEBULA_PUFFS; i++) {
      /* Generate the nebula */
      w = h = RNG(20,64);
      nebu = noise_genNebulaPuffMap( w, h, 1. );
      sur = nebu_surfaceFromNebulaMap( nebu, w, h );
      free(nebu);

      /* Load the texture */
      nebu_pufftexs[i] =  gl_loadImage( sur, 0 );
   }
}


/**
 * @brief Generates a SDL_Surface from a 2d nebula map
 *
 *    @param map Nebula map to use.
 *    @param w Map width.
 *    @param h Map height.
 *    @return A SDL Surface with the nebula.
 */
static SDL_Surface* nebu_surfaceFromNebulaMap( float* map, const int w, const int h )
{
   int i;
   SDL_Surface *sur;
   uint32_t *pix;
   double c;

   /* the good surface */
   sur = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, RGBAMASK );
   pix = sur->pixels;

   /* convert from mapping to actual colours */
   SDL_LockSurface( sur );
   for (i=0; i<h*w; i++) {
      c = map[i];
      pix[i] = RMASK + BMASK + GMASK + (AMASK & (uint32_t)((double)AMASK*c));
   }
   SDL_UnlockSurface( sur );

   return sur;
}
