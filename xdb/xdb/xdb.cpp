#pragma once

#include "xdb.h"
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
    ulib::u8string ToString(FieldType type);

    Value::Value(ulib::u8string_view strval)
    {
        mStrVal = strval;
        mType = FieldType::String;
    }

    Value::Value(uint64 ival)
    {
        mIntVal = ival;
        mType = FieldType::Int;
    }

    ulib::u8string Value::AsString() const
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

    const Value &Row::Find(ulib::u8string_view name) const
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

    Connection::Connection(ulib::u8string_view host, ulib::u8string_view user, ulib::u8string_view password,
                           ulib::u8string_view db, ushort port)
    {
        mQueryListener = [] (ulib::u8string_view) {};

        mSQL = mysql_init(nullptr);
        if (!mSQL)
            throw std::runtime_error("mysql init failed");

        bool reconnect = true;
        if(mysql_options(mSQL, MYSQL_OPT_RECONNECT, &reconnect))
            throw std::runtime_error("mysql_options failed");

        if (!mysql_real_connect(mSQL, ulib::str(host).c_str(), ulib::str(user).c_str(), ulib::str(password).c_str(),
                                ulib::str(db).c_str(), port, nullptr, 0))
            throw std::runtime_error(ulib::format("mysql_real_connect failed: {}", mysql_error(mSQL)));
    }

    Connection::Connection(Connection &&conn)
    {
        mSQL = conn.mSQL;
        conn.mSQL = nullptr;
    }

    Connection::~Connection() { mysql_close(mSQL); }

    Result Connection::SelectImpl(ulib::u8string_view query)
    {
        mQueryListener(query);

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

    ulib::u8string str(ulib::u8string_view view)
    {
        ulib::u8string escaped;
        for (auto ch : view)
        {
            if (ch == '\'' || ch == '\"')
                escaped.push_back('\\');
            escaped.push_back(ch);
        }

        return ulib::format(u8"\'{}\'", escaped);
    }
} // namespace xdb