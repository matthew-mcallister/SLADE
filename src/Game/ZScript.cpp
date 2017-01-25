
#include "Main.h"
#include "ZScript.h"
#include "Utility/StringUtils.h"

using namespace ZScript;


bool Enumerator::parse(Tokenizer& tz)
{
	// Name first
	string token = tz.getToken();
	if (tz.isSpecialCharacter(token[0]))
	{
		LOG_MESSAGE(
			1,
			"Error parsing enum: Expected valid enumerator name, got \"%s\" at line %d",
			token,
			tz.lineNo()
		);
		return false;
	}
	name = token;

	// Opening brace
	tz.getToken(&token);
	if (token != "{")
	{
		LOG_MESSAGE(
			1,
			"Error parsing enum: Expected \"{\", got \"%s\" at line %d",
			token,
			tz.lineNo()
		);
		return false;
	}

	// Parse values
	Value val;
	long tmp;
	tz.getToken(&token);
	while (token != "}")
	{
		val.name = token;

		// Don't bother parsing values yet, needs a proper parser for stuff like eg.
		// enum { VALUE1 = 2, VALUE2 = VALUE1+1 }
		/*
		tz.getToken(&token);
		if (token == ",")
		{
			val.value++;
			values.push_back({val.name, val.value});
		}
		else if (token == "=")
		{
			tz.getToken(&token);
			if (StringUtils::isInteger(token))
			{
				if (token.ToLong(&tmp))
					val.value = tmp;
				else
				{
					// Unable to parse value, just increment it
					val.value++;
					LOG_MESSAGE(
						1,
						"Warning: Unable to parse value for enum member \"%s\", "
						"later enum values will likely be wrong",
						val.name
					);
				}
			}
			else // Not valid integer
			{
				// Check previous values
				bool is_prev = false;
				for (auto& pval : values)
					if (val.name.CmpNoCase(pval.name) == 0)
					{
						is_prev = true;
						val.value = pval.value;
					}

				if (!is_prev)
				{
					LOG_MESSAGE(
						1,
						"Error parsing enum: Expected valid integer or previous enum member, "
						"got \"%s\" at line %d",
						token,
						tz.lineNo()
					);
					return false;
				}
			}

			values.push_back({val.name, val.value});

			// Next token, skip ',' if found
			tz.getToken(token);
			if (token == ",")
				tz.getToken(token);
		}*/
		tz.getToken(&token);
		while (token != "," && token != "}")
			tz.getToken(&token);

		if (token == ",")
			tz.getToken(&token);

		values.push_back({val.name, 0});
		LOG_MESSAGE(3, "Parsed enum value %s", val.name);
	}

	LOG_MESSAGE(2, "Parsed enum %s successfully", name);
	return true;
}


bool Class::parse(Tokenizer& tz)
{
	// Name first
	string token = tz.getToken();
	if (tz.isSpecialCharacter(token[0]) || token.empty())
	{
		string ctype = (type == Type::Class) ? "class" : "struct";
		LOG_MESSAGE(1, "Error parsing %s: Expected %s name, got %s at line %d", ctype, ctype, token, tz.lineNo());
		return false;
	}
	name = token;

	// Read tokens until {
	tz.getToken(&token);
	while (token != "{")
	{
		token.MakeLower();

		// ':' (inheritance)
		if (token == ":")
		{
			inherits_class = tz.getToken();
			LOG_MESSAGE(2, "Inherits %s", inherits_class);
		}

		// Native class/struct
		else if (token == "native")
			native = true;

		tz.getToken(&token);
	}

	// Skip {
	tz.getToken(&token);

	// Parse
	while (token != "}")
	{
		token.MakeLower();

		// ; (skip)
		if (token == ";")
		{
			tz.getToken(&token);
			continue;
		}

		// Enum
		else if (token == "enum")
		{
			Enumerator e;
			if (!e.parse(tz))
				return false;

			enumerators.push_back(e);
		}

		// Default block
		else if (token == "default")
		{
			// TODO
			while (token != "}")
			{
				tz.getToken(&token);
				token.MakeLower();
			}
		}

		// States
		else if (token == "states")
		{
			// TODO
			while (token != "}")
				tz.getToken(&token);
		}

		// Unknown block (skip it)
		else if (token == "{")
		{
			int level = 1;
			while (level > 0)
			{
				tz.getToken(&token);
				if (token == "}")
					level--;
				else if (token == "{")
					level++;
			}
		}

		// Something else (variable or function)
		else
		{
			// Get all tokens until ';'
			vector<string> tokens;
			tz.getTokensUntil(tokens, ";");
		}

		// Next token
		tz.getToken(&token);
	}

	LOG_MESSAGE(2, "Parsed class/struct %s successfully", name);
	return true;
}


bool Definitions::parseZScript(MemChunk& mc)
{
	Tokenizer tz;
	tz.openMem(&mc, "ZScript");

	string token;
	while (!tz.atEnd())
	{
		tz.getToken(&token);
		token.MakeLower();

		// ; (skip)
		if (token == ";")
			continue;

		else if (token == "struct")
		{
			Class ns(Class::Type::Struct);

			if (!ns.parse(tz))
				return false;

			classes.push_back(ns);
		}

		else if (token == "class")
		{
			Class nc(Class::Type::Class);

			if (!nc.parse(tz))
				return false;

			classes.push_back(nc);
		}

		else if (token == "enum")
		{
			Enumerator e;
			if (!e.parse(tz))
				return false;

			enumerators.push_back(e);
		}

		else if (!tz.atEnd())
		{
			LOG_MESSAGE(
				1,
				"Error parsing ZScript: Unexpected token \"%s\" at line %d",
				token,
				tz.lineNo()
			);
			return false;
		}
	}

	return true;
}



// TEMP TESTING CONSOLE COMMANDS
#include "MainEditor/MainWindow.h"
#include "General/Console/Console.h"

CONSOLE_COMMAND(test_parse_zscript, 1, false)
{
	auto entry = theMainWindow->getCurrentArchive()->entryAtPath(args[0]);
	if (entry)
	{
		Definitions test;
		if (test.parseZScript(entry->getMCData()))
			theConsole->logMessage("Parsed Successfully");
		else
			theConsole->logMessage("Parsing failed");
	}
}
