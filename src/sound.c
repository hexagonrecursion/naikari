/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file sound.c
 *
 * @brief Handles all the sound details.
 */


#include "sound.h"

#include "naev.h"

#include <sys/stat.h>

#include "SDL.h"
#include "SDL_thread.h"

#include "sound_priv.h"
#include "sound_openal.h"
#include "sound_sdlmix.h"
#include "log.h"
#include "ndata.h"
#include "music.h"
#include "physics.h"
#include "conf.h"


#define SOUND_PREFIX       "snd/sounds/" /**< Prefix of where to find sounds. */
#define SOUND_SUFFIX_WAV   ".wav" /**< Suffix of sounds. */
#define SOUND_SUFFIX_OGG   ".ogg" /**< Suffix of sounds. */


/*
 * Global sound properties.
 */
int sound_disabled            = 0; /**< Whether sound is disabled. */
static int sound_reserved     = 0; /**< Amount of reserved channels. */


/*
 * Sound list.
 */
static alSound *sound_list    = NULL; /**< List of available sounds. */
static int sound_nlist        = 0; /**< Number of available sounds. */


/*
 * Voices.
 */
static int voice_genid        = 0; /**< Voice identifier generator. */
alVoice *voice_active  = NULL; /**< Active voices. */
static alVoice *voice_pool    = NULL; /**< Pool of free voices. */



/*
 * Function pointers for backends.
 */
/* Creation. */
int  (*sound_sys_init) (void)          = NULL;
void (*sound_sys_exit) (void)          = NULL;
 /* Sound creation. */
int  (*sound_sys_load) ( alSound *snd, const char *filename ) = NULL;
void (*sound_sys_free) ( alSound *snd ) = NULL;
 /* Sound settings. */
int  (*sound_sys_volume) ( const double vol ) = NULL;
double (*sound_sys_getVolume) (void)   = NULL;
 /* Sound playing. */
int  (*sound_sys_play) ( alVoice *v, alSound *s )   = NULL;
int  (*sound_sys_playPos) ( alVoice *v, alSound *s,
      double px, double py, double vx, double vy ) = NULL;
int  (*sound_sys_updatePos) ( alVoice *v, double px, double py,
      double vx, double vy )           = NULL;
void (*sound_sys_updateVoice) ( alVoice *v ) = NULL;
 /* Sound management. */
void (*sound_sys_stop) ( alVoice *v )  = NULL;
void (*sound_sys_pause) (void)         = NULL;
void (*sound_sys_resume) (void)        = NULL;
/* Listener. */
int (*sound_sys_updateListener) ( double dir, double px, double py,
      double vx, double vy )           = NULL;



/*
 * prototypes
 */
/* General. */
static int sound_makeList (void);
static int sound_load( alSound *snd, const char *filename );
static void sound_free( alSound *snd );
/* Voices. */


/**
 * @brief Initializes the sound subsystem.
 *
 *    @return 0 on success.
 */
int sound_init (void)
{
   int ret;

   /* See if sound is disabled. */
   if (conf.nosound) {
      sound_disabled = 1;
      music_disabled = 1;
   }

   /* Parse conf. */
   if (sound_disabled && music_disabled)
      return 0;

   /* Choose sound system. */
   if ((conf.sound_backend != NULL) &&
         strcmp(conf.sound_backend,"sdlmix")) {
#if USE_SDLMIX
      /*
       * SDL_mixer Sound.
       */
      /* Creation. */
      sound_sys_init       = sound_mix_init;
      sound_sys_exit       = sound_mix_exit;
      /* Sound Creation. */
      sound_sys_load       = sound_mix_load;
      sound_sys_free       = sound_mix_free;
      /* Sound settings. */
      sound_sys_volume     = sound_mix_volume;
      sound_sys_getVolume  = sound_mix_getVolume;
      /* Sound playing. */
      sound_sys_play       = sound_mix_play;
      sound_sys_playPos    = sound_mix_playPos;
      sound_sys_updatePos  = sound_mix_updatePos;
      sound_sys_updateVoice = sound_mix_updateVoice;
      /* Sound management. */
      sound_sys_stop       = sound_mix_stop;
      sound_sys_pause      = sound_mix_pause;
      sound_sys_resume     = sound_mix_resume;
      /* Listener. */
      sound_sys_updateListener = sound_mix_updateListener;
#else /* USE_SDLMIX */
      WARN("SDL_mixer support not compiled in!");
      sound_disabled = 1;
      music_disabled = 1;
      return 0;
#endif /* USE_SDLMIX */
   }
   else if ((conf.sound_backend != NULL) &&
         strcmp(conf.sound_backend,"openal")) {
#if USE_OPENAL
      /*
       * OpenAL Sound.
       */
      /* Creation. */
      sound_sys_init       = sound_al_init;
      sound_sys_exit       = sound_al_exit;
      /* Sound Creation. */
      sound_sys_load       = sound_al_load;
      sound_sys_exit       = sound_al_exit;
      /* Sound settings. */
      sound_sys_volume     = sound_al_volume;
      sound_sys_getVolume  = sound_al_getVolume;
      /* Sound playing. */
      sound_sys_play       = sound_al_play;
      sound_sys_playPos    = sound_al_playPos;
      sound_sys_updatePos  = sound_al_updatePos;
      /* Sound management. */
      sound_sys_stop       = sound_al_stop;
      sound_sys_pause      = sound_al_pause;
      sound_sys_resume     = sound_al_resume;
      /* Listener. */
      sound_sys_updateListener = sound_al_updateListener;
#else /* USE_OPENAL */
      WARN("OpenAL support not compiled in!");
      sound_disabled = 1;
      music_disabled = 1;
      return 0;
#endif /* USE_OPENAL */
   }
   else {
      WARN("Unknown sound backend '%s'.", conf.sound_backend);
      sound_disabled = 1;
      music_disabled = 1;
      return 0;
   }

   /* Initialize subsystems. */
   ret = sound_sys_init();
   if (ret != 0) {
      sound_disabled = 1;
      music_disabled = 1;
      return ret;
   }
   ret = music_init();
   if (ret != 0) {
      music_disabled = 1;
   }

   /* Load available sounds. */
   ret = sound_makeList();
   if (ret != 0)
      return ret;

   /* Set volume. */
   if ((conf.sound > 1.) || (conf.sound < 0.))
      WARN("Sound has invalid value, clamping to [0:1].");
   sound_volume(conf.sound);

   return 0;
}


