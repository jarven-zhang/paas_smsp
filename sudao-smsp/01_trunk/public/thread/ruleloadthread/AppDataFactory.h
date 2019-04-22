#ifndef THREAD_RULELOADTHREAD_FACTORY_H
#define THREAD_RULELOADTHREAD_FACTORY_H

#include "platform.h"
#include "AppData.h"
#include "TableId.h"
#include "LogMacro.h"


#define CASE_TABLE(tableid) \
    { \
        case tableid: \
        { \
            return new AppData_##tableid(); \
        } \
    }


class AppDataFactory
{
public:
    static AppDataFactory& getInstance()
    {
        static AppDataFactory s;
        return s;
    }

    AppData* newAppData(UInt32 uiTableId)
    {
        switch (uiTableId)
        {
            CASE_TABLE(t_sms_table_update_time);
            CASE_TABLE(t_sms_middleware_configure);
            CASE_TABLE(t_sms_mq_configure);
            CASE_TABLE(t_sms_mq_thread_configure);
            CASE_TABLE(t_sms_component_configure);
            CASE_TABLE(t_sms_component_ref_mq);
            CASE_TABLE(t_sms_param);
            CASE_TABLE(t_sms_agent_info);
            CASE_TABLE(t_sms_agent_account);
            CASE_TABLE(t_sms_direct_client_give_pool);
            CASE_TABLE(t_sms_user_property_log);
            CASE_TABLE(t_sms_account);
            CASE_TABLE(t_sms_client_send_limit);

            default:
            {
                LogError("Invalid tableid:%u.", uiTableId);
                return NULL;
            }
        }

        return NULL;
    }

private:
    AppDataFactory() {}
    AppDataFactory(const AppDataFactory&);
    AppDataFactory& operator=(const AppDataFactory&);
};


#endif // THREAD_RULELOADTHREAD_FACTORY_H
