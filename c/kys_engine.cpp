#include "kys_engine.h"

#include <format>

std::string DescribeEngineModule()
{
    return std::format("{} -> {}", "kys_engine.pas", "resource loading and shared engine scaffold");
}