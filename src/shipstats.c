/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file shipstats.c
 *
 * @brief Handles the ship statistics.
 */


/** @cond */
#include "naev.h"
/** @endcond */

#include "shipstats.h"

#include "log.h"
#include "nstring.h"


/**
 * @brief The data type.
 */
typedef enum StatDataType_ {
   SS_DATA_TYPE_DOUBLE,          /**< Relative [0:inf] value. */
   SS_DATA_TYPE_DOUBLE_ABSOLUTE, /**< Absolute double value. */
   SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT, /**< Absolute double value as a percent. */
   SS_DATA_TYPE_INTEGER,         /**< Absolute integer value. */
   SS_DATA_TYPE_BOOLEAN          /**< Boolean value, defaults 0. */
} StatDataType;


/**
 * @brief Internal look up table for ship stats.
 *
 * Makes it much easier to work with stats at the cost of some minor performance.
 */
typedef struct ShipStatsLookup_ {
   /* Explicitly set. */
   ShipStatsType type;  /**< Type of the stat. */
   const char *name;    /**< Name to look into XML for, must match name in the structure. */
   int ship_redundant;  /**< Indicates whether the stat is redundant when
                             referring to a ship, i.e. the stat is fully
                             contained in the ship's final properties and
                             uninteresting to the player in that context. */
   const char *display; /**< Display name for visibility by player. */
   StatDataType data;   /**< Type of data for the stat. */
   int inverted;        /**< Indicates whether the good value is inverted, by
                             default positive is good, with this set negative
                             is good. */

   /* Self calculated. */
   size_t offset;       /**< Stores the byte offset in the structure. */
} ShipStatsLookup;


