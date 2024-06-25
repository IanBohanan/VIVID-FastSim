#pragma once

#include <boost/json.hpp>

void prettyPrintJson(std::ostream& os, boost::json::value const& jv, std::string* indent);