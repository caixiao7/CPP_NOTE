#include "stdafx.h"
#include "PostJson.h"
#include "curl.h"
#include <iostream>
#include <sstream>
#include "../JsonMessage.h"
#include "../Global.h"
#include "../XMaster.h"
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "wldap32.lib")
#pragma comment (lib, "Shell32.lib")
#pragma comment (lib, "Shlwapi.lib")

using namespace std;

CPostJson::CPostJson()
{
}


CPostJson::~CPostJson()
{
}

wstring AsciiToUnicode(const string& str)
{
	// 预算-缓冲区中宽字节的长度  
	int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	// 给指向缓冲区的指针变量分配内存  
	wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
	// 开始向缓冲区转换字节  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
	wstring ret_str = pUnicode;
	free(pUnicode);
	return ret_str;
}

string UnicodeToUtf8(const wstring& wstr)
{
	// 预算-缓冲区中多字节的长度  
	int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	// 给指向缓冲区的指针变量分配内存  
	char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
	// 开始向缓冲区转换字节  
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
	string ret_str = pAssii;
	free(pAssii);
	return ret_str;
}

std::string AsciiToUtf8(const std::string& str)
{
	return UnicodeToUtf8(AsciiToUnicode(str));
}


size_t http_data_writer(void* data, size_t size, size_t nmemb, void* content)
{
	long totalSize = size*nmemb;
	std::string* symbolBuffer = (std::string*)content;
	if (symbolBuffer)
	{
		symbolBuffer->append((char *)data, ((char*)data) + totalSize);
	}
	return totalSize;
}

//回调函数
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	string data((const char*)ptr, (size_t)size * nmemb);

	*((stringstream*)stream) << data << endl;

	return size * nmemb;
}

bool CPostJson::sendJson(CString rfid,CString start_time,CString end_time,CString cnc_name,int status, CString program, CString str_SQL_start_time)
{
	CURL *curl;
	CURLcode res;
	char szJsonData[1024];
	//HTTP报文头  
	struct curl_slist* headers = NULL;

	/* = "http://cnc.cck.corp/Features/Api/PostProcedureReocrd.aspx"*/
	char *url;
	url = new char[X_APP_INSTANCE->strRecordUrl.GetLength()+1];
	strcpy(url, cstringTostring(X_APP_INSTANCE->strRecordUrl.GetString()).c_str());

	curl = curl_easy_init();
	CString tmp;
	if (curl)
	{
		//string类型的json串
		memset(szJsonData, 0, sizeof(szJsonData));
		std::string strJson = "{\"Rfid\":\"1234664\"}";
		strcpy(szJsonData, strJson.c_str());

		strJson = AsciiToUtf8(strJson);//如果json串中包含有中文,必须进行转码

		//tmp_used_tool[tool_qrcode]=tool_name
		std::map<CString,CString> tmp_used_tool;
		json1( rfid, start_time, end_time, cnc_name, status, program, tmp, tmp_used_tool);
		if (!partUsedTool(rfid, str_SQL_start_time, tmp_used_tool))
		{
			CString strLog;
			strLog.Format(_T("warn:记录工件%s使用刀具异常"), rfid);
			XM_LOG(strLog);
		}
		//tmp.Format(_T("{\"Rfid\":\"%s\",\"Station\":\"%s\",\"StartTime\":\"%s\",\"EndTime\":\"%s\",\"Status\":\"%s\"}"),rfid,cnc_name,start_time,end_time,status);
		std::wstring wstrJson;
		wstrJson = tmp;
		string testJson = UnicodeToUtf8(wstrJson);

		std::stringstream out;

		//设置url
		curl_easy_setopt(curl, CURLOPT_URL, url);
		// 设置http发送的内容类型为JSON

		//构建HTTP报文头  

		headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		//curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST
												// 设置要POST的JSON数据

		//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, testJson.c_str());//以多字节上传
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, testJson.c_str());//以Unicode编码上传
																	 //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strJson.size());
																	 // 设置接收数据的处理函数和存放变量
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置回调函数
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);//设置写数据

		res = curl_easy_perform(curl);
		curl_slist_free_all(headers); /* free the list again */
		delete[]url;
		if (res == CURLcode::CURLE_OK)
		{
			long responseCode = 0;
			std::string str_json = out.str();//返回值 例如:{"status":"ok"}  
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			if (responseCode < 200 || responseCode >= 300 || str_json.empty())
			{
				ERPBufferPool(1, tmp);
				curl_easy_cleanup(curl);
				XM_LOG("warn:ERP11通信失败");
				return false;
			}
			curl_easy_cleanup(curl);
			//下面可以对应答的数据进行处理了
			// strData
			TCHAR w_str[2048];
			ZeroMemory(w_str, 2048);
			TCHAR * tmp = w_str;
			std::string temp;
			UTF_8ToGB2312(tmp, str_json);
			CString cstrData = CString(tmp);
			char c[2048];
			temp = cstringTostring(cstrData);
			strcpy(c, temp.c_str());
			/*
			int pos1 = -1, pos2 = -1;
			pos1 = cstrData.Find(_T("Status"));
			if (pos1 != -1)
			{
				for (int i = 0; i < 2; i++)
					pos1 = cstrData.Find(_T("\""), pos1 + 1);
				pos2 = cstrData.Find(_T("\""), pos1 + 1);
				CString status = cstrData.Mid(pos1 + 1, pos2 - pos1 - 1);
				if (status == _T("Completed"))
				{
					return true;
				}
			}*/
			CJsonMessage json_msg;
			json_msg.Clear();
			if (!json_msg.Parse((unsigned char*)c, 1024))
			{
				//收到JSON数据,解析合法性
				XM_LOG("warn:JSON异常");
				return false;
			}
			std::string str_tmp;
			if (!json_msg.GetPara("Status", str_tmp))
			{
				XM_LOG("warn:返回状态异常");
				return false;
			}
			if (str_tmp == "Completed")
			{
				return true;
			}
			else
			{
				if (json_msg.GetPara("ErrorMessage", str_tmp))
				{
					XM_LOG("warn:"+str_tmp);
					return false;
				}
				XM_LOG("warn:返回状态异常");
				return false;
			}

		}
	}
	ERPBufferPool(1, tmp);
	XM_LOG("warn:ERP12通信失败");
	return false;
}

