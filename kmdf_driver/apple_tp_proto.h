////by fanxiushu 2017-10-14, Э��ṹ���ݲ��Ե�����ֻ��ȡ�˲��ֽṹ
/// �����ʺ� macbook pro 2017 13��� multi-touch bar.  Apple SPI Device ������������ 2016/5/26�� �汾 6.1.6500.0
/// Apple SPI Device ���������������������ݽṹ������ʵ�ʲ��Ի�ȡ����Ϊ�������ݽṹ�����������Ժ�� Apple SPI Device�汾���ܱ�Apple��˾�޸ġ� 

#pragma once

typedef unsigned short __le16;
typedef unsigned char  u8;

////

#define PACKET_TYPE_READ        0x20
#define PACKET_TYPE_WRITE       0x40
#define PACKET_DEV_KEYB         0x01
#define PACKET_DEV_TPAD         0x02

///
#define PRESS_INTERVAL_MSEC          200   // �ᰴʱ�����ж�

#define STABLE_INTERVAL_MSEC         100    // ��ָ������������ȶ�ʱ���� 

#define RIGHT_BUTTON_LIMIT           4200   //�����곬�� ������֣�������Ҽ�


#define TP_HEADER_SIZE    46
#define TP_FINGER_SIZE    30

//// 46 length 
struct tp_protocol
{
	u8                  type;      // unknown type  =2
	u8                  clicked;   // ��ס�˴����壬 ���ܼ�����ס������ 1
	u8                  unknown1[5]; //
	u8                  is_finger;   // ����������ָ 1�����뿪˲�䣬���� 0
	u8                  unknown2[8]; // 
	u8                  unknown3[8]; // δ֪���̶� 00-01-07-97-02-00-06-00
	u8                  finger_data_length; // ��ָ�����ܳ��ȣ� ��ָ����*30
	u8                  unknown4[5]; //
	u8                  finger_number; //��ָ����
	u8                  Clicked; // ͬ�ϱߵ�clicked
	u8                  state;   // ��ָ���ϱߺ����� 0x10�� ��ָ�뿪˲��������� 1����� 0x80��0x90��������뿪�󣬻������ 0x00
	u8                  state2;  // ��ָ���ϱ� 0x20���뿪˲�� �� 0
	u8                  state3;  // ƽʱ0�� ClickedΪ 0x10
	u8                  zero;    // ʼ�� 0
	u8                  unknown5[10]; /////
};


///// 30 length
struct tp_finger
{
	short             org_x; //���º�������ֲ��䣬
	short             org_y; //
	short             x;     //������ָ�ƶ��ı䣬
	short             y;     //
	__le16            unknown[11];
};


/////////////���������ݷ�������ģ�����

struct mouse_event_t
{
	unsigned char     button; /// 0 û�� �� 1 �����ס�� 2 �Ҽ���ס��
    short             dx;
	short             dy;
	char              v_wheel; //��ֱ����
	char              h_wheel; //ˮƽ����
};

typedef void(*MOUSEEVENTCALLBACK)(mouse_event_t* evt, void* param);

void apple_tp_init(MOUSEEVENTCALLBACK cbk, void* param);

void apple_tp_parse(u8* data, LONG length);

