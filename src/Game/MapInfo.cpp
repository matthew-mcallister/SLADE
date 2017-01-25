
#include "Main.h"
#include "MapInfo.h"
#include "Archive/Archive.h"


MapInfo::MapInfo()
{
}

MapInfo::~MapInfo()
{
}

void MapInfo::clear(bool maps, bool editor_nums)
{
	if (maps)
	{
		this->maps.clear();
		default_map = Map();
	}

	if (editor_nums)
		this->editor_nums.clear();
}

MapInfo::Map& MapInfo::getMap(string name)
{
	for (auto& map : maps)
		if (map.entry_name == name)
			return map;

	return default_map;
}

bool MapInfo::addOrUpdateMap(Map& map)
{
	for (auto& m : maps)
		if (m.entry_name == map.entry_name)
		{
			m = map;
			return true;
		}

	maps.push_back(map);
	return false;
}

bool MapInfo::checkEqualsToken(Tokenizer& tz, string parsing)
{
	string check = tz.getToken();
	if (check != "=")
	{
		LOG_MESSAGE(
			1,
			"Error Parsing %s: Expected \"=\", got \"%s\" at line %d",
			parsing,
			check,
			tz.lineNo()
		);
		return false;
	}

	return true;
}

bool MapInfo::strToCol(string str, rgba_t& col)
{
	wxColor wxcol;
	if (!wxcol.Set(str))
	{
		// Parse RR GG BB string
		auto components = wxSplit(str, ' ');
		if (components.size() >= 3)
		{
			long tmp;
			components[0].ToLong(&tmp, 16);
			col.r = tmp;
			components[1].ToLong(&tmp, 16);
			col.g = tmp;
			components[2].ToLong(&tmp, 16);
			col.b = tmp;
			return true;
		}
	}
	else
	{
		col.r = wxcol.Red();
		col.g = wxcol.Green();
		col.b = wxcol.Blue();
		return true;
	}

	return false;
}

bool MapInfo::parseZMapInfo(ArchiveEntry* entry)
{
	Tokenizer tz;
	tz.openMem(&(entry->getMCData()), entry->getName());

	string token = tz.getToken();
	while (!tz.atEnd())
	{
		token.MakeLower();

		// Include
		if (token == "include")
		{
			// Get entry at include path
			tz.getToken(&token);
			ArchiveEntry* include_entry = entry->getParent()->entryAtPath(token);

			if (!include_entry)
			{
				LOG_MESSAGE(
					1,
					"Warning - Parsing ZMapInfo \"%s\": Unable to include \"%s\" at line %d",
					entry->getName(),
					token,
					tz.lineNo()
				);
			}
			else if (!parseZMapInfo(include_entry))
				return false;
		}

		// Map
		else if (token == "map" ||
			token == "defaultmap" ||
			token == "adddefaultmap")
		{
			if (!parseZMap(tz, token))
				return false;
		}

		// DoomEdNums
		else if (token == "doomednums")
		{
			if (!parseDoomEdNums(tz))
				return false;
		}

		// Unknown block (skip it)
		else if (token == "{")
		{
			LOG_MESSAGE(
				2,
				"Warning - Parsing ZMapInfo \"%s\": Skipping {} block",
				entry->getName()
			);

			int level = 1;
			while (level > 0)
			{
				tz.getToken(&token);
				if (token == "{")
					level++;
				else if (token == "}")
					level--;
			}
		}

		// Unknown
		else
		{
			LOG_MESSAGE(
				2,
				"Warning - Parsing ZMapInfo \"%s\": Unknown token \"%s\"",
				entry->getName(),
				token
			);
		}

		tz.getToken(&token);
	}

	LOG_MESSAGE(2, "Parsed ZMapInfo entry %s successfully", entry->getName());

	return true;
}

