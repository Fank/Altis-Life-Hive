Altis-Life-Hive
===============

## Function calls: ##

**0** Init the hive

**100** Get Player information

1. SteamId (int 64Bit)


**101** Update Player Civilian

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Gear (const char *)
5. Licenses (const char *)
6. Arrested (bool)
7. PlayerName (const char *)

**102** Update Player Resistance

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Gear (const char *)
5. Licenses (const char *)
6. Arrested (bool)
7. PlayerName (const char *)

**103** Update Player Cop

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Gear (const char *)
5. Licenses (const char *)
6. PlayerName (const char *)

**111** Add Player Civilian

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Arrested (bool)
5. PlayerName (const char *)

**112** Add Player Resistance

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Arrested (bool)
5. PlayerName (const char *)

**113** Add Player Cop

1. SteamId (int 64Bit)
2. Cash (int)
3. Bank (int)
4. Gear (const char *)
5. PlayerName (const char *)

**200** Get Vehicles (--)

**201** Insert vehicle

1. SteamId (int 64Bit)
2. Side (char *)
3. Type (char *)
4. Classname (char *)
5. Color (int)
6. Plate (int)

**202** Set vehicle active

1. SteamId (int 64Bit)
2. VehicleId (int)
3. Active (bool)

**203** Set vehicle alive

1. SteamId (int 64Bit)
2. VehicleId (int)
3. Alive (bool)
