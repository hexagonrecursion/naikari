require "factions/standing/skel"


_fdelta_distress = {-1, 0} -- Maximum change constraints
_fdelta_kill = {-5, 0.5} -- Maximum change constraints

_fthis = faction.get("Thurion")

_fmod_kill_enemy = 0.01


function faction_hit(current, amount, source, secondary)
    return default_hit(current, amount, source, secondary)
end
