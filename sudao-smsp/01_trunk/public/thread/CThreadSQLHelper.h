#ifndef __CTHREAD_SQLHELPER_H__
#define __CTHREAD_SQLHELPER_H__

#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>

using namespace std;


class Field
{
public:
    Field();
    ~Field();
    int GetField_NO(string field_name);

public:
    vector<string> m_name;
    vector<enum_field_types> m_type;

};


class Record
{
public:
    Record() {};
    Record(Field* m_f);
    ~Record();

    void SetData(string value);
    string operator[](string s);
    string operator[](int num);

public:
    vector<string> m_rs;
    Field *m_field;
};


class RecordSet
{
public:
    RecordSet();
    RecordSet(MYSQL *hSQL);
    ~RecordSet();

    int ExecuteSQL(const char* sql);
    int GetRecordCount();
    Record operator[](int num);
    vector<Record>& getRecordSet() {return m_s;}

private:
    vector<Record> m_s;
    unsigned long pos;
    int m_recordcount;
    int m_field_num;
    Field m_field;

    MYSQL_RES * res;
    MYSQL_FIELD * fd;
    MYSQL_ROW row;
    MYSQL* m_Data;
};

#endif /* __CTHREAD_SQLHELPER_H__ */
