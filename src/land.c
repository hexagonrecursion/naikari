/*
 * See Licensing and Copyright notice in naev.h
 */



#include "land.h"

#include "log.h"
#include "toolkit.h"
#include "player.h"
#include "rng.h"
#include "music.h"
#include "economy.h"


/* global/main window */
#define LAND_WIDTH	700
#define LAND_HEIGHT	600
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 40

/* commodity window */
#define COMMODITY_WIDTH 400
#define COMMODITY_HEIGHT 400

/* outfits */
#define OUTFITS_WIDTH	700
#define OUTFITS_HEIGHT	500

/* shipyard */
#define SHIPYARD_WIDTH	700
#define SHIPYARD_HEIGHT	500

/* news window */
#define NEWS_WIDTH	400
#define NEWS_HEIGHT	400

/* bar window */
#define BAR_WIDTH		600
#define BAR_HEIGHT	400


#define MUSIC_TAKEOFF	"liftoff"
#define MUSIC_LAND		"agriculture"


int landed = 0;

static int land_wid = 0; /* used for the primary land window */
static int secondary_wid = 0; /* used for the second opened land window (can only have 2) */
static Planet* planet = NULL;


/*
 * extern
 */
extern unsigned int player_credits;


/*
 * prototypes
 */
/* commodity exchange */
static void commodity_exchange (void);
static void commodity_exchange_close( char* str );
/* outfits */
static void outfits (void);
static void outfits_close( char* str );
static void outfits_update( char* str );
static void outfits_buy( char* str );
static void outfits_sell( char* str );
/* shipyard */
static void shipyard (void);
static void shipyard_close( char* str );
static void shipyard_update( char* str );
static void shipyard_info( char* str );
static void shipyard_buy( char* str );
/* spaceport bar */
static void spaceport_bar (void);
static void spaceport_bar_close( char* str );
/* news */
static void news (void);
static void news_close( char* str );


/*
 * the local market
 */
static void commodity_exchange (void)
{
	char** goods;
	int ngoods;

	goods = malloc(sizeof(char*)*3);
	goods[0] = strdup("hello");
	goods[1] = strdup("just");
	goods[2] = strdup("testing");
	ngoods = 3;

	secondary_wid = window_create( "Commodity Exchange",
			-1, -1, COMMODITY_WIDTH, COMMODITY_HEIGHT );

	window_addButton( secondary_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnCommodityClose",
			"Close", commodity_exchange_close );

	window_addList( secondary_wid, 20, -40,
			COMMODITY_WIDTH-40, COMMODITY_HEIGHT-80-BUTTON_HEIGHT,
			"lstGoods", goods, ngoods, 0, NULL );
}
static void commodity_exchange_close( char* str )
{
	if (strcmp(str, "btnCommodityClose")==0)
		window_destroy(secondary_wid);
}


/*
 * ze outfits
 */
static void outfits (void)
{
	char **outfits;
	int noutfits;

	secondary_wid = window_create( "Outfits", -1, -1,
			OUTFITS_WIDTH, OUTFITS_HEIGHT );

	window_addButton( secondary_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnCloseOutfits",
			"Close", outfits_close );

	window_addButton( secondary_wid, -40-BUTTON_WIDTH, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnBuyOutfit",
			"Buy", outfits_buy );

	window_addButton( secondary_wid, -40-BUTTON_WIDTH, 40+BUTTON_HEIGHT,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnSellOutfit",
			"Sell", outfits_sell );

	window_addText( secondary_wid, 40+200+20, -60,
			80, 96, 0, "txtSDesc", &gl_smallFont, &cDConsole,
			"Name:\n"
			"Type:\n"
			"Owned:\n"
			"\n"
			"Space taken:\n"
			"Free Space:\n"
			"\n"
			"Price:\n"
			"Money:\n" );
	window_addText( secondary_wid, 40+200+40+60, -60,
			250, 96, 0, "txtDDesc", &gl_smallFont, &cBlack, NULL );

	window_addText( secondary_wid, 20+200+40, -200,
			OUTFITS_WIDTH-360, 200, 0, "txtDescription",
			&gl_smallFont, NULL, NULL );


	/* set up the outfits to buy/sell */
	outfits = outfit_getAll( &noutfits );
	window_addList( secondary_wid, 20, 40,
			200, OUTFITS_HEIGHT-80, "lstOutfits",
			outfits, noutfits, 0, outfits_update );

	/* write the outfits stuff */
	outfits_update( NULL );
}
static void outfits_close( char* str )
{
	if (strcmp(str,"btnCloseOutfits")==0)
		window_destroy(secondary_wid);
}
static void outfits_update( char* str )
{
	(void)str;
	char *outfitname;
	Outfit* outfit;
	char buf[80], buf2[16], buf3[16];

	outfitname = toolkit_getList( secondary_wid, "lstOutfits" );
	outfit = outfit_get( outfitname );

	window_modifyText( secondary_wid, "txtDescription", outfit->description );
	credits2str( buf2, outfit->price, 2 );
	credits2str( buf3, player_credits, 2 );
	snprintf( buf, 80,
			"%s\n"
			"%s\n"
			"%d\n"
			"\n"
			"%d\n"
			"%d\n"
			"\n"
			"%s credits\n"
			"%s credits\n",
			outfit->name,
			outfit_getType(outfit),
			player_outfitOwned(outfitname),
			outfit->mass,
			player_freeSpace(),
			buf2,
			buf3 );
	window_modifyText( secondary_wid,  "txtDDesc", buf );
}
static void outfits_buy( char* str )
{
	(void)str;
	char *outfitname;
	Outfit* outfit;
	int q;

	outfitname = toolkit_getList( secondary_wid, "lstOutfits" );
	outfit = outfit_get( outfitname );

	q = 1; /* TODO make q dependent on MOD keys */

	pilot_addOutfit( player, outfit, q );
	outfits_update(NULL);
}
static void outfits_sell( char* str )
{
	(void)str;
	char *outfitname;
	Outfit* outfit;
	int q;

	outfitname = toolkit_getList( secondary_wid, "lstOutfits" );
	outfit = outfit_get( outfitname );
	q = 1;

	pilot_rmOutfit( player, outfit, q );
	outfits_update(NULL);
}




