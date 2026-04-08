#include "kys_event.h"

#include <format>

std::string DescribeEventModule()
{
    return std::format("{} -> {}", "kys_event.pas", "event dispatch and scripted interaction scaffold");
}