bool CPostJson::sendData(string rfid,CString& partNo, CString& orderNo,int& partType, CString& program, CString& cnc)
{
	CURL* curl = NULL;
	curl = curl_easy_init();
	CURLcode code;
	if (curl)
	{
		// 设置Url = "http://cnc.cck.corp/Features/Api/GetProcedurePlan.aspx"
		char *url;
		url = new char[X_APP_INSTANCE->strPlanUrl.GetLength()+1];
		strcpy(url, cstringTostring(X_APP_INSTANCE->strPlanUrl.GetString()).c_str());

		code = curl_easy_setopt(curl, CURLOPT_URL, url);
		// 设置post的json数据
		char szPost[64] = { 0x00 };
		sprintf_s(szPost, 64, "Rfid=%s", rfid.c_str());
		code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szPost);
		// 设置回调函数
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_data_writer);
		//设置写数据
		std::string strData;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strData);
		// 执行请求
		code = curl_easy_perform(curl);
		delete[]url;
		if (code == CURLcode::CURLE_OK)
		{
			long responseCode = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			if (responseCode < 200 || responseCode >= 300 || strData.empty())
			{
				CString tmp;
				tmp.Format(_T("Rfid=%s"), stringToCString(rfid));
				ERPBufferPool(2, tmp);
				curl_easy_cleanup(curl);
				XM_LOG("warn:ERP21通信失败");
				return false;
			}
			//下面可以对应答的数据进行处理了
			// strData
			// 清除curl对象
			curl_easy_cleanup(curl);

			/*TCHAR w_str[1024];
			ZeroMemory(w_str, 1024);
			TCHAR * tmp = w_str;
			std::string temp;
			UTF_8ToGB2312(tmp, strData);
			CString cstrData = CString(tmp);
			int pos1=-1, pos2=-1;
			pos1=cstrData.Find(_T("Status"));
			if (pos1 != -1)
			{
				for (int i = 0; i < 2; i++)
					pos1 = cstrData.Find(_T("\""), pos1 + 1);
				pos2 = cstrData.Find(_T("\""), pos1 + 1);
				CString status = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
				if (status == _T("Completed"))
				{
					pos1 = cstrData.Find(_T("PartNo"));
					if (pos1 != -1)
					{
						pos1 = cstrData.Find(_T(":"), pos1 + 1);
						pos2 = cstrData.Find(_T(","), pos1 + 1);
						partNo = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
					}
					else 
						return false;
					pos1 = cstrData.Find(_T("WorkOrderNo"));
					if (pos1 != -1)
					{
						for (int i = 0; i < 2; i++)
							pos1 = cstrData.Find(_T("\""), pos1 + 1);
						pos2 = cstrData.Find(_T("\""), pos1 + 1);
						orderNo = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
					}
					else
						return false;
					pos1 = cstrData.Find(_T("WorkType"));
					if (pos1 != -1)
					{
						pos1 = cstrData.Find(_T(":"), pos1 + 1);
						pos2 = cstrData.Find(_T(","), pos1 + 1);
						CString strPartType = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
						partType = 0;
						if (strPartType == _T("1"))
							partType = 1;
						else if (strPartType == _T("2"))
							partType = 2;
					}
					else
						return false;
					pos1 = cstrData.Find(_T("ProgramFile"));
					if (pos1 != -1)
					{
						for (int i = 0; i < 2; i++)
							pos1 = cstrData.Find(_T("\""), pos1 + 1);
						pos2 = cstrData.Find(_T("\""), pos1 + 1);
						program = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
					}
					else
						return false;
					pos1 = cstrData.Find(_T("Station"));
					if (pos1 != -1)
					{
						for (int i = 0; i < 2; i++)
							pos1 = cstrData.Find(_T("\""), pos1 + 1);
						pos2 = cstrData.Find(_T("\""), pos1 + 1);
						cnc = cstrData.Mid(pos1 + 1, pos2 - pos1-1);
					}
					return true;
				}
				else
				{
					return false;
				}*/

			TCHAR w_str[1024];
			ZeroMemory(w_str, 1024);
			TCHAR * tmp = w_str;
			std::string temp;
			UTF_8ToGB2312(tmp, strData);
			CString cstrData = CString(tmp);
			char c[1024];
			temp = cstringTostring(cstrData);
			strcpy(c, temp.c_str());

			CJsonMessage json_msg;
			json_msg.Clear();
			if (!json_msg.Parse((unsigned char*)c, 1024))
			{
				//收到JSON数据,解析合法性
				XM_LOG("warn:JSON异常");
				return false;
			}
			std::string str_tmp;
			int int_tmp = 0;
			if (!json_msg.GetPara("Status", str_tmp))
			{
				XM_LOG("warn:返回状态异常");
				return false;
			}
			if (str_tmp != "Completed")
			{
				if (json_msg.GetPara("ErrorMessage", str_tmp))
				{
					XM_LOG("warn:"+str_tmp);
					return false;
				}
				XM_LOG("warn:返回状态异常");
				return false;
			}
			if (!json_msg.GetPara("Rfid", str_tmp))
			{
				XM_LOG("rfid返回错误");
				return false;
			}
			if (str_tmp != rfid)
			{
				XM_LOG("返回rfid不匹配");
				return false;
			}

			if (!json_msg.GetPara("PartNo", int_tmp))
			{
				XM_LOG("warn:返回工件编号异常");
				return false;
			}
			partNo.Format(_T("%d"), int_tmp);

			if (!json_msg.GetPara("WorkOrderNo", str_tmp))
			{
				XM_LOG("warn:返回工单号异常");
				return false;
			}
			orderNo = stringToCString(str_tmp);
			
			if (!json_msg.GetPara("WorkType", int_tmp))
			{
				XM_LOG("warn:返回工件类型异常");
				return false;
			}
			if (int_tmp == MACHINE_PART_TYPE::MPT_STEEL)
				partType = int_tmp;
			else
				partType = MACHINE_PART_TYPE::MPT_PART;

			if (!json_msg.GetPara("ProgramFile", str_tmp))
			{
				XM_LOG("warn:返回加工程式异常");
				return false;
			}
			program = stringToCString(str_tmp);

			if (!json_msg.GetPara("Station", str_tmp))
			{
				//XM_LOG("返回绑定加工机异常");
				//return false;
			}
			cnc = stringToCString(str_tmp);
			return true;
		}
		
	}
	CString tmp;
	tmp.Format(_T("Rfid=%s"), stringToCString(rfid));
	ERPBufferPool(2, tmp);
	XM_LOG("warn:ERP22通信失败");
	return false;
}

