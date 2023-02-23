#include "ulib/fmt/format.h"
#include <stdexcept>
#include <xdb/xdb.h>

int main()
{
    try
    {
        xdb::Connection sql(u8"localhost", u8"root", u8"", u8"laravel", 3306);
        auto result = sql.Query(u8"CREATE DATABASE pizdec");
        auto fields = result.GetFields();

        for (auto row : result.GetRows())
        {
            size_t index = 0;
            for (auto field : row.Values())
            {
                fmt::print("[{}]: {}\n", fields[index].GetName(), field.AsString());
                index++;
            }

            fmt::print("\n");
        }
    }
    catch (const std::runtime_error &error)
    {
        fmt::print("error: {}\n", error.what());
    }
    
    return 0;
}