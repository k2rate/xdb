#include <ulib/format.h>
#include <stdexcept>
#include <xdb/xdb.h>

int main()
{
    try
    {
        fmt::print("{}\n", xdb::str("dsaf"));
        xdb::Connection sql(u8"localhost", u8"root", u8"", u8"conditer11", 3306);

        fmt::print("categories: {}\n", sql.IsTableExists("categories"));
        fmt::print("plakmp: {}\n", sql.IsTableExists("plakmp"));

        if (sql.Scalar(u8"select count(*) FROM users WHERE id = 1").AsInt())
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