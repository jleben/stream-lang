#pragma once

#include "../../extra/json/json.hpp"
#include "../../utility/filesystem.hpp"

#include <string>

namespace arrp {
namespace generic_io {

// For the purpose of verbose output:
struct log {};

struct options
{
    std::string base_file_name;
};

void generate(const options &, const nlohmann::json & report);

}
}
