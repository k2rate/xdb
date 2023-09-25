#pragma once

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

        const Value &Get(ulib::string_view name) const;
        const Value &Get(size_t i) const;

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
        inline const Value &At(ulib::string_view name) const { return Get(name); }

        inline const Value &operator[](size_t i) const { return mValues[i]; }
        inline const Value &operator[](ulib::string_view name) const { return At(name); }

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
}