bool CPostJson::sendStatus(CString rfid, CString cnc_name, int status)
{
#ifdef FAKE_TEST
	return true;
#endif
	CURL *curl;
	CURLcode res;
	char szJsonData[1024];
	//HTTP报文头  
	struct curl_slist* headers = NULL;

	/* = "http://cnc.cck.corp/Features/Api/PostProcedureStatus.aspx"*/
	char *url;
	url = new char[X_APP_INSTANCE->strStatusUrl.GetLength()+1];
	strcpy(url, cstringTostring(X_APP_INSTANCE->strStatusUrl.GetString()).c_str());

	curl = curl_easy_init();
	CString tmp;
	if (curl)
	{
		//string类型的json串
		memset(szJsonData, 0, sizeof(szJsonData));
		std::string strJson = "{\"Rfid\":\"1234664\"}";
		strcpy(szJsonData, strJson.c_str());

		strJson = AsciiToUtf8(strJson);//如果json串中包含有中文,必须进行转码

		
		json2(rfid,cnc_name, status, tmp);
		//tmp.Format(_T("{\"Rfid\":\"%s\",\"Station\":\"%s\",\"StartTime\":\"%s\",\"EndTime\":\"%s\",\"Status\":\"%s\"}"),rfid,cnc_name,start_time,end_time,status);
		std::wstring wstrJson;
		wstrJson = tmp;
		string testJson = UnicodeToUtf8(wstrJson);

		std::stringstream out;

		//设置url
		curl_easy_setopt(curl, CURLOPT_URL, url);
		// 设置http发送的内容类型为JSON

		//构建HTTP报文头  

		headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		//curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST
												// 设置要POST的JSON数据

												//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, testJson.c_str());//以多字节上传
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, testJson.c_str());//以Unicode编码上传
																	 //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strJson.size());
																	 // 设置接收数据的处理函数和存放变量
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置回调函数
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);//设置写数据

		res = curl_easy_perform(curl);
		curl_slist_free_all(headers); /* free the list again */
		delete[]url;
		if (res == CURLcode::CURLE_OK)
		{
			long responseCode = 0;
			std::string str_json = out.str();//返回值 例如:{"status":"ok"}  
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			if (responseCode < 200 || responseCode >= 300 || str_json.empty())
			{
				ERPBufferPool(3, tmp);
				curl_easy_cleanup(curl);
				XM_LOG("warn:ERP31通信失败");
				return false;
			}
			curl_easy_cleanup(curl);
			//下面可以对应答的数据进行处理了
			// strData
			TCHAR w_str[2048];
			ZeroMemory(w_str, 2048);
			TCHAR * tmp = w_str;
			std::string temp;
			UTF_8ToGB2312(tmp, str_json);
			CString cstrData = CString(tmp);
			char c[2048];
			temp = cstringTostring(cstrData);
			strcpy(c, temp.c_str());
			/*
			int pos1 = -1, pos2 = -1;
			pos1 = cstrData.Find(_T("Status"));
			if (pos1 != -1)
			{
			for (int i = 0; i < 2; i++)
			pos1 = cstrData.Find(_T("\""), pos1 + 1);
			pos2 = cstrData.Find(_T("\""), pos1 + 1);
			CString status = cstrData.Mid(pos1 + 1, pos2 - pos1 - 1);
			if (status == _T("Completed"))
			{
			return true;
			}
			}*/
			CJsonMessage json_msg;
			json_msg.Clear();
			if (!json_msg.Parse((unsigned char*)c, 1024))
			{
				//收到JSON数据,解析合法性
				XM_LOG("warn:JSON异常");
				return false;
			}
			std::string str_tmp;
			if (!json_msg.GetPara("Status", str_tmp))
			{
				XM_LOG("warn:返回状态异常");
				return false;
			}
			if (str_tmp == "Completed")
			{
				return true;
			}
			else
			{
				if (json_msg.GetPara("ErrorMessage", str_tmp))
				{
					XM_LOG("warn:" + str_tmp);
					return false;
				}
				XM_LOG("warn:返回状态异常");
				return false;
			}

		}
	}
	ERPBufferPool(3, tmp);
	XM_LOG("warn:ERP32通信失败");
	return false;
}

