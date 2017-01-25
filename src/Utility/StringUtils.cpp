
#include "Main.h"
#include "StringUtils.h"
#include <wx/regex.h>

namespace StringUtils
{
	wxRegEx re_int1{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT|wxRE_NOSUB };
	wxRegEx re_int2{ "^0[0-9]+$", wxRE_DEFAULT|wxRE_NOSUB };
	wxRegEx re_int3{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT|wxRE_NOSUB };
	wxRegEx re_float{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT|wxRE_NOSUB };
}

bool StringUtils::isInteger(const string& str)
{
	return (re_int1.Matches(str) || re_int2.Matches(str) || re_int3.Matches(str));
}

bool StringUtils::isFloat(const string& str)
{
	return (re_float.Matches(str));
}