/* Flexible do everything macro. */
#define ELEM( t, n, r, dsp, d , i) \
   { .type=t, .ship_redundant=r, .name=#n, .display=dsp, .data=d, .inverted=i, .offset=offsetof( ShipStats, n ) }
/* Standard types. */
#define D__ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE, 0 )
#define A__ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE_ABSOLUTE, 0 )
#define P__ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT, 0 )
#define I__ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_INTEGER, 0 )
#define B__ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_BOOLEAN, 0 )
/* Inverted types. */
#define DI_ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE, 1 )
#define AI_ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE_ABSOLUTE, 1 )
#define PI_ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT, 1 )
#define II_ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_INTEGER, 1 )
#define BI_ELEM( t, n, r, dsp ) \
   ELEM( t, n, r, dsp, SS_DATA_TYPE_BOOLEAN, 1 )
/** Nil element. */
#define N__ELEM( t ) \
   { .type=t, .ship_redundant=0, .name=NULL, .display=NULL, .inverted=0, .offset=0 }

/**
 * The ultimate look up table for ship stats, everything goes through this.
 */
static const ShipStatsLookup ss_lookup[] = {
   /* Null element. */
   N__ELEM(SS_TYPE_NIL),

   D__ELEM(SS_TYPE_D_SPEED_MOD, speed_mod, 1,
      N_("%+G%% Speed")),
   D__ELEM(SS_TYPE_D_TURN_MOD, turn_mod, 1,
      N_("%+G%% Turn")),
   D__ELEM(SS_TYPE_D_THRUST_MOD, thrust_mod, 1,
      N_("%+G%% Thrust")),
   D__ELEM(SS_TYPE_D_CARGO_MOD, cargo_mod, 1,
      N_("%+G%% Cargo Space")),
   D__ELEM(SS_TYPE_D_ARMOUR_MOD, armour_mod, 1,
      N_("%+G%% Armor Strength")),
   D__ELEM(SS_TYPE_D_ARMOUR_REGEN_MOD, armour_regen_mod, 1,
      N_("%+G%% Armor Regeneration")),
   D__ELEM(SS_TYPE_D_SHIELD_MOD, shield_mod, 1,
      N_("%+G%% Shield Strength")),
   D__ELEM(SS_TYPE_D_SHIELD_REGEN_MOD, shield_regen_mod, 1,
      N_("%+G%% Shield Regeneration")),
   D__ELEM(SS_TYPE_D_ENERGY_MOD, energy_mod, 1,
      N_("%+G%% Energy Capacity")),
   D__ELEM(SS_TYPE_D_ENERGY_REGEN_MOD, energy_regen_mod, 1,
      N_("%+G%% Energy Regeneration")),
   D__ELEM(SS_TYPE_D_CPU_MOD, cpu_mod, 1, N_("%+G%% CPU Capacity")),

   DI_ELEM(SS_TYPE_D_JUMP_DELAY, jump_delay, 1,
      N_("%+G%% Jump Time")),
   DI_ELEM(SS_TYPE_D_LAND_DELAY, land_delay, 1,
      N_("%+G%% Takeoff Time")),
   DI_ELEM(SS_TYPE_D_CARGO_INERTIA, cargo_inertia, 0,
      N_("%+G%% Cargo Inertia")),

   A__ELEM(SS_TYPE_D_RDR_RANGE, rdr_range, 1,
      N_("%+G km Radar Range")),
   A__ELEM(SS_TYPE_D_RDR_JUMP_RANGE, rdr_jump_range, 1,
      N_("%+G km Jump Detect Range")),
   D__ELEM(SS_TYPE_D_RDR_RANGE_MOD, rdr_range_mod, 0,
      N_("%+G%% Radar Range")),
   D__ELEM(SS_TYPE_D_RDR_JUMP_RANGE_MOD, rdr_jump_range_mod, 1,
      N_("%+G%% Jump Detect Range")),
   DI_ELEM(SS_TYPE_D_RDR_ENEMY_RANGE_MOD, rdr_enemy_range_mod, 0,
      N_("%+G%% Enemy Radar Range")),

   D__ELEM(SS_TYPE_D_LAUNCH_RATE, launch_rate, 0,
      N_("%+G%% Fire Rate (Launcher)")),
   D__ELEM(SS_TYPE_D_LAUNCH_RANGE, launch_range, 0,
      N_("%+G%% Range (Launcher)")),
   D__ELEM(SS_TYPE_D_LAUNCH_DAMAGE, launch_damage, 0,
      N_("%+G%% Damage (Launcher)")),
   D__ELEM(SS_TYPE_D_AMMO_CAPACITY, ammo_capacity, 0,
      N_("%+G%% Ammo Capacity")),
   D__ELEM(SS_TYPE_D_LAUNCH_RELOAD, launch_reload, 0,
      N_("%+G%% Ammo Reload Rate")),

   D__ELEM(SS_TYPE_D_FBAY_DAMAGE, fbay_damage, 0,
      N_("%+G%% Fighter Damage")),
   D__ELEM(SS_TYPE_D_FBAY_HEALTH, fbay_health, 0,
      N_("%+G%% Fighter Health")),
   D__ELEM(SS_TYPE_D_FBAY_MOVEMENT, fbay_movement, 0,
      N_("%+G%% Fighter Agility")),
   D__ELEM(SS_TYPE_D_FBAY_CAPACITY, fbay_capacity, 0,
      N_("%+G%% Fighter Bay Capacity")),
   D__ELEM(SS_TYPE_D_FBAY_RATE, fbay_rate, 0,
      N_("%+G%% Fighter Bay Launch Rate")),
   D__ELEM(SS_TYPE_D_FBAY_RELOAD, fbay_reload, 0,
      N_("%+G%% Fighter Reload Rate")),

   DI_ELEM(SS_TYPE_D_FORWARD_HEAT, fwd_heat, 0,
      N_("%+G%% Heat (Forward)")),
   D__ELEM(SS_TYPE_D_FORWARD_DAMAGE, fwd_damage, 0,
      N_("%+G%% Damage (Forward)")),
   D__ELEM(SS_TYPE_D_FORWARD_FIRERATE, fwd_firerate, 0,
      N_("%+G%% Fire Rate (Forward)")),
   DI_ELEM(SS_TYPE_D_FORWARD_ENERGY, fwd_energy, 0,
      N_("%+G%% Energy Usage (Forward)")),
   D__ELEM(SS_TYPE_D_FORWARD_DAMAGE_AS_DISABLE, fwd_dam_as_dis, 0,
      N_("%+G%% Damage as Disable (Forward)")),

   DI_ELEM(SS_TYPE_D_TURRET_HEAT, tur_heat, 0,
      N_("%+G%% Heat (Turret)")),
   D__ELEM(SS_TYPE_D_TURRET_DAMAGE, tur_damage, 0,
      N_("%+G%% Damage (Turret)")),
   D__ELEM(SS_TYPE_D_TURRET_FIRERATE, tur_firerate, 0,
      N_("%+G%% Fire Rate (Turret)")),
   DI_ELEM(SS_TYPE_D_TURRET_ENERGY, tur_energy, 0,
      N_("%+G%% Energy Usage (Turret)")),
   D__ELEM(SS_TYPE_D_TURRET_DAMAGE_AS_DISABLE, tur_dam_as_dis, 0,
      N_("%+G%% Damage as Disable (Turret)")),

   D__ELEM(SS_TYPE_D_HEAT_DISSIPATION, heat_dissipation, 0,
      N_("%+G%% Heat Dissipation")),
   D__ELEM(SS_TYPE_D_STRESS_DISSIPATION, stress_dissipation, 0,
      N_("%+G%% Stress Dissipation")),
   DI_ELEM(SS_TYPE_D_MASS, mass_mod, 1,
      N_("%+G%% Ship Mass")),
   D__ELEM(SS_TYPE_D_ENGINE_LIMIT_REL, engine_limit_rel, 1,
      N_("%+G%% Engine Mass Limit")),
   D__ELEM(SS_TYPE_D_LOOT_MOD, loot_mod, 0,
      N_("%+G%% Boarding Bonus")),
   DI_ELEM(SS_TYPE_D_TIME_MOD, time_mod, 1,
      N_("%+G%% Time Constant")),
   D__ELEM(SS_TYPE_D_TIME_SPEEDUP, time_speedup, 0,
      N_("%+G%% Speed-Up")),
   DI_ELEM(SS_TYPE_D_COOLDOWN_TIME, cooldown_time, 0,
      N_("%+G%% Ship Cooldown Time")),
   D__ELEM(SS_TYPE_D_JUMP_DISTANCE, jump_distance, 0,
      N_("%+G%% Jump Point Radius")),

   A__ELEM(SS_TYPE_A_THRUST, thrust, 1,
      N_("%+G MN/t Thrust")),
   A__ELEM(SS_TYPE_A_TURN, turn, 1,
      N_("%+G deg/s Turn Rate")),
   A__ELEM(SS_TYPE_A_SPEED, speed, 1,
      N_("%+G km/s Maximum Speed")),
   A__ELEM(SS_TYPE_A_ENERGY, energy, 1,
      N_("%+G GJ Energy Capacity")),
   A__ELEM(SS_TYPE_A_ENERGY_REGEN, energy_regen, 1,
      N_("%+G GW Energy Regeneration")),
   AI_ELEM(SS_TYPE_A_ENERGY_REGEN_MALUS, energy_regen_malus, 1,
      N_("%+G GW Energy Usage")),
   AI_ELEM(SS_TYPE_A_ENERGY_LOSS, energy_loss, 0,
      N_("%+G GW Energy Loss")),
   A__ELEM(SS_TYPE_A_SHIELD, shield, 1,
      N_("%+G GJ Shield Capacity")),
   A__ELEM(SS_TYPE_A_SHIELD_REGEN, shield_regen, 1,
      N_("%+G GW Shield Regeneration")),
   AI_ELEM(SS_TYPE_A_SHIELD_REGEN_MALUS, shield_regen_malus, 1,
      N_("%+G GW Shield Usage")),
   A__ELEM(SS_TYPE_A_ARMOUR, armour, 1,
      N_("%+G GJ Armor Capacity")),
   A__ELEM(SS_TYPE_A_ARMOUR_REGEN, armour_regen, 1,
      N_("%+G GW Armor Regeneration")),
   AI_ELEM(SS_TYPE_A_ARMOUR_REGEN_MALUS, armour_regen_malus, 1,
      N_("%+G GW Armor Usage")),

   A__ELEM(SS_TYPE_A_CPU_MAX, cpu_max, 1,
      N_("%+G TFLOPS CPU Capacity")),
   A__ELEM(SS_TYPE_A_ENGINE_LIMIT, engine_limit, 1,
      N_("%+G t Engine Mass Limit")),

   P__ELEM(SS_TYPE_P_ABSORB, absorb, 1,
      N_("%+G pp Damage Absorption")),

   P__ELEM(SS_TYPE_P_NEBULA_ABSORB_SHIELD, nebu_absorb_shield, 0,
      N_("%+G pp Nebula Resistance (Shield)")),
   P__ELEM(SS_TYPE_P_NEBULA_ABSORB_ARMOUR, nebu_absorb_armour, 0,
      N_("%+G pp Nebula Resistance (Armor)")),

   I__ELEM(SS_TYPE_I_FUEL, fuel, 1,
      N_("%+d hL Fuel")),
   I__ELEM(SS_TYPE_I_CARGO, cargo, 1,
      N_("%+d t Cargo Space")),

   B__ELEM(SS_TYPE_B_INSTANT_JUMP, misc_instant_jump, 0,
      N_("Instant Jump")),
   B__ELEM(SS_TYPE_B_REVERSE_THRUST, misc_reverse_thrust, 0,
      N_("Reverse Thrusters")),
   B__ELEM(SS_TYPE_B_ASTEROID_SCAN, misc_asteroid_scan, 0,
      N_("Asteroid Scanner")),

   /* Sentinel. */
   N__ELEM(SS_TYPE_SENTINEL)
};


/*
 * Prototypes.
 */
static const char* ss_printD_colour( double d, const ShipStatsLookup *sl );
static const char* ss_printI_colour( int i, const ShipStatsLookup *sl );
static int ss_printD( char *buf, int len, int newline, double d, const ShipStatsLookup *sl );
static int ss_printA( char *buf, int len, int newline, double d, const ShipStatsLookup *sl );
static int ss_printI( char *buf, int len, int newline, int i, const ShipStatsLookup *sl );
static int ss_printB( char *buf, int len, int newline, int b, const ShipStatsLookup *sl );
static double ss_statsGetInternal( const ShipStats *s, ShipStatsType type );
static int ss_statsGetLuaInternal( lua_State *L, const ShipStats *s, ShipStatsType type, int internal );


/**
 * @brief Creatse a shipstat list element from an xml node.
 *
 *    @param node Node to create element from.
 *    @return Liste lement created from node.
 */
ShipStatList* ss_listFromXML( xmlNodePtr node )
{
   const ShipStatsLookup *sl;
   ShipStatList *ll;
   ShipStatsType type;

   /* Try to get type. */
   type = ss_typeFromName( (char*) node->name );
   if (type == SS_TYPE_NIL)
      return NULL;

   /* Allocate. */
   ll = malloc( sizeof(ShipStatList) );
   ll->next    = NULL;
   ll->target  = 0;
   ll->type    = type;

   /* Set the data. */
   sl = &ss_lookup[ type ];
   switch (sl->data) {
      case SS_DATA_TYPE_DOUBLE:
      case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         ll->d.d     = xml_getFloat(node) / 100.;
         break;

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
         ll->d.d     = xml_getFloat(node);
         break;

      case SS_DATA_TYPE_BOOLEAN:
         ll->d.i     = !!xml_getInt(node);
         break;

      case SS_DATA_TYPE_INTEGER:
         ll->d.i     = xml_getInt(node);
         break;
   }

   return ll;
}


/**
 * @brief Checks for validity.
 */
int ss_check (void)
{
   ShipStatsType i;

   for (i=0; i<=SS_TYPE_SENTINEL; i++) {
      if (ss_lookup[i].type != i) {
         WARN(_("ss_lookup: %s should have id %d but has %d"),
               ss_lookup[i].name, i, ss_lookup[i].type );
         return -1;
      }
   }

   return 0;
}


/**
 * @brief Initializes a stat structure.
 */
int ss_statsInit( ShipStats *stats )
{
   int i;
   char *ptr;
   char *fieldptr;
   double *dbl;
   const ShipStatsLookup *sl;

   /* Clear the memory. */
   memset( stats, 0, sizeof(ShipStats) );

   ptr = (char*) stats;
   for (i=0; i<SS_TYPE_SENTINEL; i++) {
      sl = &ss_lookup[ i ];

      /* Only want valid names. */
      if (sl->name == NULL)
         continue;

      /* Handle doubles. */
      switch (sl->data) {
         case SS_DATA_TYPE_DOUBLE:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&dbl, &fieldptr, sizeof(double*));
            *dbl  = 1.0;
            break;

         /* No need to set, memset does the work. */
         case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
         case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         case SS_DATA_TYPE_INTEGER:
         case SS_DATA_TYPE_BOOLEAN:
            break;
      }
   }

   return 0;
}