/**
 * @brief Cleans up after the sound subsytem.
 */
void sound_exit (void)
{
   int i;
   alVoice *v;

   /* Exit music subsystem. */
   music_exit();

   /* free the voices. */
   while (voice_active != NULL) {
      v = voice_active;
      voice_active = v->next;
      free(v);
   }
   while (voice_pool != NULL) {
      v = voice_pool;
      voice_pool = v->next;
      free(v);
   }

   /* free the sounds */
   for (i=0; i<sound_nlist; i++)
      sound_free( &sound_list[i] );
   free( sound_list );
   sound_list = NULL;
   sound_nlist = 0;

   /* Exit sound subsystem. */
   sound_sys_exit();
}


/**
 * @brief Gets the buffer to sound of name.
 *
 *    @param name Name of the sound to get it's id.
 *    @return ID of the sound matching name.
 */
int sound_get( char* name ) 
{
   int i;

   if (sound_disabled) return 0;

   for (i=0; i<sound_nlist; i++)
      if (strcmp(name, sound_list[i].name)==0) {
         return i;
      }
   WARN("Sound '%s' not found in sound list", name);
   return -1;
}


/**
 * @brief Plays the sound in the first available channel.
 *
 *    @param sound Sound to play.
 *    @return Voice identifier on success.
 */
int sound_play( int sound )
{
   alVoice *v;
   alSound *s;

   if (sound_disabled) return 0;

   if ((sound < 0) || (sound > sound_nlist))
      return -1;

   /* Gets a new voice. */
   v = voice_new();

   /* Get the sound. */
   s = &sound_list[sound];

   /* Try to play the sound. */
   if (sound_sys_play( v, s ))
      return -1;

   /* Set state and add to list. */
   v->state = VOICE_PLAYING;
   v->id = ++voice_genid;
   voice_add(v);

   return v->id;
}


/**
 * @brief Plays a sound based on position.
 *
 *    @param sound Sound to play.
 *    @param px X position of the sound.
 *    @param py Y position of the sound.
 *    @param vx X velocity of the sound.
 *    @param vy Y velocity of the sound.
 *    @return 0 on success.
 */
int sound_playPos( int sound, double px, double py, double vx, double vy )
{
   alVoice *v;
   alSound *s;

   if (sound_disabled) return 0;

   if ((sound < 0) || (sound > sound_nlist))
      return -1;

   /* Gets a new voice. */
   v = voice_new();

   /* Get the sound. */
   s = &sound_list[sound];

   /* Try to play the sound. */
   if (sound_sys_playPos( v, s, px, py, vx, vy ))
      return -1;

   /* Actually add the voice to the list. */
   v->state = VOICE_PLAYING;
   v->id = ++voice_genid;
   voice_add(v);

   return v->id;
}


/**
 * @brief Updates the position of a voice.
 *
 *    @param voice Identifier of the voice to update.
 *    @param x New x position to update to.
 *    @param y New y position to update to.
 */
