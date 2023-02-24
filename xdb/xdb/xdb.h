#pragma once

#include "ulib/encodings/utf8/stringview.h"
#include "ulib/fmt/format.h"
#include <stdexcept>
#include <ulib/format.h>
#include <ulib/list.h>
#include <ulib/string.h>
#include <ulib/u8.h>

#include <mysql.h>
#include <string.h>

namespace xdb
{
    enum class FieldType
    {
        String,
        Int,
        Undefined
    };

    ulib::u8string ToString(FieldType type);

    class Field
    {
    public:
        Field(ulib::u8string_view name, FieldType type) : mName(name), mType(type) {}

        ulib::u8string_view GetName() const { return mName; }
        FieldType GetType() const { return mType; }

    private:
        ulib::u8string mName;
        FieldType mType;
    };

    using Fields = ulib::List<Field>;

    class Value
    {
    public:
        Value(ulib::u8string_view strval);
        Value(uint64 ival);

        ulib::u8string AsString() const;
        uint64 AsInt() const;

        FieldType GetType() { return mType; }

    private:
        FieldType mType;
        ulib::u8string mStrVal;
        uint64 mIntVal;
    };

    class Row
    {
    public:
        Row() {}
        Row(const Fields &fields, const ulib::List<Value> &values) : mFields(fields), mValues(values) {}

        Value Get(ulib::u8string_view name) const;
        Value Get(size_t i) const;

        const ulib::List<Value> &Values() const { return mValues; }

    private:
        Fields mFields;
        ulib::List<Value> mValues;
    };

    using Rows = ulib::List<Row>;

    class Result
    {
    public:
        Result() : mNumRows(0), mNumColumns(0) {}
        Result(size_t numRows, size_t numColumns, const Fields &fields, const Rows &rows)
            : mNumRows(numRows), mNumColumns(numColumns), mFields(fields), mRows(rows)
        {
        }

        size_t NumRows() { return mNumRows; }
        size_t NumColumns() { return mNumColumns; }
        const Fields &GetFields() { return mFields; }
        const Rows &GetRows() { return mRows; }

    private:
        size_t mNumRows;
        size_t mNumColumns;
        Fields mFields;
        Rows mRows;
    };

    class Connection
    {
    public:
        Connection(ulib::u8string_view host, ulib::u8string_view user, ulib::u8string_view password,
                   ulib::u8string_view db, ushort port);
        Connection(Connection &&conn);
        ~Connection();

        template <typename... T>
        void Query(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            SelectImpl(query);
        }

        template <typename... T>
        Result Select(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            return SelectImpl(query);
        }

        template <typename... T>
        Row Single(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            if (result.GetRows().Empty())
                throw std::runtime_error("No rows returned in .SelectFirst");

            return result.GetRows().At(0);
        }

        template <typename... T>
        Value Scalar(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            if (result.GetRows().Empty())
                throw std::runtime_error("No rows returned in .Scalar");

            return result.GetRows().At(0).Get(0);
        }

        Result SelectImpl(ulib::u8string_view query);

    private:
        MYSQL *mSQL;
    };

    inline ulib::u8string str(ulib::u8string_view view);
} // namespace xdb