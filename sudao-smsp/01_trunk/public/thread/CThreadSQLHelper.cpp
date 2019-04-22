#include "CThreadSQLHelper.h"
#include "LogMacro.h"


//class Field.
Field::Field()
{
}

Field::~Field()
{
}

int Field::GetField_NO(string field_name)
{

    for (unsigned int i = 0; i < m_name.size(); i++)
    {
        if (!m_name[i].compare(field_name))
            return i;

    }
    return -1;
}

//class Record.
Record::Record(Field * m_f)
{
    m_field = m_f;
}

Record::~Record()
{
}

void Record::SetData(string value)
{
    m_rs.push_back(value);
}

string Record::operator[](string s)
{
    int pos=m_field->GetField_NO(s);
    if(pos == -1)
        return "";

    return m_rs[pos];
}
string Record::operator[](int num)
{
    return m_rs[num];
}


//class RecordSet.
RecordSet::RecordSet()
{
    res = NULL;
    row = NULL;
    pos = 0;
    m_field_num=0;
    m_recordcount=0;
}

RecordSet::RecordSet(MYSQL *hSQL)
{
    res = NULL;
    row = NULL;
    m_Data = hSQL;
    pos = 0;
    m_field_num=0;
    m_recordcount=0;
}

RecordSet::~RecordSet()
{
}

int RecordSet::ExecuteSQL(const char* sql)
{
    if (NULL == m_Data)
    {
        LogErrorP("[ ExecuteSQL ] sql:[%s] m_Data is NULL", sql);
        return -1;
    }

    if (!mysql_real_query(m_Data, sql, strlen(sql)))
    {
        res = mysql_store_result( m_Data );

        if ( NULL == res )
        {
            LogErrorP("[ ExecuteSQL ] sql:[%s] errno[%d].", sql, mysql_errno( m_Data ));
            return -1;
        }

        m_recordcount = (int) mysql_num_rows(res);
        m_field_num = mysql_num_fields(res);

        for (int x = 0; (fd = (MYSQL_FIELD *)mysql_fetch_field(res))>0; x++)
        {
            m_field.m_name.push_back(fd->name);
            m_field.m_type.push_back(fd->type);
        }

        while ((row = mysql_fetch_row(res))>0)
        {
            Record temp(&m_field);

            for (int k = 0; k < m_field_num; k++)
            {
                if (row[k] == NULL || (!strlen(row[k])))
                    temp.SetData("");
                else
                    temp.SetData(row[k]);
            }
            m_s.push_back(temp);
        }

        mysql_free_result(res);
        return m_s.size();
    }

    int err = mysql_errno(m_Data);

    LogErrorP("Call mysql_real_query failed. sql[%s], errno[%d], err[%s].", sql, err, mysql_error(m_Data));

    return -1;
}

int RecordSet::GetRecordCount()
{
    return m_recordcount;
}

Record RecordSet::operator[](int num)
{
    return m_s[num];
}