/**
 * @brief Merges two different ship stats.
 *
 *    @param dest Destination ship stats.
 *    @param src Source to be merged with destination.
 */
int ss_statsMerge( ShipStats *dest, const ShipStats *src )
{
   int i;
   int *destint;
   const int *srcint;
   double *destdbl;
   const double *srcdbl;
   char *destptr;
   const char *srcptr;
   const ShipStatsLookup *sl;

   destptr = (char*) dest;
   srcptr = (const char*) src;
   for (i=0; i<SS_TYPE_SENTINEL; i++) {
      sl = &ss_lookup[ i ];

      /* Only want valid names. */
      if (sl->name == NULL)
         continue;

      switch (sl->data) {
         case SS_DATA_TYPE_DOUBLE:
            destdbl = (double*) &destptr[ sl->offset ];
            srcdbl = (const double*) &srcptr[ sl->offset ];
            *destdbl = (*destdbl) * (*srcdbl);
            break;

         case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
         case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
            destdbl = (double*) &destptr[ sl->offset ];
            srcdbl = (const double*) &srcptr[ sl->offset ];
            *destdbl = (*destdbl) + (*srcdbl);
            break;

         case SS_DATA_TYPE_INTEGER:
            destint = (int*) &destptr[ sl->offset ];
            srcint = (const int*) &srcptr[ sl->offset ];
            *destint = (*destint) + (*srcint);
            break;

         case SS_DATA_TYPE_BOOLEAN:
            destint = (int*) &destptr[ sl->offset ];
            srcint = (const int*) &srcptr[ sl->offset ];
            *destint = !!((*destint) + (*srcint));
            break;
      }
   }

   return 0;
}


