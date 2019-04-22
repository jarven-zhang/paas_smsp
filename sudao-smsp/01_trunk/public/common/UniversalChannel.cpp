#include "UniversalChannel.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"
#include "xml.h"
#include "global.h"

using namespace boost;
using namespace smsp;

#define AS_STRING(JSON)		((JSON).type() == Json::stringValue ? (JSON).asString() 					\
																  : ((JSON).type() == Json::intValue ? int2str((JSON).asInt()) : ""))
#define AS_INT(JSON)		((JSON).type() == Json::intValue ? (JSON).asInt() : 0)

namespace smsp{

string int2str(int idata){
	char chtmp[16] = {0};
	snprintf(chtmp, sizeof(chtmp), "%d", idata);

	return string(chtmp);
}

void parseXml_MT(TiXmlNode* pParent, field_pattern &pattern,
						std::string& smsid, std::string& status, std::string& strReason){
	//printf("on %s\n", __FUNCTION__);
    if(NULL == pParent)
		return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();
    //printf( "type %d \n", t);

    switch(t){
  	   case TiXmlNode::TINYXML_DOCUMENT:

           	break;

  	   case TiXmlNode::TINYXML_ELEMENT:
            //printf( "Element [%s], child: %s", pParent->Value(), pParent->FirstChild() != NULL
			//														? pParent->FirstChild()->Value() : "NULL");
			if(NULL != pParent->FirstChild()){
				for(field_pattern::iterator it = pattern.begin(); it != pattern.end(); ++it){
					regex exp(get<1>((*it).second));
					cmatch what;

					if(0 == strcmp(get<0>((*it).second).c_str(), pParent->Value())){
						if(false == regex_search(pParent->FirstChild()->Value(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n",
														exp.str().c_str(), pParent->FirstChild()->Value());

							continue;
						}
						switch((*it).first){
							case FIELD_STATUS:{
								cmatch w;
								regex reg(get<2>((*it).second).c_str());

								//printf("status: %s, ok key: %s\n", what.str().c_str(), get<2>((*it).second).c_str());
								if(false == regex_match(what.str().c_str(), w, reg)){
									printf("regex_match err, expression: %s\n", reg.str().c_str());
									status = what.str();
								}
								else{
									status = "0";		// it is represented success at outside logical
								}
								strReason = what.str();

								break;
							}
							case FIELD_SMSID:
								smsid = what.str();

								break;
							#if 0
							case FIELD_ERRMSG:{
								strReason = what.str();

								break;
							}
							#endif

							default:{

							}
						}
					}
				}
			}

            break;

 	   case TiXmlNode::TINYXML_COMMENT:
            break;

	   case TiXmlNode::TINYXML_UNKNOWN:
            break;

 	   case TiXmlNode::TINYXML_TEXT:{
	   		pText = pParent->ToText();
            break;
 	   	}
		case TiXmlNode::TINYXML_DECLARATION :
			break;

		default:
			break;
    }

    for(pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()){
    	parseXml_MT(pChild, pattern, smsid, status, strReason);
    }
}


void getNodesByTag(TiXmlNode* pParent, std::string tag, std::vector<TiXmlNode *>& nodes){
    if(NULL == pParent)
		return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();

    switch(t){
  	   case TiXmlNode::TINYXML_DOCUMENT:
           break;

  	   case TiXmlNode::TINYXML_ELEMENT:
            printf( "Element [%s], child: %s", pParent->Value(), pParent->FirstChild() != NULL
																	? pParent->FirstChild()->Value() : "NULL");
			if(0 == strcmp(pParent->Value(), tag.c_str()))
				nodes.push_back(pParent);
            break;

 	   case TiXmlNode::TINYXML_COMMENT:
            break;

	   case TiXmlNode::TINYXML_UNKNOWN:
            break;

 	   case TiXmlNode::TINYXML_TEXT:{
	   		pText = pParent->ToText();
            break;
 	   	}
		case TiXmlNode::TINYXML_DECLARATION:
			break;

		default:
			break;
    }

    for(pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()){
    	getNodesByTag(pChild, tag, nodes);
    }
}


void parseXml_RT(TiXmlNode* pParent, field_pattern &pattern, smsp::StateReport& report){
    if(NULL == pParent)
		return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();
    //printf( "type %d \n", t);

    switch(t){
  	   case TiXmlNode::TINYXML_DOCUMENT:
           break;

  	   case TiXmlNode::TINYXML_ELEMENT:
            //printf( "Element [%s], child: %s", pParent->Value(), pParent->FirstChild() != NULL
			//														? pParent->FirstChild()->Value() : "NULL");
			if(NULL != pParent->FirstChild()){
				for(field_pattern::iterator it = pattern.begin(); it != pattern.end(); ++it){
					regex exp(get<1>((*it).second));
					cmatch what;

					if(0 == strcmp(get<0>((*it).second).c_str(), pParent->Value())){
						if(false == regex_search(pParent->FirstChild()->Value(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n",
														exp.str().c_str(), pParent->FirstChild()->Value());

							continue;
						}
						switch((*it).first){

							case FIELD_SMSID:{
								report._smsId = what.str();

								break;
							}
							case FIELD_STATUS:{
								cmatch w;
								regex reg(get<2>((*it).second).c_str());

								printf("ok key: %s\n", get<2>((*it).second).c_str());
								if(false == regex_match(what.str().c_str(), w, reg)){
									printf("regex_match err, expression: %s\n", reg.str().c_str());
									report._status = SMS_STATUS_CONFIRM_FAIL;

								}
								else{
									printf("regex_match suc, expression: %s\n", get<2>((*it).second).c_str());
									report._status = SMS_STATUS_CONFIRM_SUCCESS;
								}
								report._desc = what.str();

								break;
							}
							case FIELD_MOBILES:{
								report._phone = what.str();

								break;

							}
							#if 0
							case FIELD_ERRMSG:{
								report._desc = what.str();

								break;
							}
							#endif
							default:{

							}
						}
					}
				}
			}

            break;

 	   case TiXmlNode::TINYXML_COMMENT:
            break;

	   case TiXmlNode::TINYXML_UNKNOWN:
            break;

 	   case TiXmlNode::TINYXML_TEXT:
	   		pText = pParent->ToText();
            break;

		case TiXmlNode::TINYXML_DECLARATION:
			break;

		default:
			break;
    }

    for(pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()){
    	parseXml_RT(pChild, pattern, report);
    }
}

void parseXml_MO(TiXmlNode* pParent, field_pattern &pattern, smsp::UpstreamSms &up){
    if(NULL == pParent)
		return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();
    //printf( "type %d \n", t);

    switch(t){
  	   case TiXmlNode::TINYXML_DOCUMENT:
           break;

  	   case TiXmlNode::TINYXML_ELEMENT:
            //printf( "Element [%s], child: %s", pParent->Value(), pParent->FirstChild() != NULL
			//														? pParent->FirstChild()->Value() : "NULL");
			#if 1
			if(NULL != pParent->FirstChild()){
				for(field_pattern::iterator it = pattern.begin(); it != pattern.end(); ++it){
					regex exp(get<1>((*it).second));
					cmatch what;

					if(0 == strcmp(get<0>((*it).second).c_str(), pParent->Value())){
						if(false == regex_search(pParent->FirstChild()->Value(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n",
														exp.str().c_str(), pParent->FirstChild()->Value());

							continue;
						}
						switch((*it).first){
							case FIELD_EXTCODE:{
								up._appendid = what.str();

								break;

							}
							#if 0
							case FIELD_DESTCODE:{
								up._appendid = what.str();

								break;

							}
							#endif
							case FIELD_MOBILES:{
								up._phone = what.str();

								break;

							}
							case FIELD_MSG:{
								up._content = what.str();

								break;
							}
							default:{

							}
						}
					}
				}
			}
			#endif

            break;

 	   case TiXmlNode::TINYXML_COMMENT:
            break;

	   case TiXmlNode::TINYXML_UNKNOWN:
            break;

 	   case TiXmlNode::TINYXML_TEXT:{
	   		pText = pParent->ToText();
            break;
 	   	}
		case TiXmlNode::TINYXML_DECLARATION :
			break;

		default:
			break;
    }

    for(pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()){
    	parseXml_MO(pChild, pattern, up);
    }
}

void parseJson_RT(Json::Value &value, field_pattern &pattern, std::vector<smsp::StateReport>& reportRes){
	std::vector<std::string> mem = value.getMemberNames();
	smsp::StateReport report;

	for (std::vector<std::string>::iterator it = mem.begin(); it != mem.end(); it++){
		switch(value[*it].type()){
			case Json::objectValue:{
				//printf("Json::objectValue\n");

				break;
			}
			case Json::stringValue:
			case Json::intValue:
				//printf("Json::stringValue\n");
				for(field_pattern::iterator it2 = pattern.begin(); it2 != pattern.end(); ++it2){
					regex exp(get<1>((*it2).second));
					cmatch what;

					if(0 == strcmp(get<0>((*it2).second).c_str(), (*it).c_str())){
						std::string data = AS_STRING(value[*it]);
						if(false == regex_search(data.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n",
														exp.str().c_str(), data.c_str());

							continue;
						}
						printf("data: %s, what: %s, exp: %s\n",
											data.c_str(), what.str().c_str(), exp.str().c_str());

						switch((*it2).first){
							case FIELD_STATUS:{
								cmatch w;
								regex reg(get<2>((*it2).second));

								//printf("status: %s, ok key: %s\n", what.str().c_str(), get<2>((*it).second).c_str());
								if(false == regex_match(what.str().c_str(), w, reg)){
									printf("regex_match err, expression: %s, data: %s\n",
																		reg.str().c_str(), what.str().c_str());
									report._status = SMS_STATUS_CONFIRM_FAIL;
								}
								else{
									report._status = SMS_STATUS_CONFIRM_SUCCESS;
								}
								report._desc = what.str();

								break;
							}
							case FIELD_SMSID:{
								report._smsId = what.str();

								break;
							}
							case FIELD_MOBILES:{
								report._phone = what.str();

								break;
							}

							default:{

							}
						}

					}
				}

				break;

			case Json::arrayValue:{
				for(int i = 0; i < (int)value[*it].size(); i++){
					parseJson_RT(value[*it][i], pattern, reportRes);
				}

				break;
			}
			default:{

			}
		}
	}
	if(0 != report._status && "" != report._smsId){
		report._reportTime = time(NULL);
		reportRes.push_back(report);
	}
}

void parseJson_MO(Json::Value &value, field_pattern &pattern, std::vector<smsp::UpstreamSms>& upsmsRes){
	std::vector<std::string> mem = value.getMemberNames();
	smsp::UpstreamSms up;

	for (std::vector<std::string>::iterator it = mem.begin(); it != mem.end(); it++){
		switch(value[*it].type()){
			case Json::objectValue:{
				//printf("Json::objectValue\n");

				break;
			}
			case Json::stringValue:
			case Json::intValue:
				//printf("Json::stringValue\n");
				for(field_pattern::iterator it2 = pattern.begin(); it2 != pattern.end(); ++it2){
					regex exp(get<1>((*it2).second));
					cmatch what;

					if(0 == strcmp(get<0>((*it2).second).c_str(), (*it).c_str())){
						std::string data = AS_STRING(value[*it]);
						if(false == regex_search(data.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n",
														exp.str().c_str(), data.c_str());

							continue;
						}
						printf("data: %s, what: %s, exp: %s\n",
											data.c_str(), what.str().c_str(), exp.str().c_str());

						switch((*it2).first){
							case FIELD_EXTCODE:{
								up._appendid = what.str();

								break;
							}
							case FIELD_MOBILES:{
								up._phone = what.str();

								break;
							}
							case FIELD_MSG:{
								up._content = what.str();

								break;

							}
							default:{

							}
						}

					}
				}

				break;

			case Json::arrayValue:{
				for(int i = 0; i < (int)value[*it].size(); i++){
					parseJson_MO(value[*it][i], pattern, upsmsRes);
				}

				break;
			}
			default:{

			}
		}
	}
	if("" != up._phone && "" != up._content){
		up._upsmsTimeInt = time(NULL);

		char szreachtime[64] = { 0 };
		struct tm pTime = {0};
		localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
		strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
		up._upsmsTimeStr =	std::string(szreachtime);
		upsmsRes.push_back(up);
	}
}

UniversalChannel::UniversalChannel(){

}

UniversalChannel::~UniversalChannel() {

}

#if 1
bool UniversalChannel::Init(const std::string config_path){
	if("" == config_path){
		printf("invalid config path\n");

		return false;
	}
	TiXmlDocument myDocument;
	bool rtn = true;

	try{
		myDocument.LoadFile(config_path.c_str());
		TiXmlElement *element = myDocument.RootElement();
		if(NULL == element){
			printf("xml parse err\n");

			rtn = false;
			goto END;
		}
		mt_rsp_field_pattern.clear();
		rt_field_pattern.clear();
		mo_field_pattern.clear();

		do{
			//printf("Element Value: %s, GetText: %s\n", element->Value(), element->GetText());
			if(0 == strcmp(element->Value(), XML_ELEMENT_MT_RSP_FIELD_PATTERN)){
				TiXmlElement *field = element->FirstChildElement();

				for(; field != NULL; field  = field->NextSiblingElement()){
					if(0 == strcmp(field->Value(), "status")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(3 <= patterns.size()){
							mt_rsp_field_pattern.insert(make_pair(FIELD_STATUS,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], patterns[2])));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else if(0 == strcmp(field->Value(), "smsid")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							mt_rsp_field_pattern.insert(make_pair(FIELD_SMSID,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					#if 0
					else if(0 == strcmp(field->Value(), "errmsg")){
						printf("%s: %s\n", field->Value(), field->GetText());

						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));
						printf("patterns size: %d\n", patterns.size());
						if(2 <= patterns.size()){
							#if 0
							mt_rsp_field_pattern.insert(make_pair(FIELD_ERRMSG,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], patterns[2])));
							#endif
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());

						}
					}
					#endif
					else{
						printf("unknow element: %s\n", field->Value());
					}
				}
			}
			else if(0 == strcmp(element->Value(), XML_ELEMENT_RT_FIELD_PATTERN)){
				TiXmlElement *field = element->FirstChildElement();

				for(; field != NULL; field  = field ->NextSiblingElement()){
					if(0 == strcmp(field->Value(), "status")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(3 <= patterns.size()){
							rt_field_pattern.insert(make_pair(FIELD_STATUS,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], patterns[2])));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else if(0 == strcmp(field->Value(), "smsid")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							rt_field_pattern.insert(make_pair(FIELD_SMSID,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else if(0 == strcmp(field->Value(), "mobiles")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							rt_field_pattern.insert(make_pair(FIELD_MOBILES,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else{
						printf("unknow element: %s\n", field->Value());
					}
				}
			}
			else if(0 == strcmp(element->Value(), XML_ELEMENT_MO_FIELD_PATTERN)){
				TiXmlElement *field = element->FirstChildElement();

				for(; field != NULL; field  = field ->NextSiblingElement()){
					if(0 == strcmp(field->Value(), "mobiles")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							mo_field_pattern.insert(make_pair(FIELD_MOBILES,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}

					}
					else if(0 == strcmp(field->Value(), "extcode")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							mo_field_pattern.insert(make_pair(FIELD_EXTCODE,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else if(0 == strcmp(field->Value(), "message")){
						//printf("%s: %s\n", field->Value(), field->GetText());
						std::vector<std::string> patterns;
						std::string xml_text = (NULL != field->GetText() ? field->GetText() : "");
						boost::split(patterns, xml_text, boost::is_any_of(";"));

						if(2 <= patterns.size()){
							mo_field_pattern.insert(make_pair(FIELD_MSG,
								Triplet<std::string, std::string, std::string>(patterns[0], patterns[1], "")));
						}
						else{
							printf("invalid xml text: %s\n", xml_text.c_str());
							rtn = false;
							goto END;
						}
					}
					else{
						printf("unknow element: %s\n", field->Value());
						rtn = false;
						goto END;
					}
				}
			}
			else if(0 == strcmp(element->Value(), XML_ELEMENT_ENCAP_FMT)){
				TiXmlElement *field = element->FirstChildElement();

				for(; field != NULL; field  = field ->NextSiblingElement()){
					if(0 == strcmp(field->Value(), "mt_rsp")){
						mt_rsp_encap_fmt = (NULL != field->GetText() ? atoi(field->GetText()) : 0);
						printf("mt_rsp_encap_fmt: %d\n", mt_rsp_encap_fmt);
					}
					else if(0 == strcmp(field->Value(), "rt_mo")){
						rt_mo_encap_fmt = (NULL != field->GetText() ? atoi(field->GetText()) : 0);
						printf("rt_mo_encap_fmt : %d\n", rt_mo_encap_fmt);
					}
					else{
						printf("unknow element: %s\n", field->Value());
						rtn = false;
						goto END;
					}
				}
			}
			else if(0 == strcmp(element->Value(), XML_ELEMENT_RT_MO_SEGMENT_TAG)){
				TiXmlElement *field = element->FirstChildElement();

				for(; field != NULL; field  = field ->NextSiblingElement()){
					if(0 == strcmp(field->Value(), "rt")){
						rt_segment_tag = (NULL != field->GetText() ? field->GetText() : "");
						printf("rt_segment_tag: %s\n", rt_segment_tag.c_str());
					}
					else if(0 == strcmp(field->Value(), "mo")){
						mo_segment_tag = (NULL != field->GetText() ? field->GetText() : "");
						printf("mo_segment_tag : %s\n", mo_segment_tag.c_str());
					}
					else{
						printf("unknow element: %s\n", field->Value());
						rtn = false;
						goto END;
					}
				}
			}
			else {
				printf("unknow element: %s\n", element->Value());
				rtn = false;
				goto END;
			}
			element = element->NextSiblingElement();
		}while(NULL != element);

	}
	catch(...){
		printf("fatal error\n");

		rtn = false;
		goto END;
	}

END:
	//myDocument.clear();
	return rtn;
}
#endif

bool UniversalChannel::parseSend(std::string respone, std::string& smsid, std::string& status, std::string& strReason){
	printf("on %s::%s:%d, respone: %s\n", __FILE__, __FUNCTION__, __LINE__, respone.c_str());

	/*
		CH: command=MT_RESPONSE&spid=M10015&mtstat=ACCEPTD&status=0&msgid=3358123

		HL: OK;6711aaf-2a0a-4c58-9327-77b2ee6
	*/
	try{
    	if ((0 >= mt_rsp_field_pattern.size()
				&& 0 >= rt_field_pattern.size()
				&& 0 >= mo_field_pattern.size())
			|| respone.empty()){
			printf("invalid argument or has not initialized\n");

			return false;
		}
		switch(mt_rsp_encap_fmt){
			case ENCAP_FORMAT_XML:{
				TiXmlDocument myDocument;

				myDocument.Parse(respone.data(), 0, TIXML_DEFAULT_ENCODING);
				TiXmlElement *element = myDocument.RootElement();
				if(NULL == element){
					printf("xml parse err\n");

					return false;
				}

				/* traverse all element node */
				parseXml_MT(element, mt_rsp_field_pattern, smsid, status, strReason);

				break;
			}
			case ENCAP_FORMAT_JSON:{
				Json::Reader reader(Json::Features::strictMode());
				std::string js;
				Json::Value root;
				Json::Value obj;

	            if (json_format_string(respone, js) < 0)
	            {
					printf("json_format_string err\n");

					return false;
	            }
	            if(!reader.parse(js,root)){
					printf("json parse err\n");

					return false;
	            }
				printf("root size: %d\n", root.size());
				std::vector<std::string> mem = root.getMemberNames();
				for (std::vector<std::string>::iterator it = mem.begin(); it != mem.end(); it++){
					printf("%s\n", AS_STRING(root[*it]).c_str());

					for(field_pattern::iterator it2 = mt_rsp_field_pattern.begin(); it2 != mt_rsp_field_pattern.end(); ++it2){
						regex exp(get<1>((*it2).second));
						cmatch what;

						if(0 == strcmp(get<0>((*it2).second).c_str(), (*it).c_str())){
							std::string data = AS_STRING(root[*it]);
							if(false == regex_search(data.c_str(), what, exp)){
								printf("regex_search err, exp: %s, data: %s\n",
															exp.str().c_str(), AS_STRING(root[*it]).c_str());

								continue;
							}
							printf("data: %s, what: %s, exp: %s\n",
												data.c_str(), what.str().c_str(), exp.str().c_str());
							switch((*it2).first){
								case FIELD_STATUS:{
									cmatch w;
									//regex reg(get<2>((*it2).second).c_str());
									regex reg(get<2>((*it2).second));

									//printf("status: %s, ok key: %s\n", what.str().c_str(), get<2>((*it).second).c_str());
									if(false == regex_match(what.str().c_str(), w, reg)){
										printf("regex_match err, expression: %s, data: %s\n",
																			reg.str().c_str(), what.str().c_str());
										status = what.str();
									}
									else{
										status = "0";		// it is represented success at outside logical
									}
									strReason = what.str();

									break;
								}
								case FIELD_SMSID:
									smsid = what.str();

									break;
								#if 0
								case FIELD_ERRMSG:{
									strReason = what.str();

									break;
								}
								#endif

								default:{

								}
							}

						}
					}
				}

				break;
			}
			case ENCAP_FORMAT_URLENCODED:{
				std::vector<std::string> vecSegTag;
				boost::split(vecSegTag, respone , boost::is_any_of("&"));

				for(field_pattern::iterator it = mt_rsp_field_pattern.begin(); it != mt_rsp_field_pattern.end(); ++it){
					regex exp("(?<=" + get<0>((*it).second) + "=)" + get<1>((*it).second));
					cmatch what;

					if(false == regex_search(respone.c_str(), what, exp)){
						printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

						continue;
					}

					switch((*it).first){
						case FIELD_SMSID:{
							//printf("SMSID: %s\n", root[(*it).second.first].asString().c_str());
							smsid = what.str();

							break;
						}
						#if 0
						case FIELD_ERRMSG:{
							//printf("ERRMSG: %s\n", root[(*it).second.first].asString().c_str());
							strReason = what.str();

							break;
						}
						#endif
						case FIELD_STATUS:{
							//printf("CODE: %d\n", root[(*it).second.first].asInt());
							status = what.str();
							strReason = what.str();

							break;
						}
						default:{

						}
					}
				}

				break;
			}
			case ENCAP_FORMAT_CUSTOM:{

				for(field_pattern::iterator it = mt_rsp_field_pattern.begin(); it != mt_rsp_field_pattern.end(); ++it){
					regex exp(get<1>((*it).second));
					cmatch what;

					if(false == regex_search(respone.c_str(), what, exp)){
						printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

						continue;
					}
					switch((*it).first){
						case FIELD_STATUS:{
							cmatch w;
							regex reg(get<2>((*it).second).c_str());

							//printf("status: %s, ok key: %s\n", what.str().c_str(), get<2>((*it).second).c_str());
							if(false == regex_match(what.str().c_str(), w, reg)){
								printf("regex_match err, expression: %s\n", reg.str().c_str());
								status = what.str();
							}
							else{
								//printf("regex_match suc, expression: %s\n", get<2>((*it).second).c_str());
								status = "0";		// it is represented success at outside logical
							}
							strReason = what.str();

							break;
						}
						case FIELD_SMSID:
							smsid = what.str();

							break;
						#if 0
						case FIELD_ERRMSG:{
							strReason = what.str();

							break;
						}
						#endif

						default:{

						}
					}

				}

				break;
			}

			default:{

			}
		}
    }
    catch(...){
		printf("fatal error\n");

        return false;
    }

	return true;
}

bool UniversalChannel::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) {
	printf("on %s::%s:%d, respone: %s\n", __FILE__, __FUNCTION__, __LINE__, respone.c_str());

	/*
		状态报告: command=RT_RESPONSE&spid=SP0004&msgid=123456&sa=13512345678&rterrcode=000
			    phone=15920532399&taskid=3607085-f50e-4761-bba9-d375d1b&mr=DELIVRD

		上行: command=MO_RESPONSE&spid=SP0003&moinfo=159****,C4E3BAC3B5C4
			phone=15920532399&message=this is a test
	*/
	try{
		if ((0 >= mt_rsp_field_pattern.size()
				&& 0 >= rt_field_pattern.size()
				&& 0 >= mo_field_pattern.size())
			|| respone.empty()){
			printf("invalid argument or has not initialized\n");

			return false;
		}

		unsigned char flag = 0;
		unsigned char mask_rt = 1;	//		0 | 0 | 0 | 0 | 0 | 0 | 0 | 1
		unsigned char mask_mo = 2;	//		0 | 0 | 0 | 0 | 0 | 0 | 1 | 0

		/*
		*	check the RT/MO type
		*/
		printf("rt_field_pattern size: %d, mo_field_pattern size: %d\n", (int)rt_field_pattern.size(), (int)mo_field_pattern.size());
		/* RT field detection */
		field_pattern::iterator it = rt_field_pattern.begin();
		do{
			if(rt_field_pattern.end() == it)
				break;

			if(string::npos == respone.find(get<0>((*it).second))){
				printf("RT field: %s dose not exit\n", get<0>((*it).second).c_str());

				break;
			}
			++it;

			if(it == rt_field_pattern.end()){
				flag = flag | mask_rt;
			}
		}while(it != rt_field_pattern.end());

		/* MO field detection */
		it = mo_field_pattern.begin();
		do{
			if(mo_field_pattern.end() == it)
				break;

			if(string::npos == respone.find(get<0>((*it).second))){
				printf("MO field: %s dose not exit\n", get<0>((*it).second).c_str());

				break;
			}
			++it;

			if(it == mo_field_pattern.end()){
				flag = flag | mask_mo;
			}
		}while(it != mo_field_pattern.end());

		if(0 >= flag){
			printf("cat not recognize the RT/MO msg: %s, flag: %d\n", respone.c_str(), flag);

			return false;
		}

		/* Report */
		switch(rt_mo_encap_fmt){
			case ENCAP_FORMAT_XML:{
				TiXmlDocument myDocument;

				myDocument.Parse(respone.data(), 0, TIXML_DEFAULT_ENCODING);
				TiXmlElement *element = myDocument.RootElement();
				if(NULL == element){
					printf("xml parse err\n");

					return false;
				}

				/* traverse all element node */
				std::vector<TiXmlNode *> nodes;

				/* RT */
				if(flag & mask_rt){
					getNodesByTag(element, rt_segment_tag, nodes);
					printf("report nodes size: %d\n", (int)nodes.size());

					for(std::vector<TiXmlNode *>::iterator it = nodes.begin(); it != nodes.end(); it++){
						smsp::StateReport report;

						parseXml_RT(*it, rt_field_pattern, report);
						report._reportTime = time(NULL);
						reportRes.push_back(report);
					}
				}
				else if(flag & mask_mo){/* MO */
					getNodesByTag(element, mo_segment_tag, nodes);
					printf("upstream nodes size: %d\n", (int)nodes.size());

					for(std::vector<TiXmlNode *>::iterator it = nodes.begin(); it != nodes.end(); it++){
						smsp::UpstreamSms up;

						parseXml_MO(*it, mo_field_pattern, up);
						up._upsmsTimeInt = time(NULL);

						char szreachtime[64] = { 0 };
						struct tm pTime = {0};
						localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
						strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
						up._upsmsTimeStr =	std::string(szreachtime);
						upsmsRes.push_back(up);
					}
				}
				else{
					printf("internal err\n");
				}

				break;
			}
			case ENCAP_FORMAT_JSON:{
				Json::Reader reader(Json::Features::strictMode());
				std::string js;
				Json::Value root;

	            if (json_format_string(respone, js) < 0)
	            {
					printf("json_format_string err\n");
					return false;
	            }
	            if(!reader.parse(js,root)){
					printf("json parse err\n");

					return false;
	            }
				printf("json field size: %d\n", root.size());
				std::vector<std::string> mem = root.getMemberNames();

				if(flag & mask_rt){
					printf("parsing rt...");
					parseJson_RT(root, rt_field_pattern, reportRes);
				}
				else if(flag & mask_mo){/* MO */
					printf("parsing mo...");
					parseJson_MO(root, mo_field_pattern, upsmsRes);
				}
				else{
					printf("internal err\n");
				}

				break;
			}
			case ENCAP_FORMAT_URLENCODED:{
				std::vector<std::string> vecSegTag;
				boost::split(vecSegTag, respone , boost::is_any_of("&"));

				/* RT */
				if(flag & mask_rt){
					smsp::StateReport report;

					for(field_pattern::iterator it = rt_field_pattern.begin(); it != rt_field_pattern.end(); ++it){
						regex exp("(?<=" + get<0>((*it).second) + "=)" + get<1>((*it).second));
						cmatch what;

						if(false == regex_search(respone.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

							continue;
						}
						//printf("got: %s, exp: %s\n", what.str().c_str(), exp.str().c_str());

						switch((*it).first){
							case FIELD_SMSID:{
								report._smsId = what.str();

								break;
							}
							#if 0
							case FIELD_ERRMSG:{
								report._desc = what.str();

								break;
							}
							#endif
							case FIELD_STATUS:{
								cmatch w;
								regex reg(get<2>((*it).second).c_str());

								printf("ok key: %s\n", get<2>((*it).second).c_str());
								if(false == regex_match(what.str().c_str(), w, reg)){
									printf("regex_match err, expression: %s\n", reg.str().c_str());
									report._status = SMS_STATUS_CONFIRM_FAIL;

								}
								else{
									printf("regex_match suc, expression: %s\n", get<2>((*it).second).c_str());
									report._status = SMS_STATUS_CONFIRM_SUCCESS;
								}
								report._desc = what.str();

								break;
							}
							case FIELD_MOBILES:{
								report._phone = what.str();

								break;

							}
							default:{

							}
						}



					}
					report._reportTime = time(NULL);
					reportRes.push_back(report);
				}
				else if(flag & mask_mo){/* MO */
					smsp::UpstreamSms up;

					for(field_pattern::iterator it = mo_field_pattern.begin(); it != mo_field_pattern.end(); ++it){
						regex exp("(?<=" + get<0>((*it).second) + "=)" + get<1>((*it).second));
						cmatch what;

						if(false == regex_search(respone.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

							continue;
						}
						//printf("got: %s, exp: %s\n", what.str().c_str(), exp.str().c_str());

						switch((*it).first){
							case FIELD_MOBILES:{
								up._phone = what.str();

								break;

							}
							case FIELD_MSG:{
								up._content = what.str();

								break;
							}
							default:{

							}
						}
					}
					up._upsmsTimeInt = time(NULL);

					char szreachtime[64] = { 0 };
					struct tm pTime = {0};
					localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
					strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
					up._upsmsTimeStr =	std::string(szreachtime);
					upsmsRes.push_back(up);
				}
				else{
					printf("internal err, %s\n", respone.c_str());

				}
				break;
			}
			case ENCAP_FORMAT_CUSTOM:{
				/* RT */
				if(flag & mask_rt){
					smsp::StateReport report;

					for(field_pattern::iterator it = rt_field_pattern.begin(); it != rt_field_pattern.end(); ++it){
						regex exp(get<1>((*it).second));
						cmatch what;

						if(false == regex_search(respone.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

							continue;
						}

						switch((*it).first){

							case FIELD_SMSID:{
								report._smsId = what.str();

								break;
							}
							#if 0
							case FIELD_ERRMSG:{
								report._desc = what.str();

								break;
							}
							#endif
							case FIELD_STATUS:{
								cmatch w;
								regex reg(get<2>((*it).second).c_str());

								printf("ok key: %s\n", get<2>((*it).second).c_str());
								if(false == regex_match(what.str().c_str(), w, reg)){
									printf("regex_match err, expression: %s\n", reg.str().c_str());
									report._status = SMS_STATUS_CONFIRM_FAIL;

								}
								else{
									printf("regex_match suc, expression: %s\n", get<2>((*it).second).c_str());
									report._status = SMS_STATUS_CONFIRM_SUCCESS;
								}
								report._desc = what.str();

								break;
							}
							case FIELD_MOBILES:{
								report._phone = what.str();

								break;

							}
							default:{

							}
						}

					}
					report._reportTime = time(NULL);
					reportRes.push_back(report);
				}
				else if(flag & mask_mo){/* MO */
					smsp::UpstreamSms up;

					for(field_pattern::iterator it = mo_field_pattern.begin(); it != mo_field_pattern.end(); ++it){
						regex exp(get<1>((*it).second));
						cmatch what;

						if(false == regex_search(respone.c_str(), what, exp)){
							printf("regex_search err, exp: %s, data: %s\n", exp.str().c_str(), respone.c_str());

							continue;
						}
						//printf("got: %s, exp: %s\n", what.str().c_str(), exp.str().c_str());

						switch((*it).first){
							case FIELD_DESTCODE:{
								up._appendid = what.str();

								break;

							}
							case FIELD_MOBILES:{
								up._phone = what.str();

								break;

							}
							case FIELD_MSG:{
								up._content = what.str();

								break;
							}
							default:{

							}
						}
					}
					up._upsmsTimeInt = time(NULL);

					char szreachtime[64] = { 0 };
					struct tm pTime = {0};
					localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
					strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
					up._upsmsTimeStr =	std::string(szreachtime);
					upsmsRes.push_back(up);
				}
				else{
					printf("internal err\n", respone.c_str());
				}

				break;
			}

			default:{

			}
		}
	}
	catch(...){
		printf("fatal error\n");

		return false;
	}

	return true;
}

int UniversalChannel::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){
	printf("on %s::%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if ((0 >= mt_rsp_field_pattern.size()
			&& 0 >= rt_field_pattern.size()
			&& 0 >= mo_field_pattern.size())){
		printf("invalid argument or has not initialized\n");

		return -1;
	}
	//LogInfo("");

	//if(NULL == sms){return -1;}
	//replace_all_distinct(sms->m_strContent, "%", "");

	return 0;
}

} /* namespace smsp */
