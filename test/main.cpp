#include "ulib/fmt/format.h"
#include <fmt/chrono.h>
#include <stdexcept>
#include <xdb/xdb.h>


int main()
{
    try
    {
        xdb::Connection sql(u8"localhost", u8"root", u8"", u8"laravel", 3306);

        {
            xdb::Connection sql(u8"localhost", u8"root", u8"", u8"netunit", 3306);

            auto result = sql.Select(u8"SELECT * FROM `activations`");
            for (auto &row : result)
            {
                auto id = row[u8"id"].AsInt();
                auto cheatName = row[u8"cheat_name"].AsString();
                auto timeEnd = row[u8"time_end"].AsInt();
                auto key = row[u8"key"].AsInt();

                fmt::print("[{}] [{}] [{}] until {}\n", id, cheatName, key,
                           ulib::format(u8"[{:%Y-%m-%d %H:%M:%S}] ", fmt::localtime(timeEnd)));
            }
        }

        if (sql.Scalar(u8"select count(*) FROM users WHERE id = 1")->AsInt())
        {
            fmt::print("table users is exists\n");
        }
        else
        {
            fmt::print("table users does not exists\n");
        }

        {
            auto result = sql.Select(u8"CREATE DATABASE pizdec");
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
    }
    catch (const std::runtime_error &error)
    {
        fmt::print("error: {}\n", error.what());
    }

    return 0;
}