bool CPostJson::json1(CString rfid, CString start_time, CString end_time, CString cnc_name, int status, CString program,CString& str_json, std::map<CString, CString> &map_tool_name_qrcode)
{
	CJsonMessage json_msg;
	json_msg.AddRequest("Rfid", cstringTostring(rfid));
	json_msg.AddRequest("Status", status);
	json_msg.AddRequest("Station", cstringTostring(cnc_name));
	json_msg.AddRequest("StartTime", cstringTostring(start_time));
	json_msg.AddRequest("EndTime", cstringTostring(end_time));
	std::vector<std::string> toolJson;
	double Z = 0.0;
	getToolData(rfid, cnc_name, program, toolJson, Z, map_tool_name_qrcode);
	json_msg.AddRequest("Cutlerys", toolJson);
	json_msg.AddRequest("ZPosition", Z);

	StringBuffer sb = json_msg.OutPut();
	size_t send_len = sb.GetSize();
	str_json=stringToCString((char*)sb.GetString());
	str_json.Replace(_T("\"{"), _T("{"));
	str_json.Replace(_T("}\""), _T("}"));
	str_json.Replace(_T("\\n"), _T("\n"));
	str_json.Replace(_T("\\\""), _T("\""));
	return true;
}
bool CPostJson::json2(CString rfid, CString cnc_name, int status, CString& str_json)
{
	CJsonMessage json_msg;
	json_msg.AddRequest("Rfid", cstringTostring(rfid));
	json_msg.AddRequest("Status", status);
	json_msg.AddRequest("Station", cstringTostring(cnc_name));

	StringBuffer sb = json_msg.OutPut();
	size_t send_len = sb.GetSize();
	str_json = stringToCString((char*)sb.GetString());
	return true;
}
bool CPostJson::getToolData(CString rfid, CString cnc_name, CString program, std::vector<std::string>& toolJson, double &Z, std::map<CString, CString> &map_tool_name_qrcode)
{
	CStdioFile file;
	CString strProgram,strLog;
	strProgram.Format(_T("C:/Users/admin/Desktop/Service/CNC/cnc_data/%s/_N_TIME_SPF"), cnc_name);
	//strProgram.Format(_T("E:/xtec/%s/_N_TIME_SPF"), cnc_name);
	if (!file.Open(strProgram, CFile::modeRead))
	{
		strLog.Format(_T("warn:加工机<%s>_N_TIME_SPF无法打开"), cnc_name);
		XM_LOG(strLog);
		return false;
	}
	CString strTool;
	while (file.ReadString(strTool))
	{
		if (strTool.Find(program) == -1)
			continue;
		else
		{
			CJsonMessage json_msg;
			toolJson.clear();
			map_tool_name_qrcode.clear();
			while (file.ReadString(strTool))
			{
				int pos1 = -1, pos2 = -1;
				CString tmp;
				//0419547C0C  :  2019/3/22-18:17:50
				//Z  :  -300.7299442
				//100041 /1011 /2 /RADIUS:2.9963 /RUNOUT:0.0003 /2019/3/22-18:22:36

				if (strTool.Find(_T("END")) != -1)
					break;
				if (strTool.Find(_T("Z  :")) != -1)
				{
					pos1 = strTool.Find(_T("Z  :"));
					tmp = strTool.Mid(pos1 + 6, strTool.GetLength());
					Z= _ttof(tmp);
				}
				if (strTool.Find(_T("RADIUS")) == -1)
					continue;
				else
				{
					json_msg.Clear();

					CString tmp_qrcode;

					pos1 = strTool.Find(_T("/"));
					tmp_qrcode = strTool.Mid(0, pos1-1);

					pos2 = strTool.Find(_T("/"),pos1+1);
					tmp = strTool.Mid(pos1 + 1, pos2 - pos1 - 2);
					json_msg.AddRequest("Location0", _ttoi(tmp));

					map_tool_name_qrcode[tmp_qrcode] = tmp;

					pos1 = strTool.Find(_T("RADIUS"));
					tmp = strTool.Mid(pos2 + 1, pos1 - pos2 - 3);
					json_msg.AddRequest("Location1", _ttoi(tmp));

					pos2 = strTool.Find(_T("RUNOUT"));
					tmp = strTool.Mid(pos1 + 7, pos2 - pos1 - 9);
					json_msg.AddRequest("Radius", _ttof(tmp));

					pos1 = strTool.Find(_T("/"),pos2+7);
					tmp = strTool.Mid(pos2 + 7, pos1 - pos2 - 8);
					json_msg.AddRequest("Runout", _ttof(tmp));

					tmp = strTool.Mid(pos1 + 1, strTool.GetLength());
					tmp.Replace(_T(" "), _T(""));
					tmp.Replace(_T("-"), _T(" "));
					json_msg.AddRequest("DateTime", cstringTostring(tmp));

					StringBuffer sb = json_msg.OutPut();
					size_t send_len = sb.GetSize();
					toolJson.push_back((char*)sb.GetString());
				}

			}
		}

	}
	file.Close();
	return true;
}

