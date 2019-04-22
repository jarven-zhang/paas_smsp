#include "keywordClassify.h"
#include<iostream>
#include <algorithm>
#include "main.h"
#include "Comm.h"
#include "UrlCode.h"
#include "CotentFilter.h"


KeywordClassify::KeywordClassify(string childName)
 :m_uDefaultGroupId(0),m_childName(childName)
{
    
}

KeywordClassify::~KeywordClassify()
{
     map<UInt32,searchTree*>::iterator iterOld = m_keywordMap.begin();
     for (; iterOld != m_keywordMap.end();)
     {
         if (NULL != iterOld->second)
         {
             delete iterOld->second;
         }
     
         m_keywordMap.erase(iterOld++);
     }
}

void KeywordClassify::initParam()
{

    return;
}

void KeywordClassify::setClientGrpRefClient(map<string,UInt32>& clientGrpRefClientMap)
{
    m_clientGrpRefClientMap = clientGrpRefClientMap;
    return;
}

void KeywordClassify::setCgrpRefKeywordGrp(map<UInt32,UInt32>&          cgrpRefKeywordGrp, UInt32  defKgrpId)
{
    m_clientGrpRefKeywordGrpMap = cgrpRefKeywordGrp;
    m_uDefaultGroupId = defKgrpId;
    return;
}

void KeywordClassify::setkeyGrpRefCategory(map<UInt32,set<UInt32> >& keywordGrpRefCategoryMap)
{
    m_keywordGrpRefCategoryMap = keywordGrpRefCategoryMap;
    return;
}

void KeywordClassify::initKeywordSearchTree(map<UInt32,list<string> >& mapSetIn)
{
    map<UInt32,searchTree*>::iterator iterOld = m_keywordMap.begin();
    for (; iterOld != m_keywordMap.end();)
    {
        if (NULL != iterOld->second)
        {
            delete iterOld->second;
        }

        m_keywordMap.erase(iterOld++);
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

        m_keywordMap.insert(make_pair(iterNew->first,pTree));
    }

    LogDebug("Update keyword search tree!");
}

bool KeywordClassify::getKeywordCategoryIdSet(UInt32 uKGroupId,set<UInt32>& CategoryIdSet)
{
    std::map<UInt32,set<UInt32> >::iterator itr = m_keywordGrpRefCategoryMap.find(uKGroupId);
    if(itr == m_keywordGrpRefCategoryMap.end())
    {
        LogWarn("KGroupId[%u] not find in t_sms_%s_kgroup_ref_category,please check!",uKGroupId, m_childName.data());
        return false;
    }
    else
    {
        CategoryIdSet = itr->second;
    }
    return true;
}

UInt32 KeywordClassify::getKGroupId(string strClient)
{
    UInt32 uKGroupId = 0;
    map<string,UInt32>::iterator itr = m_clientGrpRefClientMap.find(strClient);
    if(itr == m_clientGrpRefClientMap.end())
    {
        uKGroupId =  m_uDefaultGroupId;
    }
    else
    {
        map<UInt32,UInt32>::iterator itor = m_clientGrpRefKeywordGrpMap.find(itr->second);
        if(itor == m_clientGrpRefKeywordGrpMap.end())
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

bool KeywordClassify::keywordSearch(searchTree* pTree,string& strContent,string& strSign,string& strOut)
{
    if(NULL == pTree)
        return false;
    if (true == pTree->searchSign(strSign,strOut))
    {
        return true;
    }
    if ((!strContent.empty()) && (true == pTree->search(strContent,strOut)))
    {
        return true;
    }
    return false;
}

bool KeywordClassify::keywordCheck(string strClientId, const string &strContent, string strSign, string& strOut, int& keywordFilRegl, bool bIncludeChinese)
{
    UInt32 uKGroupId = 0;
    set<UInt32> CategoryIdSet;
    if(strClientId.empty())
    {
        LogWarn("%skeyword KeywordCheck() failed.  strClientId[%s] is null", m_childName.data(), strClientId.c_str());
        return false;
    }

    string strSignData = "";
    // 若为中文直接采用中文签名符【】
    if (bIncludeChinese)
    {
        static const string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
        static const string strRight = http::UrlCode::UrlDecode("%e3%80%91");
        strSignData = strLeft + strSign + strRight;
    }
    // 若为英文直接采用英文签名符[]
    else
    {
        strSignData = "[" + strSign + "]";
    }

    filter::ContentFilter keywordFilter;
    string dbKeywordSign = "", dbKeywordContent = "";
    keywordFilter.contentFilter(strSignData, dbKeywordSign, keywordFilRegl);
    if (!strContent.empty())
    {
        keywordFilter.contentFilter(strContent, dbKeywordContent, keywordFilRegl);
    }

    uKGroupId = getKGroupId(strClientId);
    LogDebug("%skeyword Client[%s] get uKGroupId[%u] strSignData[%s]", m_childName.data(), strClientId.c_str(), uKGroupId, strSignData.c_str());
    if(false == getKeywordCategoryIdSet(uKGroupId,CategoryIdSet))
    {
        LogWarn("%skeyword Client[%s] Get CategoryId fail",m_childName.data(), strClientId.c_str());
        return false;
    }

    for(set<UInt32>::iterator itr = CategoryIdSet.begin(); itr != CategoryIdSet.end(); itr++)
    {
        LogDebug("%skeyword Client[%s] search keyword in CategoryId[%u]",m_childName.data(), strClientId.c_str(), *itr);
        std::map<UInt32,searchTree*>::iterator itor = m_keywordMap.find(*itr);
        if(itor == m_keywordMap.end())
        {
            continue;
        }
        if(true == keywordSearch(itor->second,dbKeywordContent,dbKeywordSign,strOut))
        {
            return true;
        }
    }   
    return false;
    
}

