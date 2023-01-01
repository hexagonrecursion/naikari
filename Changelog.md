# NAIKARI CHANGELOG

## 0.5.2

* Fixed some OpenGL code that made assumptions that are invalid in the
  OpenGL spec, causing the game to not display on some systems as a
  result.

## 0.5.1

* Made it so that the player's escorts do not respond to AI distress
  calls. This prevents annoying situations where they pull away from
  formation unnecessarily or, worse, go to help an ally against a
  faction who's an enemy to them, but who you would rather remain
  neutral with.
* Fixed a breakage in the Collective AI initialization code.

## 0.5.0

### Major Gameplay and Design Changes

* Cargo missions now reward much more credits if they are on a route
  that has pirate presence.
* Increased the amount of health medium-heavy and heavier ships have.
* Weapons have been heavily altered. Weapon damage, range, and behavior
  vary a lot more widely than before.
* Removed the faction standing cap system. You can now gain as much
  reputation as you want with all factions (even the Collective). This
  also means all ships and outfits are available to obtain without
  having to resort to piracy.
* Added refined Unicorp platings, which serve as a middle-ground between
  the basic plating and the S&K platings. These new Unicorp X platings
  use the graphics of the Unicorp B platings Naev used to have.
* Added automatic bounties awarded whenever you kill a faction's enemy
  within a system it has presence in (e.g. killing pirates in systems
  with Empire presence, or killing FLF pilots in systems with Dvaered
  presence).
* Coupled together Frontier and FLF standing: if you are enemies with
  the Frontier, FLF standing will not go any higher than your Frontier
  standing (and your reputation will drop to enemy status when you
  become enemies with the Frontier).
* Buffed the Rotary Turbo Modulator.
* Buffed the Gawain's speed and takeoff speed. (It was already the
  fastest ship in the galaxy before, but the difference is now more
  pronounced.)
* Increased variation of commodity prices.
* Removed the mission computer bounty missions and the Assault on
  Unicorn mission as these are now redundant (bounties are earned
  without having to have a corresponding quest).
* Significantly improved the Seek and Destroy mission: difficulty levels
  have been added (like the old bounty missions had), the way NPCs
  behave is now more customized per-faction, and when you arrive at the
  appropriate system, you now need to locate your target within the
  system (similar to the ship stealing mission). The mission also now
  rewards you immediately upon completion instead of requiring you to
  land to collect your reward.
* Replaced the news entry generator, which previously just generated
  random filler with no connection to the story, with news entries that
  are relevant to the player, like mission hints.
* Added "escape jumps", which allow you to engage your hyperspace engine
  without actually going into hyperspace, taking you to a distant part
  of the system at a cost of one jump of fuel, all of your battery
  charge, half of your remaining shield, and half of your remaining
  armor. (The maneuver takes you a distance of around 30,000 mAU.)
* Converted the Reynir mission into a hidden tutorial for the local
  jumps feature and changed the rewarded commodity from Food (hot dogs)
  to Luxury Goods (robot teddy bears), making it more rewarding.
* "High-class" planets now require 10% reputation instead of 0% to land
  on.
* Removed the DIY-Nerds mission (the one where you take a group of nerds
  to a science fair or something). It never felt all that important and
  wasn't really worth doing.
* Civilians and traders will now always offer to refuel you if you
  cannot make a jump, even if you don't have credits to pay them.
* Collective drones will now refuel you if you are friendly to them.
* Added a rare "refuel request" event which causes an NPC pilot who
  needs fuel to request assistance from the player in exchange for some
  credits.
* Essentially completely redesigned the universe, most notably changing
  what planets sell, but also changing or removing some planets and
  systems. The new design has the following characteristics:
  * All planets contain a common set of basic outfits, which includes
    the "MK1" forward-facing variant of each type of weapon, basic stat
    enhancers, maps, the Mercenary License, and the low-quality
    zero-cost cores.
  * Each faction has a different set of weapons that they sell, with
    Independent and Frontier planets getting the weapons of their
    neighbors. The Empire sells laser weapons, the Soromid sell plasma
    weapons, the Dvaered sell impact weapons, the Za'lek sell beam
    weapons, and the Siriusites sell razor weapons.
  * Made sure every planet has a purpose within the system; no more
    redundant, useless planets that you would never want to land on,
    unless those planets are "low-class" (making them useful to
    outlaws).
  * Planets and stations that require more reputation to land on give
    progressively more access to exclusive ships and outfits, and
    progressively better outfits in general.
  * Planets with both an outfitter and a shipyard have their outfits
    chosen deliberately to compliment the ships they have on offer.
  * All licenses needed to purchase all outfits and ships on a given
    planet are available for purchase on that same planet (no more need
    to go looking for the license you need).
  * Changed jump points in some locations (particularly in Sirius space)
    to avoid inordinate numbers of dead-end systems. Dead-end systems
    still exist, but they are now much less common and an effort was
    made to ensure that the ones that still exist are worth going to
    for some reason or another, or otherwise serve a legitimate gameplay
    purpose.