/*
 * ze shipyard
 */
static void shipyard (void)
{
	char **ships;
	int nships;

	secondary_wid = window_create( "Shipyard",
			-1, -1, SHIPYARD_WIDTH, SHIPYARD_HEIGHT );

	window_addButton( secondary_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnCloseShipyard",
			"Close", shipyard_close );

	window_addButton( secondary_wid, -40-BUTTON_WIDTH, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnBuyShip",
			"Buy", shipyard_buy );

	window_addButton( secondary_wid, -40-BUTTON_WIDTH, 40+BUTTON_HEIGHT,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnInfoShip",
			"Info", shipyard_info );

	window_addRect( secondary_wid, -40, -50,
			128, 96, "rctTarget", &cBlack, 0 );
	window_addImage( secondary_wid, -40-128, -50-96,
			"imgTarget", NULL );

	window_addText( secondary_wid, 40+200+40, -55,
			80, 96, 0, "txtSDesc", &gl_smallFont, &cDConsole,
			"Name:\n"
			"Class:\n"
			"Fabricator:\n"
			"\n"
			"Price:\n"
			"Money:\n" );
	window_addText( secondary_wid, 40+200+40+80, -55,
			130, 96, 0, "txtDDesc", &gl_smallFont, &cBlack, NULL );

	window_addText( secondary_wid, 20+200+40, -160,
			SHIPYARD_WIDTH-360, 200, 0, "txtDescription",
			&gl_smallFont, NULL, NULL );


	/* set up the ships to buy/sell */
	ships = ship_getAll( &nships );
	window_addList( secondary_wid, 20, 40,
			200, SHIPYARD_HEIGHT-80, "lstShipyard",
			ships, nships, 0, shipyard_update );

	/* write the shipyard stuff */
	shipyard_update( NULL );
}
static void shipyard_close( char* str )
{
	if (strcmp(str,"btnCloseShipyard")==0)
		window_destroy(secondary_wid);
}
static void shipyard_update( char* str )
{
	(void)str;
	char *shipname;
	Ship* ship;
	char buf[80], buf2[16], buf3[16];
	
	shipname = toolkit_getList( secondary_wid, "lstShipyard" );
	ship = ship_get( shipname );

	window_modifyText( secondary_wid, "txtDescription", ship->description );
	window_modifyImage( secondary_wid, "imgTarget", ship->gfx_target );
	credits2str( buf2, ship->price, 2 );
	credits2str( buf3, player_credits, 2 );
	snprintf( buf, 80,
			"%s\n"
			"%s\n"
			"%s\n"
			"\n"
			"%s credits\n"
			"%s credits\n",
			ship->name,
			ship_class(ship),
			ship->fabricator,
			buf2,
			buf3);
	window_modifyText( secondary_wid,  "txtDDesc", buf );
}
static void shipyard_info( char* str )
{
	(void)str;
	char *shipname;

	shipname = toolkit_getList( secondary_wid, "lstShipyard" );
	ship_view(shipname);
}
static void shipyard_buy( char* str )
{
	(void)str;
	char *shipname;
	Ship* ship;

	shipname = toolkit_getList( secondary_wid, "lstShipyard" );
	ship = ship_get( shipname );

	player_newShip( ship );
}