/**
 * @brief Modifies a stat structure using a single element.
 *
 *    @param stats Stat structure to modify.
 *    @param list Single element to apply.
 *    @return 0 on success.
 */
int ss_statsModSingle( ShipStats *stats, const ShipStatList* list )
{
   char *ptr;
   char *fieldptr;
   double *dbl;
   int *i;
   const ShipStatsLookup *sl = &ss_lookup[ list->type ];

   ptr = (char*) stats;
   switch (sl->data) {
      case SS_DATA_TYPE_DOUBLE:
         fieldptr = &ptr[ sl->offset ];
         memcpy(&dbl, &fieldptr, sizeof(double*));
         *dbl *= 1.0+list->d.d;
         if (*dbl < 0.) /* Don't let the values go negative. */
            *dbl = 0.;
         break;

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
      case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         fieldptr = &ptr[ sl->offset ];
         memcpy(&dbl, &fieldptr, sizeof(double*));
         *dbl += list->d.d;
         break;

      case SS_DATA_TYPE_INTEGER:
         fieldptr = &ptr[ sl->offset ];
         memcpy(&i, &fieldptr, sizeof(int*));
         *i   += list->d.i;
         break;

      case SS_DATA_TYPE_BOOLEAN:
         fieldptr = &ptr[ sl->offset ];
         memcpy(&i, &fieldptr, sizeof(int*));
         *i    = 1; /* Can only set to true. */
         break;
   }

   return 0;
}


