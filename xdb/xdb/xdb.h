#pragma once

#include <stdexcept>

#include <functional>
#include <ulib/format.h>
#include <ulib/list.h>
#include <ulib/runtimeerror.h>
#include <ulib/string.h>

#include "row.h"

namespace xdb
{
    enum class ValueType
    {
        Int,
        VarChar,
    };

    struct TableColumn
    {
        ValueType type;
        bool isNullable;
        size_t size;
    };

    class Blueprint
    {
    public:
    private:
        ulib::List<TableColumn> mColumns;
    };

    class Connection
    {
    public:
        Connection(ulib::string_view host, ulib::string_view user, ulib::string_view password, ulib::string_view db,
                   ushort port);
        Connection(Connection &&conn);
        ~Connection();

        void SetQueryListener(std::function<void(ulib::string_view)> listener) { mQueryListener = listener; }

        template <typename... T>
        void Query(ulib::string_view fmt, T &&...args)
        {
            ulib::string query = ulib::format(fmt, args...);
            QueryImpl(query);
        }

        template <typename... T>
        Result Select(ulib::string_view fmt, T &&...args)
        {
            ulib::string query = ulib::format(fmt, args...);
            return SelectImpl(query);
        }

        template <typename... T>
        Row Single(ulib::string_view fmt, T &&...args)
        {
            ulib::string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            if (result.GetRows().Empty())
                throw std::runtime_error("No rows returned in .SelectFirst");

            return result.GetRows().At(0);
        }

        template <typename... T>
        Value Scalar(ulib::string_view fmt, T &&...args)
        {
            ulib::string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            if (result.GetRows().Empty())
                throw std::runtime_error("No rows returned in .Scalar");

            return result.GetRows().At(0).Get(0);
        }

        Result SelectImpl(ulib::string_view query);
        void QueryImpl(ulib::string_view query);

        // void CreateTable(ulib::string_view table, std::function<void()>) {}
        bool IsTableExists(ulib::string_view name);

    private:
        void *mSQL;
        ulib::string mSchemaName;
        std::function<void(ulib::string_view)> mQueryListener;
    };

    ulib::string str(ulib::string_view view);
} // namespace xdb