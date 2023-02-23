#pragma once

#include "ulib/encodings/utf8/stringview.h"
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

    ulib::u8string ToString(FieldType type)
    {
        switch (type)
        {
        case FieldType::Int:
            return u8"Int";
        case FieldType::String:
            return u8"String";
        default:
            return u8"Undefined";
        }
    }

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
        Value(ulib::u8string_view strval)
        {
            mStrVal = strval;
            mType = FieldType::String;
        }

        Value(uint64 ival)
        {
            mIntVal = ival;
            mType = FieldType::Int;
        }

        const ulib::u8string &AsString()
        {
            if (mType == FieldType::Int)
            {
                mStrVal = ulib::u8(std::to_string(mIntVal));
                return mStrVal;
            }

            if (mType == FieldType::String)
                return mStrVal;

            throw std::runtime_error("Invalid MYSQL type [is not a string]");
        }

        uint64 AsInt()
        {
            if (mType == FieldType::Int)
                return mIntVal;

            throw std::runtime_error("Invalid MYSQL type [is not an integer]");
        }

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

        Value Get(ulib::u8string_view name)
        {
            size_t index = 0;
            for (auto field : mFields)
            {
                if (field.GetName() == name)
                    return mValues[index];

                index++;
            }

            throw std::runtime_error("unknown name of row element");
        }

        const ulib::List<Value>& Values()
        {
            return mValues;
        }

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
                   ulib::u8string_view db, ushort port)
        {
            mSQL = mysql_init(nullptr);
            if (!mSQL)
                throw std::runtime_error("mysql init failed");

            if (!mysql_real_connect(mSQL, ulib::str(host).c_str(), ulib::str(user).c_str(), ulib::str(password).c_str(),
                                    ulib::str(db).c_str(), port, nullptr, 0))
                throw std::runtime_error(ulib::format("mysql_real_connect failed: {}", mysql_error(mSQL)));
        }

        Connection(Connection &&conn)
        {
            mSQL = conn.mSQL;
            conn.mSQL = nullptr;
        }

        ~Connection() { mysql_close(mSQL); }

        Result Query(ulib::u8string_view query)
        {
            if (mysql_query(mSQL, ulib::str(query).c_str()) != 0)
                throw std::runtime_error(ulib::format("mysql_query failed: {}", mysql_error(mSQL)));

            MYSQL_RES *result = mysql_store_result(mSQL);
            if (!result)
            {
                if (mysql_errno(mSQL) != 0)
                    throw std::runtime_error(ulib::format("mysql_store_result failed: {}", mysql_error(mSQL)));

                return Result{};
            }

            Fields fields;

            size_t numFields = mysql_num_fields(result);
            while (MYSQL_FIELD *field = mysql_fetch_field(result))
            {
                FieldType type = FieldType::Undefined;
                switch (field->type)
                {
                case enum_field_types::MYSQL_TYPE_STRING:
                case enum_field_types::MYSQL_TYPE_VAR_STRING:
                case enum_field_types::MYSQL_TYPE_VARCHAR:
                    type = FieldType::String;
                    break;
                case enum_field_types::MYSQL_TYPE_LONG:
                case enum_field_types::MYSQL_TYPE_INT24:
                case enum_field_types::MYSQL_TYPE_SHORT:
                case enum_field_types::MYSQL_TYPE_TINY:
                case enum_field_types::MYSQL_TYPE_DECIMAL:
                case enum_field_types::MYSQL_TYPE_LONGLONG:
                    type = FieldType::Int;
                    break;
                default:
                    throw std::runtime_error(ulib::format("undefined field type: {}", (int)field->type));
                }

                fields.PushBack(Field{ulib::u8(field->name), type});
            }

            Rows rows;
            while (MYSQL_ROW row = mysql_fetch_row(result))
            {
                unsigned long *lengths = mysql_fetch_lengths(result);

                ulib::List<Value> values;
                for (size_t i = 0; i < numFields; i++)
                {
                    size_t length = lengths[i];
                    char *data = row[i];
                    FieldType type = fields[i].GetType();

                    if (type == FieldType::String)
                    {
                        values.Add(Value(ulib::u8string_view((char8 *)data, length)));
                    }
                    else if (type == FieldType::Int)
                    {
                        values.Add(Value(std::stoll(ulib::u8string_view((char8 *)data, length))));
                    }
                    else
                    {
                        throw std::runtime_error("Bad field type");
                    }
                }

                rows.Add(Row(fields, values));
            }

            return Result(rows.Size(), numFields, fields, rows);
        }

    private:
        MYSQL *mSQL;
    };
} // namespace xdb