/**
 * @brief Updates a stat structure from a stat list.
 *
 *    @param stats Stats to update.
 *    @param list List to update from.
 */
int ss_statsModFromList( ShipStats *stats, const ShipStatList* list )
{
   int ret;
   const ShipStatList *ll;

   ret = 0;
   for (ll = list; ll != NULL; ll = ll->next)
      ret |= ss_statsModSingle( stats, ll );

   return ret;
}


/**
 * @brief Gets the name from type.
 *
 * O(1) look up.
 *
 *    @param type Type to get name of.
 *    @return Name of the type.
 */
const char* ss_nameFromType( ShipStatsType type )
{
   return ss_lookup[ type ].name;
}


/**
 * @brief Gets the offset from type.
 *
 *    @param type Type to get offset of.
 *    @return Offset of the type.
 */
size_t ss_offsetFromType( ShipStatsType type )
{
   return ss_lookup[ type ].offset;
}


/**
 * @brief Gets the type from the name.
 *
 *    @param name Name to get type of.
 *    @return Type matching the name.
 */
ShipStatsType ss_typeFromName( const char *name )
{
   int i;
   for (i=0; i<SS_TYPE_SENTINEL; i++)
      if ((ss_lookup[i].name != NULL) && (strcmp(name,ss_lookup[i].name)==0))
         return ss_lookup[i].type;

   WARN(_("ss_typeFromName: No ship stat matching '%s'"), name);
   return SS_TYPE_NIL;
}


