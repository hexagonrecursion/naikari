--[[
   Simple skeleton for your standard faction. This is more or less what the
   standard behaviour can be, but from here you can let your imagination go
   wild.
--]]

-- Reputation settings.
_fdelta_distress = {-1, 0} -- Maximum change constraints
_fdelta_kill = {-5, 1} -- Maximum change constraints

-- Secondary hit modifiers.
_fmod_distress_enemy = 0 -- Distress of the faction's enemies
_fmod_distress_friend = 0.3 -- Distress of the faction's allies
_fmod_kill_enemy = 0.3 -- Kills of the faction's enemies
_fmod_kill_friend = 0.3 -- Kills of the faction's allies
_fmod_misn_enemy = 0.3 -- Missions done for the faction's enemies
_fmod_misn_friend = 0.3 -- Missions done for the faction's allies

_fstanding_friendly = 70
_fstanding_neutral = 0


_ftext_standing = {}
_ftext_standing[100] = _("Legend")
_ftext_standing[90] = _("Hero")
_ftext_standing[70] = _("Comrade")
_ftext_standing[50] = _("Ally")
_ftext_standing[30] = _("Partner")
_ftext_standing[10] = _("Associate")
_ftext_standing[0] = _("Neutral")
_ftext_standing[-1] = _("Outlaw")
_ftext_standing[-30] = _("Criminal")
_ftext_standing[-50] = _("Enemy")

_ftext_friendly = _("Friendly")
_ftext_neutral = _("Neutral")
_ftext_hostile = _("Hostile")
_ftext_bribed = _("Bribed")


--[[
   @brief Clamps a value x between low and high.
--]]
function clamp( low, high, x )
   return math.max( low, math.min( high, x ) )
end


--[[
   @brief Linearly interpolates x between x1,y1 and x2,y2
--]]
function lerp( x, x1, y1, x2, y2 )
   local m = (y1-y2)/(x1-x2)
   local b = y1-m*x1
   return m*x + b
end


--[[
   @brief Same as lerp but clamps to [0,1].
--]]
function clerp( x, x1, y1, x2, y2 )
   return clamp( 0, 1, lerp( x, x1, y1, x2, y2 ) )
end


--[[
   @brief Duplicates a table to avoid clobbering.
--]]
function clone(t)
   local new = {}
   for k, v in pairs(t) do
      new[k] = v
   end
   return new
end


--[[
   @brief Handles a faction hit for a faction.

   Possible sources:
    - "kill" : Pilot death.
    - "distress" : Pilot distress signal.
    - "script" : Either a mission or an event.

      @param current Current faction player has.
      @param amount Amount of faction being changed.
      @param source Source of the faction hit.
      @param secondary Flag that indicates whether this is a secondary (through ally or enemy) hit.
      @return The faction amount to set to.
--]]
function default_hit( current, amount, source, secondary )
   -- Comfort macro
   local f = current
   local delta = {-200, 200}

   -- Set deltas based on source
   local mod = 1
   if source == "distress" then
      delta = clone(_fdelta_distress)

      -- Adjust for secondary hit
      if secondary then
         if amount > 0 then
            mod = mod * _fmod_distress_enemy
         else
            mod = mod * _fmod_distress_friend
         end
      end
   elseif source == "kill" then
      delta = clone(_fdelta_kill)

      -- Adjust for secondary hit
      if secondary then
         if amount > 0 then
            mod = mod * _fmod_kill_enemy
         else
            mod = mod * _fmod_kill_friend
         end
      end
   else
      -- Adjust for secondary hit
      if secondary then
         if amount > 0 then
            mod = mod * _fmod_misn_friend
         else
            mod = mod * _fmod_misn_enemy
         end
      end
   end

   amount = mod * amount
   delta[1] = mod * delta[1]
   delta[2] = mod * delta[2]

   -- Faction gain
   if amount > 0 then
      if source == "kill" or source == "distress" then
         local has_planet
         -- Positive kill, which means an enemy of this faction got
         -- killed. We need to check if this happened in the faction's
         -- territory, otherwise it doesn't count.
         local diff = 0
         local witness = pilot.get({_fthis})
         if witness and #witness > 0 then
            diff = math.min(delta[2], 0.5 * amount * clerp(f, 0, 1, 100, 0.2))
            for i, p in ipairs(witness) do
               if player.pos():dist(p:pos()) < 5000 then
                  -- Witness is close by, so more reputation is gained.
                  diff = math.min(delta[2], amount * clerp(f, 0, 1, 100, 0.2))
                  break
               end
            end
         end
         f = math.min(100, f + diff)
      else
         -- Script induced change. No diminishing returns on these.
         f = math.min(100, f + math.min(delta[2], amount))
      end
   -- Faction loss.
   else
      -- No diminishing returns on loss.
      f = math.max(-100, f + math.max(delta[1], amount))
   end
   return f
end


--[[
   @brief Returns a text representation of the player's standing.

      @param standing Current standing of the player.
      @return The text representation of the current standing.
--]]
function faction_standing_text( standing )
   for i = math.floor( standing ), 0, ( standing < 0 and 1 or -1 ) do
      if _ftext_standing[i] ~= nil then
         return _ftext_standing[i]
      end
   end
   return _ftext_standing[0]
end


--[[
   @brief Returns whether or not the player is a friend of the faction.

      @param standing Current standing of the player.
      @return true if the player is a friend, false otherwise.
--]]
function faction_player_friend( standing )
   return standing >= _fstanding_friendly
end


--[[
   @brief Returns whether or not the player is an enemy of the faction.

      @param standing Current standing of the player.
      @return true if the player is an enemy, false otherwise.
--]]
function faction_player_enemy( standing )
   return standing < _fstanding_neutral
end


--[[
   @brief Returns a text representation of the player's broad standing.

      @param standing Current standing of the player.
      @param bribed Whether or not the respective pilot is bribed.
      @param override If positive it should be set to ally, if negative it should be set to hostile.
      @return The text representation of the current broad standing.
--]]
function faction_standing_broad( standing, bribed, override )
   if override == nil then override = 0 end

   if bribed then
      return _ftext_bribed
   elseif override > 0 or faction_player_friend( standing ) then
      return _ftext_friendly
   elseif override < 0 or faction_player_enemy( standing ) then
      return _ftext_hostile
   else
      return _ftext_neutral
   end
end