int sound_updatePos( int voice, double px, double py, double vx, double vy )
{
   alVoice *v;

   if (sound_disabled) return 0;

   v = voice_get(voice);
   if (v != NULL) {

      /* Update the voice. */
      if (sound_sys_updatePos( v, px, py, vx, vy))
         return -1;
   }

   return 0;
}


/**
 * @brief Updates the sonuds removing obsolete ones and such.
 *
 *    @return 0 on success.
 */
int sound_update (void)
{
   alVoice *v, *tv;

   /* Update music if needed. */
   music_update();

   if (sound_disabled)
      return 0;

   if (voice_active == NULL)
      return 0;

   /* The actual control loop. */
   for (v=voice_active; v!=NULL; v=v->next) {

      /* Destroy and toss into pool. */
      if ((v->state == VOICE_STOPPED) || (v->state == VOICE_DESTROY)) {

         /* Remove from active list. */
         tv = v->prev;
         if (tv == NULL) {
            voice_active = v->next;
            if (voice_active != NULL)
               voice_active->prev = NULL;
         }
         else {
            tv->next = v->next;
            if (tv->next != NULL)
               tv->next->prev = tv;
         }

         /* Add to free pool. */
         v->next = voice_pool;
         v->prev = NULL;
         voice_pool = v;
         if (v->next != NULL)
            v->next->prev = v;

         /* Avoid loop blockage. */
         v = (tv != NULL) ? tv->next : voice_active;
         if (v == NULL) break;
      }

      sound_sys_updateVoice( v );
   }

   return 0;
}


/**
 * @brief Pauses all the sounds.
 */
void sound_pause (void)
{
   sound_sys_pause();
}


/**
 * @brief Resumes all the sounds.
 */
void sound_resume (void)
{
   sound_sys_resume();
}


/**
 * @brief Stops a voice from playing.
 *
 *    @param voice Identifier of the voice to stop.
 */
void sound_stop( int voice )
{
   alVoice *v;

   if (sound_disabled) return;

   v = voice_get(voice);
   if (v != NULL) {
      sound_sys_stop( v );
      v->state = VOICE_STOPPED;
   }

}


/**
 * @brief Updates the sound listener.
 *
 *    @param dir Direction listener is facing.
 *    @param px X position of the listener.
 *    @param py Y position of the listener.
 *    @param vx X velocity of the listener.
 *    @param vy Y velocity of the listener.
 *    @return 0 on success.
 *
 * @sa sound_playPos
 */
int sound_updateListener( double dir, double px, double py,
      double vx, double vy )
{
   if (sound_disabled) return 0;

   return sound_sys_updateListener( dir, px, py, vx, vy );
}


/**
 * @brief Makes the list of available sounds.
 */
static int sound_makeList (void)
{
   char** files;
   uint32_t nfiles,i;
   char path[PATH_MAX];
   char tmp[64];
   int len, suflen, flen;
   int mem;

   if (sound_disabled) return 0;

   /* get the file list */
   files = ndata_list( SOUND_PREFIX, &nfiles );

   /* load the profiles */
   mem = 0;
   suflen = strlen(SOUND_SUFFIX_WAV);
   for (i=0; i<nfiles; i++) {
      flen = strlen(files[i]);

      /* Must be longer then suffix. */
      if (flen < suflen) {
         free(files[i]);
         continue;
      }

      /* Make sure is wav or ogg. */
      if ((strncmp( &files[i][flen - suflen], SOUND_SUFFIX_WAV, suflen)!=0) &&
            (strncmp( &files[i][flen - suflen], SOUND_SUFFIX_OGG, suflen)!=0)) {
         free(files[i]);
         continue;
      }

      /* grow the selection size */
      sound_nlist++;
      if (sound_nlist > mem) { /* we must grow */
         mem += 32; /* we'll overallocate most likely */
         sound_list = realloc( sound_list, mem*sizeof(alSound));
      }

      /* remove the suffix */
      len = flen - suflen;
      strncpy( tmp, files[i], len );
      tmp[len] = '\0';

      /* Load the sound. */
      sound_list[sound_nlist-1].name = strdup(tmp);
      snprintf( path, PATH_MAX, SOUND_PREFIX"%s", files[i] );
      if (sound_load( &sound_list[sound_nlist-1], path )) {
         sound_nlist--; /* Song not actually added. */
      }

      /* Clean up. */
      free(files[i]);
   }
   /* shrink to minimum ram usage */
   sound_list = realloc( sound_list, sound_nlist*sizeof(alSound));

   DEBUG("Loaded %d sound%s", sound_nlist, (sound_nlist==1)?"":"s");

   /* More clean up. */
   free(files);

   return 0;
}


/**
 * @brief Sets the volume.
 *
 *    @param vol Volume to set to.
 *    @return 0 on success.
 */