/**
 * @brief Some colour coding for ship stats doubles.
 */
static const char* ss_printD_colour( double d, const ShipStatsLookup *sl )
{
   if (sl->inverted) {
      if (d < 0.)
         return "g";
      return "r";
   }

   if (d > 0.)
      return "g";
   return "r";
}
/**
 * @brief Some colour coding for ship stats integers.
 */
static const char* ss_printI_colour( int i, const ShipStatsLookup *sl )
{
   if (sl->inverted) {
      if (i < 0)
         return "g";
      return "r";
   }

   if (i > 0)
      return "g";
   return "r";
}
/**
 * @brief Some colour coding for ship stats doubles.
 */
static const char* ss_printD_symbol( double d, const ShipStatsLookup *sl )
{
   if (sl->inverted) {
      if (d < 0.)
         return "";
      return "!! ";
   }

   if (d > 0.)
      return "";
   return "!! ";
}
/**
 * @brief Some colour coding for ship stats integers.
 */
static const char* ss_printI_symbol( int i, const ShipStatsLookup *sl )
{
   if (sl->inverted) {
      if (i < 0)
         return "";
      return "!! ";
   }

   if (i > 0)
      return "";
   return "!! ";
}


/**
 * @brief Helper to print doubles.
 */
static int ss_printD( char *buf, int len, int newline, double d, const ShipStatsLookup *sl )
{
   char buf2[STRMAX_SHORT];

   if (FABS(d) < 1e-10)
      return 0;

   snprintf( buf2, sizeof(buf2), _(sl->display), d*100 );

   return scnprintf( buf, len, "%s#%s%s%s#0",
         (newline) ? "\n" : "",
         ss_printD_colour( d, sl ),
         ss_printD_symbol( d, sl ),
         buf2 );
}


/**
 * @brief Helper to print absolute doubles.
 */
static int ss_printA( char *buf, int len, int newline, double d, const ShipStatsLookup *sl )
{
   char buf2[STRMAX_SHORT];

   if (FABS(d) < 1e-10)
      return 0;

   snprintf( buf2, sizeof(buf2), _(sl->display), d );

   return scnprintf( buf, len, "%s#%s%s%s#0",
         (newline) ? "\n" : "",
         ss_printD_colour( d, sl ),
         ss_printD_symbol( d, sl ),
         buf2 );
}


/**
 * @brief Helper to print integers.
 */
static int ss_printI( char *buf, int len, int newline, int i, const ShipStatsLookup *sl )
{
   char buf2[STRMAX_SHORT];

   if (i == 0)
      return 0;

   snprintf( buf2, sizeof(buf2), _(sl->display), i );

   return scnprintf( buf, len, "%s#%s%s%s#0",
         (newline) ? "\n" : "",
         ss_printI_colour( i, sl ),
         ss_printI_symbol( i, sl ),
         buf2 );
}


/**
 * @brief Helper to print booleans.
 */
static int ss_printB( char *buf, int len, int newline, int b, const ShipStatsLookup *sl )
{
   if (!b)
      return 0;
   return scnprintf( buf, len, "%s#%s%s%s#0",
         (newline) ? "\n" : "",
         ss_printI_colour( b, sl ),
         ss_printI_symbol( b, sl ),
         _(sl->display) );
}




/**
 * @brief Writes the ship statistics description.
 *
 *    @param ll Ship stats to use.
 *    @param buf Buffer to write to.
 *    @param len Space left in the buffer.
 *    @param newline Add a newline at start.
 *    @return Number of characters written.
 */
