#pragma once

#include <stdexcept>

#include <ulib/format.h>
#include <ulib/list.h>
#include <ulib/string.h>

namespace xdb
{
    enum class FieldType
    {
        String,
        Int,
        Undefined
    };

    ulib::string ToString(FieldType type);

    class Field
    {
    public:
        Field(ulib::string_view name, FieldType type) : mName(name), mType(type) {}

        ulib::string_view GetName() const { return mName; }
        FieldType GetType() const { return mType; }

    private:
        ulib::string mName;
        FieldType mType;
    };

    using Fields = ulib::List<Field>;

    class Value
    {
    public:
        Value(ulib::string_view strval);
        Value(uint64 ival);

        ulib::string AsString() const;
        uint64 AsInt() const;

        FieldType GetType() { return mType; }

    private:
        FieldType mType;
        ulib::string mStrVal;
        uint64 mIntVal;
    };

    class Row
    {
    public:
        Row() {}
        Row(const Fields &fields, const ulib::List<Value> &values) : mFields(fields), mValues(values) {}

        Value Get(ulib::string_view name) const;
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
        Connection(ulib::string_view host, ulib::string_view user, ulib::string_view password,
                   ulib::string_view db, ushort port);
        Connection(Connection &&conn);
        ~Connection();

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

    private:
        void *mSQL;
    };

    inline ulib::string str(ulib::string_view view);
} // namespace xdb