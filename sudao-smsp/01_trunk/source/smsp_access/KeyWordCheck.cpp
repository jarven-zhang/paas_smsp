#include "KeyWordCheck.h"
#include<iostream>
#include <algorithm>
#include "main.h"
#include "Comm.h"
#include "UrlCode.h"
#include "CotentFilter.h"

using namespace std;


KeyWordCheck::KeyWordCheck(Int32 iKeywordCovRegular)
{
    m_mapAuditCGroupRefClient.clear();
    m_mapAuditClientGroup.clear();
    m_mapAuditKgroupRefCategory.clear();
    m_uDefaultGroupId = 0;
    m_iAuditKeywordCovRegular = iKeywordCovRegular;
}

KeyWordCheck::~KeyWordCheck()
{
    map<UInt32,searchTree*>::iterator iterOld = m_mapAuditKeyWord.begin();
    for (; iterOld != m_mapAuditKeyWord.end();)
    {
        if (NULL != iterOld->second)
        {
            delete iterOld->second;
        }

        m_mapAuditKeyWord.erase(iterOld++);
    }
}

void KeyWordCheck::initParam()
{
    m_mapAuditCGroupRefClient.clear();
    m_mapAuditClientGroup.clear();
    m_mapAuditKgroupRefCategory.clear();
    m_uDefaultGroupId = 0;
    m_mapKeywordIgnore.clear();
    map<UInt32,list<string> > mapAuditKeyWordList;
    mapAuditKeyWordList.clear();

    g_pRuleLoadThread->getIgnoreAuditKeyWordMap(m_mapKeywordIgnore);

    g_pRuleLoadThread->getAuditKeyWordMap(mapAuditKeyWordList);
    initAuditKeyWordSearchTree(mapAuditKeyWordList);

    g_pRuleLoadThread->getAuditKgroupRefCategoryMap(m_mapAuditKgroupRefCategory);
    g_pRuleLoadThread->getAuditClientGroupMap(m_mapAuditClientGroup,m_uDefaultGroupId);
    g_pRuleLoadThread->getAuditCGroupRefClientMap(m_mapAuditCGroupRefClient);
    return;
}
void KeyWordCheck::initKeyWordRegular(Int32 iKeywordCovRegular)
{
    m_iAuditKeywordCovRegular = iKeywordCovRegular;
    LogNotice("Update AuditKeyWord Regular:%d",m_iAuditKeywordCovRegular);
    return;
}
void KeyWordCheck::initAuditKeyWordSearchTree(map<UInt32,list<string> >& mapSetIn)
{
    map<UInt32,searchTree*>::iterator iterOld = m_mapAuditKeyWord.begin();
    for (; iterOld != m_mapAuditKeyWord.end();)
    {
        if (NULL != iterOld->second)
        {
            delete iterOld->second;
        }

        m_mapAuditKeyWord.erase(iterOld++);
    }

    for (map<UInt32,list<string> >::iterator iterNew = mapSetIn.begin(); iterNew != mapSetIn.end(); ++iterNew)
    {
        searchTree* pTree = new searchTree();
        if (NULL == pTree)
        {
            LogFatal("pTree is NULL.");
            return;
        }

        pTree->initTree(iterNew->second);
        pTree->InitIgnoreMap(m_mapKeywordIgnore);
        m_mapAuditKeyWord.insert(make_pair(iterNew->first,pTree));
    }
}
void KeyWordCheck::initIgnoreAuditKeyWord(map<string,set<string> >& mapKeywordIgnore)
{
    m_mapKeywordIgnore = mapKeywordIgnore;
    for(map<UInt32,searchTree*>::iterator it = m_mapAuditKeyWord.begin(); it != m_mapAuditKeyWord.end(); it++)
    {
        it->second->InitIgnoreMap(mapKeywordIgnore);
    }
    return;
}
bool KeyWordCheck::GetAuditKeyWordCategoryIdSet(UInt32 uKGroupId,set<UInt32>& CategoryIdSet)
{
    std::map<UInt32,set<UInt32> >::iterator itr = m_mapAuditKgroupRefCategory.find(uKGroupId);
    if(itr == m_mapAuditKgroupRefCategory.end())
    {
        LogWarn("KGroupId[%u] not find in t_sms_audit_kgroup_ref_category,please check!",uKGroupId);
        return false;
    }
    else
    {
        CategoryIdSet = itr->second;
    }
    return true;
}
UInt32 KeyWordCheck::GetAuditKGroupId(string strClient)
{
    UInt32 uKGroupId = 0;
    map<string,UInt32>::iterator itr = m_mapAuditCGroupRefClient.find(strClient);
    if(itr == m_mapAuditCGroupRefClient.end())
    {
        uKGroupId =  m_uDefaultGroupId;
    }
    else
    {
        map<UInt32,UInt32>::iterator itor = m_mapAuditClientGroup.find(itr->second);
        if(itor == m_mapAuditClientGroup.end())
        {
            uKGroupId = m_uDefaultGroupId;
        }
        else
        {
            uKGroupId = itor->second;
        }
    }
    return uKGroupId;
}
bool KeyWordCheck::KeyWordSearch(searchTree* pTree,string& strContent,string& strSign,string& strOut,const string& strIgnoreMapKey)
{
    if(NULL == pTree)
        return false;
    if (true == pTree->searchSign(strSign,strOut,strIgnoreMapKey))
    {
        return true;
    }
    if (true == pTree->search(strContent,strOut,strIgnoreMapKey))
    {
        return true;
    }
    return false;
}
bool KeyWordCheck::AuditKeywordCheck(string strClientId,string strContent,string strSign,string& strOut, string& strIgnoreMapKey)
{
    UInt32 uKGroupId = 0;
    set<UInt32> CategoryIdSet;
    if(strContent.empty())
    {
        LogWarn("AuditKeywordCheck() failed. strContent is null,strClientId[%s]", strClientId.c_str());
        return false;
    }

    static const string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
    static const string strRight = http::UrlCode::UrlDecode("%e3%80%91");

    string strSignData = "";
    strSignData.append(strLeft);
    strSignData.append(strSign);
    strSignData.append(strRight);

    filter::ContentFilter sysKeywordFilter;
    string dbAuditKeywordSign;
    string dbAuditKeywordContent;
    sysKeywordFilter.contentFilter(strSignData, dbAuditKeywordSign, m_iAuditKeywordCovRegular);
    sysKeywordFilter.contentFilter(strContent, dbAuditKeywordContent, m_iAuditKeywordCovRegular);

    uKGroupId = GetAuditKGroupId(strClientId);
    LogDebug("Client[%s] get Audit uKGroupId[%u]",strClientId.c_str(),uKGroupId);
    if(false == GetAuditKeyWordCategoryIdSet(uKGroupId,CategoryIdSet))
    {
        LogWarn("Client[%s] Get AuditCategoryId fail",strClientId.c_str());
        return false;
    }

    for(set<UInt32>::iterator itr = CategoryIdSet.begin(); itr != CategoryIdSet.end(); itr++)
    {
        LogDebug("Client[%s] search auditkeyword in CategoryId[%u]",strClientId.c_str(),*itr);
        std::map<UInt32,searchTree*>::iterator itor = m_mapAuditKeyWord.find(*itr);
        if(itor == m_mapAuditKeyWord.end())
        {
            continue;
        }
        if(true == KeyWordSearch(itor->second,dbAuditKeywordContent,dbAuditKeywordSign,strOut,strIgnoreMapKey))
        {
            return true;
        }
    }
    return false;

}



