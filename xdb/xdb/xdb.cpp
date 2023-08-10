#pragma once

#include "xdb.h"
#include <stdexcept>
#include <ulib/format.h>
#include <ulib/list.h>
#include <ulib/string.h>

#include <mysql.h>
#include <string.h>

namespace xdb
{
    ulib::string ToString(FieldType type);

    Value::Value(ulib::string_view strval)
    {
        mStrVal = strval;
        mType = FieldType::String;
    }

    Value::Value(uint64 ival)
    {
        mIntVal = ival;
        mType = FieldType::Int;
    }

    ulib::string Value::AsString() const
    {
        if (mType == FieldType::Int)
        {
            return ulib::u8(std::to_string(mIntVal));
        }

        if (mType == FieldType::String)
            return mStrVal;

        throw std::runtime_error("Invalid MYSQL type [is not a string]");
    }

    uint64 Value::AsInt() const
    {
        if (mType == FieldType::Int)
            return mIntVal;

        throw std::runtime_error("Invalid MYSQL type [is not an integer]");
    }

    Value Row::Get(ulib::string_view name) const
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

    Value Row::Get(size_t i) const
    {
        if (mValues.Size() <= i)
            throw std::runtime_error("Row::Get length error");

        return mValues.At(i);
    }

    Connection::Connection(ulib::string_view host, ulib::string_view user, ulib::string_view password,
                           ulib::string_view db, ushort port)
    {
        mSQL = mysql_init(nullptr);
        if (!mSQL)
            throw std::runtime_error("mysql init failed");

        if (!mysql_real_connect((MYSQL *)mSQL, ulib::str(host).c_str(), ulib::str(user).c_str(),
                                ulib::str(password).c_str(), ulib::str(db).c_str(), port, nullptr, 0))
            throw std::runtime_error(ulib::format("mysql_real_connect failed: {}", mysql_error((MYSQL *)mSQL)));
    }

    Connection::Connection(Connection &&conn)
    {
        mSQL = conn.mSQL;
        conn.mSQL = nullptr;
    }

    Connection::~Connection() { mysql_close((MYSQL *)mSQL); }

    Result Connection::SelectImpl(ulib::string_view query)
    {
        if (mysql_query((MYSQL *)mSQL, ulib::str(query).c_str()) != 0)
            throw std::runtime_error(ulib::format("mysql_query failed: {}", mysql_error((MYSQL *)mSQL)));

        MYSQL_RES *result = mysql_store_result((MYSQL *)mSQL);
        if (!result)
        {
            if (mysql_errno((MYSQL *)mSQL) != 0)
                throw std::runtime_error(ulib::format("mysql_store_result failed: {}", mysql_error((MYSQL *)mSQL)));

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

            fields.PushBack(Field{field->name, type});
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
                    values.Add(Value(ulib::string_view(data, length)));
                }
                else if (type == FieldType::Int)
                {
                    values.Add(Value(std::stoll(ulib::string_view(data, length))));
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

    void Connection::QueryImpl(ulib::string_view query)
    {
        if (mysql_query((MYSQL *)mSQL, ulib::str(query).c_str()) != 0)
            throw std::runtime_error(ulib::format("mysql_query failed: {}", mysql_error((MYSQL *)mSQL)));
    }

    ulib::string str(ulib::string_view view)
    {
        ulib::string escaped;
        for (auto ch : view)
        {
            if (ch == '\'' || ch == '\"')
                escaped.push_back(ch);
        }

        return ulib::format(u8"\'{}\'", escaped);
    }
} // namespace xdb