int sound_volume( const double vol )
{
   if (sound_disabled) return 0;

   return sound_sys_volume( vol );
}


/**
 * @brief Gets the current sound volume.
 *
 *    @return The current sound volume level.
 */
double sound_getVolume (void)
{
   return sound_sys_getVolume();
}


/**
 * @brief Loads a sound into the sound_list.
 *
 *    @param snd Sound to load into.
 *    @param filename Name fo the file to load.
 *    @return 0 on success.
 *
 * @sa sound_makeList
 */
static int sound_load( alSound *snd, const char *filename )
{
   if (sound_disabled) return -1;

   return sound_sys_load( snd, filename );
}


/**
 * @brief Frees the sound.
 *
 *    @param snd Sound to free.
 */
static void sound_free( alSound *snd )
{
   /* Free general stuff. */
   if (snd->name) {
      free(snd->name);
      snd->name = NULL;
   }
   
   /* Free internals. */
   sound_sys_free(snd);
}


/**
 * @brief Reserves num channels.
 *
 *    @param num Number of channels to reserve.
 *    @return 0 on success.
 */
int sound_reserve( int num )
{
   int ret;

   if (sound_disabled) return 0;

   sound_reserved += num;
   ret = Mix_ReserveChannels(num);

   if (ret != sound_reserved) {
      WARN("Unable to reserve %d channels: %s", sound_reserved, Mix_GetError());
      return -1;
   }

   return 0;
}


/**
 * @brief Creates a sound group.
 *
 *    @param tag Identifier of the group to creat.
 *    @param start Where to start creating the group.
 *    @param size Size of the group.
 *    @return 0 on success.
 */
int sound_createGroup( int tag, int start, int size )
{
   int ret;

   if (sound_disabled) return 0;

   ret = Mix_GroupChannels( start, start+size-1, tag );

   if (ret != size) {
      WARN("Unable to create sound group: %s", Mix_GetError());
      return -1;
   }

   return 0;
}


/**
 * @brief Plays a sound in a group.
 *
 *    @param group Group to play sound in.
 *    @param sound Sound to play.
 *    @param once Whether to play only once.
 *    @return 0 on success.
 */
int sound_playGroup( int group, int sound, int once )
{
   int ret, channel;

   if (sound_disabled) return 0;

   channel = Mix_GroupAvailable(group);
   if (channel == -1) {
      WARN("Group '%d' has no free channels!", group);
      return -1;
   }

   ret = Mix_PlayChannel( channel, sound_list[sound].u.mix.buf,
         (once == 0) ? -1 : 0 );
   if (ret < 0) {
      WARN("Unable to play sound %d for group %d: %s",
            sound, group, Mix_GetError());
      return -1;
   }

   return 0;
}


/**
 * @brief Stops all the sounds in a group.
 *
 *    @param group Group to stop all it's sounds.
 */
void sound_stopGroup( int group )
{
   (void) group;
   if (sound_disabled) return;

   /*sound_sys_stopGroup( group );*/
}


/**
 * @brief Gets a new voice ready to be used.
 *
 *    @return New voice ready to use.
 */
alVoice* voice_new (void)
{
   alVoice *v;

   /* No free voices, allocate a new one. */
   if (voice_pool == NULL) {
      v = malloc(sizeof(alVoice));
      memset(v, 0, sizeof(alVoice));
      voice_pool = v;
      return v;
   }

   /* First free voice. */
   v = voice_pool; /* We do not touch the next nor prev, it's still in the pool. */
   return v;
}


/**
 * @brief Adds a voice to the active voice stack.
 *
 *    @param v Voice to add to the active voice stack.
 *    @return 0 on success.
 */
int voice_add( alVoice* v )
{
   alVoice *tv;

   /* Remove from pool. */
   if (v->prev != NULL) {
      tv = v->prev;
      tv->next = v->next;
      if (tv->next != NULL)
         tv->next->prev = tv;
   }
   else { /* Set pool to be the next. */
      voice_pool = v->next;
      if (voice_pool != NULL)
         voice_pool->prev = NULL;
   }

   /* Insert to the front of active voices. */
   tv = voice_active;
   v->next = tv;
   v->prev = NULL;
   voice_active = v;
   if (tv != NULL)
      tv->prev = v;
   return 0;
}


/**
 * @brief Gets a voice by identifier.
 *
 *    @param id Identifier to look for.
 *    @return Voice matching identifier or NULL if not found.
 */
alVoice* voice_get( int id )
{
   alVoice *v;

   if (voice_active==NULL) return NULL;

   for (v=voice_active; v!=NULL; v=v->next)
      if (v->id == id) {
         return v;
      }

   return NULL;
}