int ss_statsListDesc( const ShipStatList *ll, char *buf, int len, int newline )
{
   int i, left, newl;
   const ShipStatsLookup *sl;
   i     = 0;
   newl  = newline;
   for ( ; ll != NULL; ll=ll->next) {
      left  = len-i;
      if (left < 0)
         break;
      sl    = &ss_lookup[ ll->type ];

      switch (sl->data) {
         case SS_DATA_TYPE_DOUBLE:
         case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
            i += ss_printD( &buf[i], left, newl, ll->d.d, sl );
            break;

         case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
            i += ss_printA( &buf[i], left, newl, ll->d.d, sl );
            break;

         case SS_DATA_TYPE_INTEGER:
            i += ss_printI( &buf[i], left, newl, ll->d.i, sl );
            break;

         case SS_DATA_TYPE_BOOLEAN:
            i += ss_printB( &buf[i], left, newl, ll->d.i, sl );
            break;
      }

      newl = 1;
   }
   return i;
}


/**
 * @brief Writes the ship statistics description.
 *
 *    @param s Ship stats to use.
 *    @param buf Buffer to write to.
 *    @param len Space left in the buffer.
 *    @param newline Add a newline at start.
 *    @param include_redundant Include "ship redundant" stats.
 *    @return Number of characters written.
 */
int ss_statsDesc( const ShipStats *s, char *buf, int len, int newline, int include_redundant )
{
   int i, l, left;
   char *ptr;
   char *fieldptr;
   double *dbl;
   int *num;
   const ShipStatsLookup *sl;

   l   = 0;
   ptr = (char*) s;
   for (i=0; i<SS_TYPE_SENTINEL; i++) {
      sl = &ss_lookup[ i ];

      /* Only want valid names. */
      if (sl->name == NULL)
         continue;

      /* Only include redundant stats if requested. */
      if ((sl->ship_redundant) && (!include_redundant))
         continue;

      /* Calculate offset left. */
      left = len-l;
      if (left < 0)
         break;

      switch (sl->data) {
         case SS_DATA_TYPE_DOUBLE:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&dbl, &fieldptr, sizeof(double*));
            l    += ss_printD( &buf[l], left, (newline||(l!=0)), ((*dbl)-1.), sl );
            break;

         case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&dbl, &fieldptr, sizeof(double*));
            l    += ss_printD( &buf[l], left, (newline||(l!=0)), (*dbl), sl );
            break;

         case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&dbl, &fieldptr, sizeof(double*));
            l    += ss_printA( &buf[l], left, (newline||(l!=0)), (*dbl), sl );
            break;

         case SS_DATA_TYPE_INTEGER:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&num, &fieldptr, sizeof(int*));
            l    += ss_printI( &buf[l], left, (newline||(l!=0)), (*num), sl );
            break;

         case SS_DATA_TYPE_BOOLEAN:
            fieldptr = &ptr[ sl->offset ];
            memcpy(&num, &fieldptr, sizeof(int*));
            l    += ss_printB( &buf[l], left, (newline||(l!=0)), (*num), sl );
            break;
      }
   }

   return l;
}


/**
 * @brief Frees a list of ship stats.
 *
 *    @param ll List to free.
 */
void ss_free( ShipStatList *ll )
{
   while (ll != NULL) {
      ShipStatList *tmp = ll;
      ll = ll->next;
      free(tmp);
   }
}



/**
 * @brief Sets a ship stat by name.
 */
int ss_statsSet( ShipStats *s, const char *name, double value, int overwrite )
{
   const ShipStatsLookup *sl;
   ShipStatsType type;
   char *ptr;
   double *destdbl;
   int *destint;
   double v;

   type = ss_typeFromName( name );
   if (type == SS_TYPE_NIL) {
      WARN(_("Unknown ship stat type '%s'!"), name );
      return -1;
   }

   sl = &ss_lookup[ type ];
   ptr = (char*) s;
   switch (sl->data) {
      case SS_DATA_TYPE_DOUBLE:
         destdbl = (double*) &ptr[ sl->offset ];
         v = 1.0 + value / 100.;
         if (overwrite)
            *destdbl = v;
         else
            *destdbl *= v;
         break;

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
      case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         destdbl  = (double*) &ptr[ sl->offset ];
         if (overwrite)
            *destdbl = value;
         else
            *destdbl += value;
         break;

      case SS_DATA_TYPE_BOOLEAN:
         destint  = (int*) &ptr[ sl->offset ];
         if (overwrite)
            *destint = !(fabs(value) > 1e-5);
         else
            *destint |= !(fabs(value) > 1e-5);
         break;

      case SS_DATA_TYPE_INTEGER:
         destint  = (int*) &ptr[ sl->offset ];
         if (overwrite)
            *destint = round(value);
         else
            *destint += round(value);
         break;
   }

   return 0;
}


