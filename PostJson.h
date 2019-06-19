#pragma once
#include <vector>
#include <map>
//ERP��httpclientͨ�ţ�httpclient��libcurl.lib,�������·��
//��Դ��https://10.10.10.133/svn/X-MASTER_FUCHUAN/3 Source Code/curl-7.26.0
class CPostJson
{
public:
	CPostJson();
	~CPostJson();

	//�ӹ���ɺ��ͼӹ���Ϣ��ERP
	bool sendJson(CString rfid, CString start_time, CString end_time, CString cnc_name, int status, CString program, CString str_SQL_start_time);
	//����rfid��ERP��ȡ��������
	bool sendData(std::string rfid, CString& partNo, CString& orderNo, int& partType, CString& program, CString& cnc);
	//���͹�����ʵʱ״̬��ERP
	bool sendStatus(CString rfid, CString cnc_name, int status);
	//����ERPʧ��ʱ����Email
	void SendEmail(CString str_cmd);
	//warning��error��Ϣ���͵�΢��
	void SendWX(CString msg);
private:
	//���ɼӹ���Ϣjson����ʼ������ʱ�䣬�ӹ�CNC���ӹ�״̬����������ʹ�õ�����Ϣ��
	bool json1(CString rfid, CString start_time, CString end_time, CString cnc_name, int status, CString program,CString& str_json, std::map<CString, CString> &map_tool_name_qrcode);
	//���ɹ���״̬��Ϣjson
	bool json2(CString rfid, CString cnc_name, int status, CString& str_json);
	//��ȡ_N_TIME_SPF�й���ʱ�õĵ�����Ϣ������json
	bool getToolData(CString rfid, CString cnc_name, CString program, std::vector<std::string>& toolJson, double &Z, std::map<CString,CString> &map_tool_name_qrcode);
	//ERP����ʧ�ܻ����
	void ERPBufferPool(int url_type,CString post_data);
	//����ʹ�õ�����Ϣ��¼�����ݿ�
	bool partUsedTool(CString rfid, CString start_time, std::map<CString, CString> map_tool_name_qrcode);
};