bool MapInfo::parseZMap(Tokenizer& tz, string type)
{
	// TODO: Handle adddefaultmap

	Map map = default_map;

	// Normal map, get lump/name/etc
	string token = tz.getToken();
	if (type == "map")
	{
		// Entry name should be just after map keyword
		map.entry_name = token;

		// Parse map name
		tz.getToken(&token);
		if (token.CmpNoCase("lookup") == 0)
		{
			map.lookup_name = true;
			map.name = tz.getToken();
		}
		else
		{
			map.lookup_name = false;
			map.name = token;
		}

		tz.getToken(&token);
	}

	if (token != "{")
	{
		LOG_MESSAGE(
			1,
			"Error Parsing ZMapInfo: Expecting \"{\", got \"%s\" at line %d",
			token,
			tz.lineNo()
		);
		return false;
	}

	tz.getToken(&token);
	while (token != "}")
	{
		token.MakeLower();

		// Block (skip it)
		if (token == "{")
		{
			int level = 1;
			while (level > 0)
			{
				tz.getToken(&token);
				if (token == "{")
					level++;
				else if (token == "}")
					level--;
			}
		}

		// LevelNum
		else if (token == "levelnum")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// Parse number
			// TODO: Checks
			tz.getInteger(&map.level_num);
		}

		// Sky1
		else if (token == "sky1")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			tz.getToken(&map.sky1);

			// Scroll speed
			// TODO: Checks
			if (tz.checkToken(","))
				tz.getFloat(&map.sky1_scroll_speed);
		}

		// Sky2
		else if (token == "sky2")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			tz.getToken(&map.sky2);

			// Scroll speed
			// TODO: Checks
			if (tz.checkToken(","))
				tz.getFloat(&map.sky2_scroll_speed);
		}

		// Skybox
		else if (token == "skybox")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			tz.getToken(&map.sky1);
		}

		// DoubleSky
		else if (token == "doublesky")
			map.sky_double = true;

		// ForceNoSkyStretch
		else if (token == "forcenoskystretch")
			map.sky_force_no_stretch = true;

		// SkyStretch
		else if (token == "skystretch")
			map.sky_stretch = true;

		// Fade
		else if (token == "fade")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			if (!strToCol(tz.getToken(), map.fade))
				return false;
		}

		// OutsideFog
		else if (token == "outsidefog")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			if (!strToCol(tz.getToken(), map.fade_outside))
				return false;
		}

		// EvenLighting
		else if (token == "evenlighting")
		{
			map.lighting_wallshade_h = 0;
			map.lighting_wallshade_v = 0;
		}

		// SmoothLighting
		else if (token == "smoothlighting")
			map.lighting_smooth = true;

		// VertWallShade
		else if (token == "vertwallshade")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// TODO: Checks
			tz.getInteger(&map.lighting_wallshade_v);
		}

		// HorzWallShade
		else if (token == "horzwallshade")
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// TODO: Checks
			tz.getInteger(&map.lighting_wallshade_h);
		}

		// ForceFakeContrast
		else if (token == "forcefakecontrast")
			map.force_fake_contrast = true;

		tz.getToken(&token);
	}

	if (type == "map")
	{
		LOG_MESSAGE(2, "Parsed ZMapInfo Map %s (%s) successfully", map.entry_name, map.name);
		
		// Update existing map
		bool updated = false;
		for (auto& m : maps)
			if (m.entry_name == map.entry_name)
			{
				m = map;
				updated = true;
				break;
			}

		// Add if it didn't exist
		if (!updated)
			maps.push_back(map);
	}
	else if (type == "defaultmap")
		default_map = map;

	return true;
}

bool MapInfo::parseDoomEdNums(Tokenizer& tz)
{
	// Opening brace
	string token = tz.getToken();
	if (token != "{")
	{
		LOG_MESSAGE(
			1,
			"Error Parsing ZMapInfo: Expecting \"{\", got \"%s\" at line %d",
			token,
			tz.lineNo()
		);
		return false;
	}

	tz.getToken(&token);
	long number, tmp;
	while (token != "}")
	{
		// Editor number
		if (!token.ToLong(&number))
		{
			LOG_MESSAGE(
				1,
				"Error Parsing ZMapInfo DoomEdNums: Expecting editor number, got \"%s\" at line %d",
				tz.currentToken(),
				tz.lineNo()
			);
			return false;
		}

		// Reset editor number values
		editor_nums[number].special = "";
		for (int a = 0; a < 5; a++)
			editor_nums[number].args[a] = 0;

		// =
		if (!tz.checkToken("="))
		{
			LOG_MESSAGE(
				1,
				"Error Parsing ZMapInfo DoomEdNums: Expecting \"=\", got \"%s\" at line %d",
				tz.currentToken(),
				tz.lineNo()
			);
			return false;
		}

		// Actor Class
		tz.getToken(&token);
		editor_nums[number].actor_class = token;

		// Check for special/args definition
		tz.getToken(&token);
		if (token == ",")
		{
			int arg = 0;

			// Check if next is a special or arg
			if (!tz.checkLong(&tmp))
				editor_nums[number].special = tz.currentToken();
			else
				editor_nums[number].args[arg++] = tmp;

			// Parse any further args
			while (tz.peekToken() == ",")
			{
				tz.skipToken();

				if (!tz.checkLong(&tmp) && tz.currentToken() != "+")
				{
					LOG_MESSAGE(
						1,
						"Error Parsing ZMapInfo DoomEdNums: Expecting arg value, got \"%s\" at line %d",
						tz.currentToken(),
						tz.lineNo()
					);
					return false;
				}

				if (arg < 5 && tz.currentToken() != "+")
					editor_nums[number].args[arg++] = tmp;
			}

			tz.getToken(&token);
		}
	}

	LOG_MESSAGE(2, "Parsed ZMapInfo DoomEdNums successfully");

	return true;
}

void MapInfo::dumpDoomEdNums()
{
	for (auto& num : editor_nums)
	{
		if (num.second.actor_class == "")
			continue;

		LOG_MESSAGE(
			1,
			"DoomEdNum %d: Class \"%s\", Special \"%s\", Args %d,%d,%d,%d,%d",
			num.first,
			num.second.actor_class,
			num.second.special,
			num.second.args[0],
			num.second.args[1],
			num.second.args[2],
			num.second.args[3],
			num.second.args[4]
		);
	}
}



// TEMP TESTING STUFF
#include "MainEditor/MainWindow.h"
#include "General/Console/Console.h"

CONSOLE_COMMAND(test_parse_zmapinfo, 1, false)
{
	Archive* archive = theMainWindow->getCurrentArchive();
	if (archive)
	{
		ArchiveEntry* entry = archive->entryAtPath(args[0]);
		if (!entry)
			theConsole->logMessage("Invalid entry path");
		else
		{
			MapInfo test;
			if (test.parseZMapInfo(entry))
				test.dumpDoomEdNums();
		}
	}
}