static double ss_statsGetInternal( const ShipStats *s, ShipStatsType type )
{
   const ShipStatsLookup *sl;
   const char *ptr;
   const double *destdbl;
   const int *destint;

   sl = &ss_lookup[ type ];
   ptr = (const char*) s;
   switch (sl->data) {
      case SS_DATA_TYPE_DOUBLE:
         destdbl = (const double*) &ptr[ sl->offset ];
         return 100.*((*destdbl) - 1.0);

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         destdbl = (const double*) &ptr[ sl->offset ];
         return 100.*(*destdbl);

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
         destdbl  = (const double*) &ptr[ sl->offset ];
         return *destdbl;

      case SS_DATA_TYPE_BOOLEAN:
      case SS_DATA_TYPE_INTEGER:
         destint  = (const int*) &ptr[ sl->offset ];
         return *destint;
   }
   return 0.;
}


static int ss_statsGetLuaInternal( lua_State *L, const ShipStats *s, ShipStatsType type, int internal )
{
   const ShipStatsLookup *sl;
   const char *ptr;
   const double *destdbl;
   const int *destint;

   sl = &ss_lookup[ type ];
   ptr = (const char*) s;
   switch (sl->data) {
      case SS_DATA_TYPE_DOUBLE:
         destdbl = (const double*) &ptr[ sl->offset ];
         if (internal)
            lua_pushnumber(L, *destdbl );
         else
            lua_pushnumber(L, 100.*((*destdbl) - 1.0) );
         return 0;

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE_PERCENT:
         destdbl = (const double*) &ptr[ sl->offset ];
         if (internal)
            lua_pushnumber(L, *destdbl );
         else
            lua_pushnumber(L, 100.*(*destdbl));
         return 0;

      case SS_DATA_TYPE_DOUBLE_ABSOLUTE:
         destdbl  = (const double*) &ptr[ sl->offset ];
         lua_pushnumber(L, *destdbl);
         return 0;

      case SS_DATA_TYPE_BOOLEAN:
         destint  = (const int*) &ptr[ sl->offset ];
         lua_pushboolean(L, *destint);
         return 0;

      case SS_DATA_TYPE_INTEGER:
         destint  = (const int*) &ptr[ sl->offset ];
         lua_pushinteger(L, *destint);
         return 0;
   }
   lua_pushnil(L);
   return -1;
}



/**
 * @brief Gets a ship stat value by name.
 */
double ss_statsGet( const ShipStats *s, const char *name )
{
   ShipStatsType type;

   type = ss_typeFromName( name );
   if (type == SS_TYPE_NIL) {
      WARN(_("Unknown ship stat type '%s'!"), name );
      return 0;
   }

   return ss_statsGetInternal( s, type );
}

/**
 * @brief Gets a ship stat value by name and pushes it to Lua.
 */
int ss_statsGetLua( lua_State *L, const ShipStats *s, const char *name, int internal )
{
   ShipStatsType type;

   if (name==NULL)
      return ss_statsGetLuaTable( L, s, internal );

   type = ss_typeFromName( name );
   if (type == SS_TYPE_NIL) {
      WARN(_("Unknown ship stat type '%s'!"), name );
      return -1;
   }

   return ss_statsGetLuaInternal( L, s, type, internal );
}


/**
 * @brief Converts ship stats to a Lua table, which is pushed on the Lua stack.
 */
int ss_statsGetLuaTable( lua_State *L, const ShipStats *s, int internal )
{
   int i;
   const ShipStatsLookup *sl;

   lua_newtable(L);

   for (i=0; i<SS_TYPE_SENTINEL; i++) {
      sl = &ss_lookup[ i ];

      /* Only want valid names. */
      if (sl->name == NULL)
         continue;

      /* Push name and get value. */
      lua_pushstring(L, sl->name);
      ss_statsGetLuaInternal( L, s, i, internal );
      lua_rawset( L, -3 );
   }

   return 0;
}
