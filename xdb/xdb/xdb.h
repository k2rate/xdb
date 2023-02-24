#pragma once

#include "ulib/encodings/utf8/stringview.h"
#include "ulib/fmt/format.h"
#include <stdexcept>
#include <ulib/format.h>
#include <ulib/list.h>
#include <ulib/string.h>
#include <ulib/u8.h>

#include <mysql.h>
#include <optional>
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

        const Value& Find(ulib::u8string_view name) const;

        inline auto begin() const { return mValues.Begin(); }
        inline auto end() const { return mValues.End(); }
        inline auto begin() { return mValues.Begin(); }
        inline auto end() { return mValues.End(); }
        inline size_t size() const { return mValues.Size(); }
        inline bool empty() const { return mValues.Empty(); }
        inline bool contains() const { return !mValues.Empty(); }
        inline const Value &front() const { return mValues.Front(); }
        inline const Value &at(size_t i) const { return mValues.At(i); }

        inline auto Begin() const { return mValues.Begin(); }
        inline auto End() const { return mValues.End(); }
        inline auto Begin() { return mValues.Begin(); }
        inline auto End() { return mValues.End(); }
        inline size_t Size() const { return mValues.Size(); }
        inline bool Empty() const { return mValues.Empty(); }
        inline bool Contains() const { return !mValues.Empty(); }
        inline const Value &Front() const { return mValues.Front(); }
        inline const Value &At(size_t i) const { return mValues.At(i); }
        inline const Value &At(ulib::u8string_view name) const { return Find(name); }

        inline const Value &operator[](size_t i) const { return mValues[i]; }
        inline const Value &operator[](ulib::u8string_view name) const { return At(name); }

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

        size_t NumRows() const { return mNumRows; }
        size_t NumColumns() const { return mNumColumns; }

        const Fields &GetFields() const { return mFields; }
        const Rows &GetRows() const { return mRows; }

        inline auto begin() const { return mRows.Begin(); }
        inline auto end() const { return mRows.End(); }
        inline auto begin() { return mRows.Begin(); }
        inline auto end() { return mRows.End(); }
        inline size_t size() const { return NumRows(); }
        inline bool empty() const { return mNumRows == 0; }
        inline bool contains() const { return mNumRows != 0; }
        inline const Row &front() const { return mRows.Front(); }
        inline const Row &at(size_t i) const { return mRows[i]; }

        inline auto Begin() const { return mRows.Begin(); }
        inline auto End() const { return mRows.End(); }
        inline auto Begin() { return mRows.Begin(); }
        inline auto End() { return mRows.End(); }
        inline size_t Size() const { return NumRows(); }
        inline bool Empty() const { return mNumRows == 0; }
        inline bool Contains() const { return mNumRows != 0; }
        inline const Row &Front() const { return mRows.Front(); }
        inline const Row &At(size_t i) const { return mRows[i]; }

        inline const Row &operator[](size_t i) const { return mRows[i]; }
        inline const Row &Single()
        {
            if (Empty())
                throw std::runtime_error("Row does not have any elements");
            return At(0);
        }

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
        std::optional<Row> Single(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            auto &rows = result.GetRows();
            if (rows.Empty())
                return std::nullopt_t({});

            return rows.Front();
        }

        template <typename... T>
        std::optional<Value> Scalar(ulib::u8string_view fmt, T &&...args)
        {
            ulib::u8string query = ulib::format(fmt, args...);
            auto result = SelectImpl(query);

            if (result.Empty())
                return std::nullopt_t({});

            auto &row = result.Front();
            if (row.Empty())
                return std::nullopt_t({});

            return row.Values().Front();
        }

        Result SelectImpl(ulib::u8string_view query);

    private:
        MYSQL *mSQL;
    };

    ulib::u8string str(ulib::u8string_view view);
} // namespace xdb