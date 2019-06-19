#pragma once
#include <vector>
#include <map>
//ERP用httpclient通信，httpclient用libcurl.lib,引用相对路径
//库源码https://10.10.10.133/svn/X-MASTER_FUCHUAN/3 Source Code/curl-7.26.0
class CPostJson
{
public:
	CPostJson();
	~CPostJson();

	//加工完成后发送加工信息到ERP
	bool sendJson(CString rfid, CString start_time, CString end_time, CString cnc_name, int status, CString program, CString str_SQL_start_time);
	//发送rfid到ERP获取工件资料
	bool sendData(std::string rfid, CString& partNo, CString& orderNo, int& partType, CString& program, CString& cnc);
	//发送工件的实时状态到ERP
	bool sendStatus(CString rfid, CString cnc_name, int status);
	//发送ERP失败时发送Email
	void SendEmail(CString str_cmd);
	//warning和error信息发送到微信
	void SendWX(CString msg);
private:
	//生成加工信息json（开始、结束时间，加工CNC，加工状态，程序名，使用刀具信息）
	bool json1(CString rfid, CString start_time, CString end_time, CString cnc_name, int status, CString program,CString& str_json, std::map<CString, CString> &map_tool_name_qrcode);
	//生成工件状态信息json
	bool json2(CString rfid, CString cnc_name, int status, CString& str_json);
	//获取_N_TIME_SPF中工件时用的刀具信息，生成json
	bool getToolData(CString rfid, CString cnc_name, CString program, std::vector<std::string>& toolJson, double &Z, std::map<CString,CString> &map_tool_name_qrcode);
	//ERP发送失败缓存池
	void ERPBufferPool(int url_type,CString post_data);
	//工件使用刀具信息记录到数据库
	bool partUsedTool(CString rfid, CString start_time, std::map<CString, CString> map_tool_name_qrcode);
};