/*
 * the spaceport bar
 */
static void spaceport_bar (void)
{
	secondary_wid = window_create( "Spaceport Bar",
			-1, -1, BAR_WIDTH, BAR_HEIGHT );

	window_addText( secondary_wid, 20, 20 + BUTTON_HEIGHT + 20,
			BAR_WIDTH - 140, BAR_HEIGHT - 40 - BUTTON_HEIGHT - 60, 0,
			"txtDescription", &gl_smallFont, &cBlack, planet->bar_description );
	
	window_addButton( secondary_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnCloseBar",
			"Close", spaceport_bar_close );
}
static void spaceport_bar_close( char* str )
{
	if (strcmp(str,"btnCloseBar")==0)
		window_destroy(secondary_wid);
}



/*
 * the planetary news reports
 */
static void news (void)
{
	secondary_wid = window_create( "News Reports",
			-1, -1, NEWS_WIDTH, NEWS_HEIGHT );

	window_addText( secondary_wid, 20, 20 + BUTTON_HEIGHT + 20,
			NEWS_WIDTH-40, NEWS_HEIGHT - 20 - BUTTON_HEIGHT - 20 - 20 - 20,
			0, "txtNews", &gl_smallFont, &cBlack,
			"News reporters report that they are on strike!");

	window_addButton( secondary_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnCloseNews",
			"Close", news_close );

}
static void news_close( char* str )
{
	if (strcmp(str,"btnCloseNews")==0)
		window_destroy(secondary_wid);
}


/*
 * lands the player
 */
void land( Planet* p )
{
	if (landed) return;

	/* change music */
	music_load( MUSIC_LAND );
	music_play();

	planet = p;
	land_wid = window_create( p->name, -1, -1, LAND_WIDTH, LAND_HEIGHT );
	
	/*
	 * pretty display
	 */
	window_addImage( land_wid, 20, -40, "imgPlanet", p->gfx_exterior );
	window_addText( land_wid, 440, 80, 200, 460, 0, 
			"txtPlanetDesc", &gl_smallFont, &cBlack, p->description);

	/*
	 * buttons
	 */
	window_addButton( land_wid, -20, 20,
			BUTTON_WIDTH, BUTTON_HEIGHT, "btnTakeoff",
			"Takeoff", (void(*)(char*))takeoff );
	if (planet_hasService(planet, PLANET_SERVICE_COMMODITY))
		window_addButton( land_wid, -20, 20 + BUTTON_HEIGHT + 20,
				BUTTON_WIDTH, BUTTON_HEIGHT, "btnCommodity",
				"Commodity Exchange", (void(*)(char*))commodity_exchange);

	if (planet_hasService(planet, PLANET_SERVICE_SHIPYARD))
		window_addButton( land_wid, -20 - BUTTON_WIDTH - 20, 20,
				BUTTON_WIDTH, BUTTON_HEIGHT, "btnShipyard",
				"Shipyard", (void(*)(char*))shipyard);
	if (planet_hasService(planet, PLANET_SERVICE_OUTFITS))
		window_addButton( land_wid, -20 - BUTTON_WIDTH - 20, 20 + BUTTON_HEIGHT + 20,
				BUTTON_WIDTH, BUTTON_HEIGHT, "btnOutfits",
				"Outfits", (void(*)(char*))outfits);

	if (planet_hasService(planet, PLANET_SERVICE_BASIC)) {
	window_addButton( land_wid, 20, 20,
		BUTTON_WIDTH, BUTTON_HEIGHT, "btnNews",
		"News", (void(*)(char*))news);
	window_addButton( land_wid, 20, 20 + BUTTON_HEIGHT + 20,
		BUTTON_WIDTH, BUTTON_HEIGHT, "btnBar",
		"Spaceport Bar", (void(*)(char*))spaceport_bar);
	}


	landed = 1;
}


/*
 * takes off the player
 */
void takeoff (void)
{
	if (!landed) return;

	music_load( MUSIC_TAKEOFF );
	music_play();

	int sw, sh;
	sw = planet->gfx_space->w;
	sh = planet->gfx_space->h;

	/* set player to another position with random facing direction and no vel */
	player_warp( planet->pos.x + RNG(-sw/2,sw/2), planet->pos.y + RNG(-sh/2,sh/2) );
	vect_pset( &player->solid->vel, 0., 0. );
	player->solid->dir = RNG(0,359) * M_PI/180.;

	/* heal the player */
	player->armour = player->armour_max;
	player->shield = player->shield_max;

	space_init(NULL);

	planet = NULL;
	window_destroy( land_wid );
	landed = 0;
}

