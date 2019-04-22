#include "searchTree.h"
#include "Comm.h"


searchTree::searchTree()
{

}

searchTree::~searchTree()
{
	DelNode(m_mapSet);
}

void searchTree::initTree(list<string>& listSet)
{

	for (list<string>::iterator iter = listSet.begin(); iter != listSet.end(); ++iter) 
	{
		string& strData = *iter;		
		m_signSet.insert(strData);
		
		map<char, searchNode*>* pMapSet = &m_mapSet;
		for (unsigned int i = 0; i < strData.length(); i++)
		{
			map<char, searchNode*>::iterator itr = pMapSet->find(strData[i]);
			if (itr == pMapSet->end())
			{
				searchNode* pNode = new searchNode(strData[i]);
				pMapSet->insert(make_pair(strData[i], pNode));
				if (i == (strData.length() - 1))
				{
					pNode->m_flag = true;
				}
				pMapSet = &(pNode->m_mapSet);
			}
			else
			{
				if (i == (strData.length() - 1))
				{
					itr->second->m_flag = true;
				}
				pMapSet = &(itr->second->m_mapSet);
			}
		}
	}
}
void searchTree::InitIgnoreMap(map<string,set<string> >& mapKeywordIgnore)
{
	m_mapKeywordIgnore = mapKeywordIgnore;
}

void searchTree::DelTree()
{
	DelNode(m_mapSet);
}

void searchTree::DelSet()
{
	m_signSet.clear();
	m_mapKeywordIgnore.clear();
}

bool searchTree::searchSign(string& strSign,string& strOut,const string& strIgnoreMapKey)
{
	std::set<string>::iterator it;
	if (strSign.empty())
		return false;
	string strData = strSign;
	it = m_signSet.find(strData);
	if (it == m_signSet.end())
	{
		return false;
	}
	if(!strIgnoreMapKey.empty() && searchIgnoreKeyword(strData,strIgnoreMapKey))
	{
		return false;
	}
	strOut.append(strSign);
	return true;
}

bool searchTree::search(string& strIn,string& strOut,const string& strIgnoreMapKey)
{
	if (strIn.empty())
		return false;

	string strData = strIn;
	int p = 0;
	int v = 0;
	
	map<char, searchNode*>* pMapSet = NULL;

	bool bFlag = false;

	for (unsigned int i = 0; i < strData.length();)
	{
		pMapSet = &m_mapSet;
		p = i;
		v = 0;

		for (;;)
		{
			map<char, searchNode*>::iterator itr = pMapSet->find(strData[p++]);
			if (itr == pMapSet->end())
			{
				i++;
				break;
			}
			else
			{
				pMapSet = &(itr->second->m_mapSet);
			}

			if (true == itr->second->m_flag)
			{
				if(!strIgnoreMapKey.empty() && searchIgnoreKeyword(strIn.substr(i,p-i),strIgnoreMapKey))
				{
					continue;
				}
				v = p;
				bFlag = true;
				break;
			}
		}

		if (true == bFlag)
		{
			strOut = strIn.substr(i,v-i);
			return true;
		}
	}

	return false;
}


void searchTree::DelNode(map<char,searchNode*>& mapSet)
{
	map<char, searchNode*>::iterator itr =  mapSet.begin();
	for(;itr != mapSet.end();)
	{
		DelNode(itr->second->m_mapSet);
		delete itr->second;
		mapSet.erase(itr++);
	}
}

/*
	find strDateIn in m_mapKeywordIgnore return true
	    else return false
*/
bool searchTree::searchIgnoreKeyword(const string& strDateIn,const string& strIgnoreMapKey)
{
	if(strDateIn.empty() || strIgnoreMapKey.empty())
		return false;
	
	map<string,set<string> >::iterator itr = m_mapKeywordIgnore.find(strIgnoreMapKey);
	if(itr != m_mapKeywordIgnore.end() )
	{
		set<string>::iterator itor = itr->second.find(strDateIn);
		if(itor != itr->second.end())
		{
			return true;
		}
	}
	return false;
}