void CPostJson::SendEmail(CString str_cmd)
{
	//邮件发送
	char msg[1024] = { 0x00 };
	SOCKET sock;
	WSADATA wsaData;
	SOCKADDR_IN server_addr;
	WORD wVersion;
	wVersion = MAKEWORD(2, 2);
	WSAStartup(wVersion, &wsaData);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8193);
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		XM_LOG("warn:邮件客户端创建失败");
	}
	if (connect(sock, (struct sockaddr*) &server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		XM_LOG("warn:邮件服务器连接失败");
	}

	std::string cmd = cstringTostring(str_cmd);
	if (send(sock, cmd.c_str(), strlen(cmd.c_str()), 0) == SOCKET_ERROR)
	{

	}
	int timeout = 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
	int res = 0;
	if ((res = recv(sock, msg, 1024, 0)) == -1)
	{

	}
	else
	{
		msg[res] = '\0';
	}
	closesocket(sock);
}

void CPostJson::SendWX(CString str_cmd)
{
#ifdef FAKE_TEST
	return;
#endif
	//邮件微信
	char msg[1024] = { 0x00 };
	SOCKET sock;
	WSADATA wsaData;
	SOCKADDR_IN server_addr;
	WORD wVersion;
	try
	{
		wVersion = MAKEWORD(2, 2);
		WSAStartup(wVersion, &wsaData);
		server_addr.sin_addr.s_addr = inet_addr("192.168.2.103");
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(8194);
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		{
			XM_LOG("微信客户端创建失败");
		}
		if (connect(sock, (struct sockaddr*) &server_addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		{
			XM_LOG("微信服务器连接失败");
		}

		std::string cmd = cstringTostring(str_cmd);
		if (send(sock, cmd.c_str(), strlen(cmd.c_str()), 0) == SOCKET_ERROR)
		{

		}
		int timeout = 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
		int res = 0;
		if ((res = recv(sock, msg, 1024, 0)) == -1)
		{

		}
		else
		{
			msg[res] = '\0';
		}
		closesocket(sock);
	}
	catch (CException * e)
	{
		XM_LOG("发送微信失败");
	}
}

void CPostJson::ERPBufferPool(int url_type, CString post_data)
{
	CString strSQL;
	post_data.Replace(_T("'"), _T("''"));
	strSQL.Format(_T("insert into erp_buffer(`time`,`url_type`,`data`) values(now(),%d,'%s');"), url_type,post_data);
	X_APP_INSTANCE->db_execution(strSQL);
}

bool CPostJson::partUsedTool(CString rfid, CString start_time, std::map<CString, CString> map_tool_name_qrcode)
{
	CString strSQL;
	bool ret = true;
	if (map_tool_name_qrcode.size() == 0)
		return false;
	for (auto it = map_tool_name_qrcode.begin(); it != map_tool_name_qrcode.end(); it++)
	{
		strSQL.Format(_T("insert xtec_part_used_tool (rfid,start_time,tool_name,qrcode) values('%s','%s','%s','%s');"), rfid, start_time, it->second, it->first);
		if (!X_APP_INSTANCE->db_execution(strSQL))
			ret = false;
	}
	return ret;
}