* The race mission no longer requires you to be piloting a yacht ship to
  participate, and the ships you race against are more varied.

### Other Changes

* All bar missions are prevented from spawning in Hakoi and Eneguez
  until you finish the Ian Structure missions.
* The Empire Recruitment mission is now guaranteed to send you to a
  nearby planet with commodities as well as missions.
* Removed a random chance that existed of pirates attacking to kill
  (meaning they will now always stop attacking and board you as soon as
  you're disabled).
* Added some rescue code to the starting missions in case the player
  takes off their weapons, installing new laser cannons for them in the
  moment that they're needed.
* The Options menu now always shows 1280×720 as a stock resolution
  choice, even if not listed as a supported mode by the OS (since that
  is Naikari's default resolution), and no longer shows a choice with
  the resolution you were at when opening the Options menu.
* Double-tap afterburn is now disabled by default.
* Replaced "cycles" with "galactic years", which are 36,000,000 galactic
  seconds long (360 galactic days). Lore-wise, time units are now
  defined in relation to our 24-hour Earth days, meaning the new
  galactic year is exactly 360 days.
* The economy system no longer tracks "price knowledge". The player now
  always has perfect knowledge of pricing variation for all discovered
  planets and systems.
* The patrol mission now rewards you immediately upon completion without
  having to land in the faction's territory first.
* Changed the unit of mass used from tonnes to kilotonnes (meaning we no
  longer have the silliness of space carriers weighing less than naval
  aircraft carriers, etc).
* The game no longer refuses to resize a maximized window if you tell it
  to in the Options menu. (This change also fixes bugs with the code
  that implemented this restriction.)
* Changed the way autonav detects "hostile presence" for the purposes
  of determining whether to reset the speed that time passes. It now
  only counts ships that are close enough that one of the two ships
  will be within weapon range soon (or already are). This avoids most
  unnecessary time slowdowns while also being cautious enough to give
  advance warning to the player.
* Made it so that the debris graphics in asteroid fields are entirely
  separate from the asteroids themselves, making it more obvious which
  asteroids are real and which are just decoration.
* Changed the relation between real-world time passage and in-universe
  time passage: one real-world second is now equal to 750 in-universe
  seconds, rather than 30 in-universe seconds as before. In tandem, the
  primary components to a date have been changed from year, hour, second
  to year, day, second. (In-game time-related things have been changed
  to now use days instead of hours; for example, jump time is now one
  day instead of one hour.)
* Changed the distance unit used from kilometers to thousandths of an
  astronomical unit (mAU).
* Changed the display of unidirectional jumps in the map to use the
  color black for the exit-only side, rather than white. This is a bit
  more intuitive and also should be easier to see for colorblind
  players.
* Auto-saves are no longer disabled when landing on planets that do not
  refuel you. Instead, the design philosophy is being shifted to
  ensuring that unwinnable states are impossible anywhere you're allowed
  to land.
* Made NPC mercenaries no longer explicitly enemies with pirates (though
  they will still usually attack pirates due to their "bounty hunting"
  behavior).
* The System Info display now shows planets' color and character
  (indicating whether or not they can be landed on) on the selection
  list, which makes it much easier to navigate.
* Default weapons for ships now vary (instead of all of them using laser
  weaponry); Sirius ships have razor weaponry, Dvaered ships have
  kinetic weaponry, and Za'lek ships have beam weaponry. (Soromid ships
  already had bio-plasma weaponry since that was the only option.)
* Improved the patrol mission's detection of hostile ships and made it
  more sensitive.
* Reworked bullet graphics to make better use of what's available. There
  are no longer large numbers of weapons using the same graphics as each
  other.
* Race missions now use random portraits, rather than always one
  particular portrait.

### Bugfixes

* Fixed some problems with the way the Combat Practice mission closed
  out.
* Fixed the rescue script that activates when you take off, which had
  several small problems in how it worked caused by changes that weren't
  properly accommodated.
* Fixed a bug which caused the game to sometimes try to change to an
  arbitrary small resolution (640×480 in all observed cases) on some
  systems (noticed on Windows, but may have affected others) when
  clicking OK in the options menu.
* Fixed a problem where the Waste Dump mission would make your own
  escorts hostile toward you if you aborted it while in space.
* Extremely high turn rates no longer have the potential to mess up
  autonav during time compression.
* Fixed a rare bug caused by dying while in the landing procedure.
* Fixed possibility of dying while landing due to nebula volatility.
* Added code to prevent phantom 0 kt cargo entries from showing up in
  your cargo list.
* Fixed escorts of AI ships not jumping when their leader jumped (which
  left them adrift in the system indefinitely).
* Fixed the player being allowed to board and steal credits from their
  own hired escorts.
* Adjusted the music script to hopefully fix an uncommon bug where
  combat music played while landed.
* Fixed a double land denial message when attempting to land on
  something you're not allowed to land on with the land key.
* Fixed a game-breaking bug where the "animal trouble" event made it
  impossible to proceed with the game without rescuing yourself via the
  Lua console due to a script error.
* Fixed spawning of hostile mercenaries by the third Nexus mission (it
  was spawning the mercenaries, but failed to set them as hostile to the
  player).
* Fixed Za'lek non-drone ships being impossible to hail, as with drones.
* Fixed AI ships which spawned prior to you entering a system being
  uncooperative and/or unresponsive.
* Fixed a 14-year-old bug where the comm_no flag of AI ships (the one
  that caused the text "No response" to be printed rather than showing
  the comm window when you hailed them) pointed to an arbitrary ship,
  rather than necessarily pointing to the relevant one. This usually
  didn't have noticeable effects, *except* that it caused Za'lek boss
  ships to be unresponsive, as if they were drones, most of the time.

## 0.4.1

* Added conversion brackets to the starting missions showing what the
  "t" and "¢" suffixes mean.
* Reverted the change that causes Target Nearest to exclude escorts, as
  this caused a regression where you couldn't target them by clicking on
  them.
* Reverted an experimental SDL hint which broke the non-Linux builds and
  likely wasn't necessary anyway.

## 0.4.0

* Corrected the calculation for beam heat-up; the previous inaccuracy
  led to beams heating up faster than they were supposed to.
* Adjusted the Equipment screen and the Info ship tab, showing the
  player's credits on the Equipment screen and the ship's value on the
  Info ship tab.
* Added a Net Worth stat to the Info overview tab, showing the total
  value of your credits, ships, and outfits combined.
* Added some NPC messages.
* Fixed and adjusted some missions.
* Knowledge of the FLF's hidden jumps is now erased if you betray them.
* Fixed bad rendering of marker text.
* The FLF/Dvaered derelicts event now requires a Mercenary License and
  can only occur outside of Frontier space.
* Changed the name of the Info Window to Ship Computer.
* New combat practice mission available through the mission computer.
* New map showing waste disposal locations.
* Opening tutorial replaced with a new start-of-game campaign that
  teaches the basics of playing the game in a more natural and
  integrated fashion. This new campaign is now integrated with the
  Empire Recruitment mission, used as the basis for why you are
  recruited by the Empire rather than them just randomly choosing a warm
  body.
* Reworked the trader escort mission: the trader convoy now travels as
  a more natural fleet (going into a formation), and it also limits its
  speed so it isn't faster than you.
* Replaced the UST time system inherited from Naev with a slightly
  different time system which is called GCT, or Galactic Common Time.
  The time units are the same, but "periods" are now called "galactic
  hours", "hectoseconds" are now called "galactic minutes",
  "decaperiods" are now called "galactic days", and the words "week"
  and "month" are now colloquially used to describe time units similar
  to a week or a month.
* The Equipment screen's slot tooltips now show slot properties even
  when an item is equipped, alongside the properties of the equipped
  item.
* Added tooltips when the mouse is placed over the shipyard slot
  indicator squares. The tooltips are identical to the tooltips shown on
  the Equipment screen; they show what kind of slot they indicate and
  what outfit comes with the ship in that slot, if any.
* Right-clicking on jump points no longer engages auto-hyperspace
  behavior and instead simply falls back to standard location-based
  autonav. This is for consistency with planet auto-landing.
* The detail of shipyards being required to manage ships you aren't
  currently flying in the Equipment screen has been removed; the
  Equipment screen now functions identically on all planets that have
  them. The previous paradigm was unnecessarily confusing and didn't
  really add anything to the game.
* Added a quantity display to the image array in the Commodity tab,
  allowing you to see at a glance what cargo you're carrying.
* Added a "Follow Target" keyboard control (bound to F by default).
* The Target Nearest key now will not ever target your own escorts,
  since this is not something you're likely to want to do. The Target
  Next and Target Previous keys can still target your escorts if needed.
* Added a jump that's easy to miss to the Frontier map.
* Imported new ion and razor weaponry graphics from Naev.
* The Brushed GUI now displays the instant mode weapon set for weapons
  assigned to them. This is particularly important as it shows the
  player how to launch fighters from fighter bays with the default
  weapon configuration. To accommodate this change and keep it looking
  consistent, the Brushed GUI has been altered slightly.
* Hired escorts now list their speed so you can know in advance whether
  or not they can keep up with your ship.
* Removed the intro crawl when starting a new game. It didn't really add
  anything necessary, it was basically just a history lesson delivered
  in a rather boring manner. Any such information could easily be
  conveyed in better ways and most of it already is.
* Changed the mission marker colors and slot size colors to yellow,
  orange, and red, retaining full colorblind accessibility and the
  gradual color shift of the previous coloring while making the colors
  used more distinct.
* AI ships that come with fighter bays now count more toward filling the
  presence quota, which means they no longer inflate the amount of ships
  in the system like they used to.

## 0.3.0

* Changed Soromid taunts (the old ones inherited from Naev sounded too
  much like social darwinism, a harmful and pseudo-scientific ideology).
* Changed volume sliders to show a percentage in a standard form rather
  than the raw floating-point numbers that were previously displayed.
* The alt text when hovering your mouse over an outfit has been
  slightly rearranged and a credits display has been added to it.
* Removed the once-per-version "Welcome to Naikari" message.
* Pirate names are now generated in a different way which should be a
  bit nicer.
* Added many more possibilities for the randomly generated pilot names.
* Fixed a bug which caused pirate cargo missions to incorrectly
  calculate number of jumps to the destination (leading to lower
  rewards and a warning about you not knowing the fastest route).
* Fixed a cosmetic bug which could lead to you having phantom cargo
  after stealing cargo from ships (caused by a condition that would lead
  to AI pilots having 0 tonnes of a cargo added to them).
* Bombers now have a 20% radar range bonus (which makes their missiles
  more accurate).
* Restructured the FLF campaign so that the "Anti-Dvaered" and "Hero"
  storylines happen in parallel, rather than in an entangled fashion.
* Tutorial has been streamlined so that it introduces the game much
  better than before.
* System names for unknown systems that are marked by a mission are now
  consistently displayed everywhere (whereas previously they were
  displayed in the starmap's sidebar, but nowhere else).
* Fixed some problems with the Waste Dump mission.
* Pulled new beam store graphics from Naev.
* Escort missions now have hardened claims, preventing edge cases of
  escort missions conflicting with other missions that do things like
  clear the system. A side effect of this is that trader escort missions
  are now only offered one at a time. In tandem with this, escort
  missions' probability has been adjusted so that they are still seen
  frequently, and the trader escort mission in particular has been given
  a higher priority so that bounty missions don't prevent it from
  showing up.
* Fixed glitchy text in overlay when appearing over other text.
* Fixed pirates sometimes using Lancelot fighter bays.
* Added several new texts for the Love Train mission.
* Changed the story context of the first FLF diversion mission; it is
  now a diversion to rescue FLF soldiers trapped in a Dvaered system.
* Added several new components for randomly generated pilot names.
* Mace rockets no longer have thrust, and rockets with thrust no longer
  start at 0 speed. This change was made because the calculations the
  game made for range were not even close when trying to account for the
  acceleration component (a problem that has existed for many years in
  Naev but only became a problem because of mace rockets' reduced
  range). Thrust is now used solely for guided missiles.
* Removed the planet named "Sheik Hall" since it has no gameplay value
  and its name is iffy.

## 0.2.1

* Fixed a bug that caused the game to not start under Windows.

## 0.2.0

* Changed the land and takeoff music to be the same as ambient music.
* Adjusted balancing of outfits and ships, most noticeably changing the
  Kestrel's two large fighter bay slots to medium fighter bay slots and
  making the Huntsman torpedo much stronger.
* All ships now come with pre-installed weapons when you buy them, not
  just your first ship.
* The warning shown when warping into a system with a volatile nebula
  now shows exactly how much damage your shield and armor take from it,
  rather than only showing the direct volatility rating.
* AI pilots now launch fighters if they have fighter bays.
* Several new fighter bays have been added (mostly miniaturized versions
  of existing fighter bays, but also three variations of a Shark
  fighter bay).
* Hired escorts that are created on restricted planets and stations now
  pilot factional military ships.
* The Info window's missions tab now displays the current objective
  according to the OSD.
* Logo now lights up red for Autism Acceptance Month, turns into a
  rainbow for Queer Pride Month, and turns into aromantic pride colors
  for Aromantic Spectrum Awareness Week.
* Asteroid Scanner now always shows you scan information for asteroids
  you can see, rather than only after you've targeted them.
* Replaced Improved Refrigeration Cycle with the Rotary Turbo Modulator,
  which does the opposite of what the Improved Refrigeration Cycle did
  and serves as an equivalent to the Power Regulation Override outfit
  for turrets.
* Instant mode weapons now show up on the weapon bar in the Brushed GUI.
* Ship Stealing mission now allows stealing of non-designated targets,
  at the cost of having to pay for the entire value of the ship (meaning
  it can't be used to make credits, it can only be used to steal
  particular ships you want to add to your collection). If it fails, it
  keeps running so you can steal a ship of your choice instead, and the
  OSD message changes to note this.
* Adjusted how much money ships carry to hopefully make illegal piracy
  more rewarding and make pirating pirates less rewarding, while also
  adding variety to piracy (some factions carry a lot of credits, some
  carry very little).
* Pirates no longer have a one-way hidden jump to Collective space
  (which served no purpose and was essentially just a death trap).
* Adjusted speed and size range of background nebulae and stars so they
  look nicer.
* Ship Stealing targets now lose most of their ammo in addition to armor
  and battery, and their armor regeneration is now disabled.
* Added a limit to mercenary group size in systems with low paying
  faction presence.
* Normalized some wonky presence costs, which in particular prevents
  Za'lek drones from outnumbering pirates in systems where they're
  supposed to have lower presence than them.
* Added a new set of stations and hidden jumps to help pirates get to
  Sirius space.
* The Brushed and Slim GUIs now hide the radar while the overlay is
  open.
* Maps now each have their own individual graphics, rather than all of
  them sharing the same graphic. The graphics are screenshots showing a
  Discovery mode map window showing just the information provided by the
  respective map.
* Improved the way music tracks are chosen, preventing needless music
  changes and allowing nebula ambient music in factional areas.
* Fixed some pirates on pirate worlds being described as civilians.
* Afterburners no longer cause the screen to wobble.
* Removed the "Time Constant" tutorial, both to avoid making the system
  look more significant than it is and because the information about how
  to equip ships is redundant now that this is effectively
  self-documented (with all ships coming with reasonable weapons).
* Removed the ability to delete ship logs, which was a feature that
  didn't have any real utility for the player and risked deleting
  important information.
* Simplified the ship log display, removing the "log type" selector and
  adding a display of the currently selected log.

## 0.1.3

* Fixed Drunkard mission not being able to start up.
* Fixed Nebula Probe mission failing to advance to the next step after
  launching the probe (making it impossible to complete).
* Fixed extra copy of the Pinnacle during the Baron campaign missions.
* Fixed Pinnacle stalling after completing Baron missions (caused by a
  long-standing API bug).
* Fixed Anxious Merchant mission being unable to be accepted.

## 0.1.2

* Properly fixed a segfault in 0.1.0. Version 0.1.1 attempted a fix, but
  the fix turned out to be faulty and led to breakage.

## 0.1.1

Faulty release.

## 0.1.0

Initial release.
