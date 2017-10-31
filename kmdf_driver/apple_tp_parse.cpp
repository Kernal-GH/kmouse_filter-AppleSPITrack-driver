/// by fanxiushu 2017-10-16
///���ݲ��Ի�ȡ��ƻ�� macbook pro 2017 13���touch bar�Ĵ���������ݣ�ģ��������Ϊ�� ģ�������������������������ˮƽ�ʹ�ֱ����

//  �����ʺ� macbook pro 2017 13��� multi-touch bar.  Apple SPI Device ������������ 2016/5/26�� �汾 6.1.6500.0
/// �����汾�� Apple SPI Device û���Թ���


#include "kmouse_filter.h"
#include "apple_tp_proto.h"

#define MAXFINGER_CNT 10

struct apple_tp_state
{
	MOUSEEVENTCALLBACK evt_cbk;
	void*              evt_param;
	/////
	u8                 first_clicked_finger_count; //��һ����ѹʱ����ָ����,����ģ���Ҽ�ʱ����ж�
	u8                 clicked; 
	LARGE_INTEGER      first_clicked_tick; ///��һ���ذ���ʱ��

	u8                 finger_number;

	u8                 max_finger_count;
	u8                 is_finger; ///
	LARGE_INTEGER      last_finger_tick; ///��ָ�ڴ�����ͣ��ʱ�� ���������㡯�ᰴ��
	LONG               light_press_pre_state; ///
	BOOLEAN            right_finger_press_small; ///˫ָ�ᰴģ���Ҽ�ʱ���ж�˫ָ��ľ���

	ULONG              tick_count;

	LARGE_INTEGER      last_stable_tick; //��ָ�ŵ�������֮����ȶ�ʱ�䣬������֤��ָ�ȶ���������֮����������dx��dyƫ��

	LONG               evt_repeat_count;
	mouse_event_t      last_evt;

	BOOLEAN            is_wheel; //����
	LARGE_INTEGER      last_wheel_tick ;

	//��ָ�϶�
	BOOLEAN            is_3finger_drag;  ///

	struct FINGER{
		short              last_x;
		short              last_y;
		///
		LARGE_INTEGER      last_tick;
	}
	fingers[MAXFINGER_CNT];

};
static apple_tp_state tp_state;
#define tp  (&tp_state)

void apple_tp_init(MOUSEEVENTCALLBACK cbk, void* param)
{
	RtlZeroMemory(tp, sizeof(apple_tp_state));

	tp->tick_count = KeQueryTimeIncrement(); ///
	tp->evt_cbk  = cbk;
	tp->evt_param = param;
	////

}

static __forceinline short abs(short x)
{
	if (x < 0)return -x;
	return x;
}

static __forceinline BOOLEAN two_finger_small(tp_finger* g1, tp_finger* g2 )
{
	const LONG SMALL = 3000;
	if (abs(g1->x - g2->x) <= SMALL && abs(g1->y - g2->y) <= SMALL) return TRUE;

	return FALSE;
}

///���������ݳ���4 ��
static __forceinline short modify_value(LONG v)
{
	LONG r ;

	r = v / 4;
//	r = v / 5;
//	r = v * 22 / 100;
	return (short)r;
}

void apple_tp_parse(u8* data, LONG length)
{
	///TEST
/*	{
		static char tmp[4096]; tmp[0] = 0;
		tp_protocol* p = (tp_protocol*)data;
	//	sprintf(tmp, "clicked=%d, is_finger=%d, len=%d, fingercount=%d, state=[%.2X,%.2X,%.2X] -> ",
	//		p->clicked, p->is_finger, p->finger_data_length, p->finger_number, p->state, p->state2, p->state3);
		tp_finger* f1 = (tp_finger*)(data + 46);
		for (u8 i = 0; i < 30; ++i) {
			sprintf( tmp + strlen(tmp), "%.2X-", data[46 +i] );
		}
	//	sprintf(tmp + strlen(tmp), "");
		////
		if (p->finger_number == 2) {
			tp_finger* f2 = (tp_finger*)(data + 46 + 30 );
			sprintf(tmp + strlen(tmp), "< [%d, %d]-[%d, %d] -- [%d, %d]-[%d, %d] >", f1->org_x,f1->org_y, f1->x,f1->y, f2->org_x,f2->org_y,f2->x,f2->y);
		}
		DbgPrint("%s\n", tmp);
		////
	}*/

	////
	struct tp_protocol* pr = (struct tp_protocol*)data;
	mouse_event_t evt;

	if (length < TP_HEADER_SIZE || length < TP_HEADER_SIZE + TP_FINGER_SIZE*pr->finger_number || pr->finger_number >= MAXFINGER_CNT )return; //
	////
	BOOLEAN is_called_evt = FALSE;
	BOOLEAN finger_change = FALSE;
	BOOLEAN number_change = FALSE; ///
	BOOLEAN clicked_change = FALSE;///
	
	LARGE_INTEGER tick, dt; 
	KeQueryTickCount(&tick);

	//�ж���ָ�Ƿ��һ���뿪���߷ŵ���������
	if (pr->is_finger != tp->is_finger) { 
		tp->is_finger = pr->is_finger;
		finger_change = TRUE;
		////
		if (pr->is_finger) {

			tp->last_stable_tick = tp->last_finger_tick = tick; ////

			/////
			tp->light_press_pre_state = 1; ///��ָ�ŵ�������

		}
		else {
			///
			RtlZeroMemory(&tp->fingers, sizeof(tp->fingers)); ////
		}
	}

	////�жϷŵ����������ָ�����仯
	if (pr->finger_number != tp->finger_number || (!tp->is_finger && finger_change ) ) { //��ָ�����仯�����뿪������

		tp->finger_number = pr->finger_number;
		number_change = TRUE;
		////
		tp->last_stable_tick = tick; ///

		///
	}

	///�ж��Ƿ��һ����ѹ
	if (pr->clicked != tp->clicked) {
		tp->clicked = pr->clicked;
		clicked_change = TRUE;

		tp->first_clicked_finger_count = pr->finger_number;
		////

		tp->first_clicked_tick = tp->last_stable_tick = tick;
		///
	}

	///
	evt.v_wheel = evt.h_wheel = 0; 
	evt.dx = evt.dy = 0;

	//����dx��dy
	short dx=0, dy=0; LONG d_xy = 0;
	long curr_index = -1;
	short last_x1=0, last_y1=0, last_x2=0, last_y2=0; //���ڹ����ж�

#define X_MAX_RANGE        6600
#define Y_MIN_RANGE        100

	///�����������ָ
	u8 left_finger = 255; short left_x = 30000;
	if (pr->finger_number > 1) {
		for (u8 i = 0; i < pr->finger_number; ++i) {
			struct tp_finger* g = (struct tp_finger*)(data + TP_HEADER_SIZE + i*TP_FINGER_SIZE); //
			if (g->x < left_x ) {
				left_x = g->x;
				left_finger = i;
			}
		}

	}

	///��������ָ�ƶ��У������ƶ�����ҵ���ָ��Ϊ�ƶ��仯ֵ; 
	for (u8 i = 0; i < pr->finger_number; ++i) {
		/////
		apple_tp_state::FINGER* f = &tp->fingers[i];
		struct tp_finger* g = (struct tp_finger*)(data + TP_HEADER_SIZE + i*TP_FINGER_SIZE );
		
		if(number_change){ //��ָ���������仯��
			///

			f->last_x = g->x;  f->last_y = g->y;
			/////
		}
		else if ( !tp->is_finger) { //��ָ�뿪������
			////

		}
		else {
			///
			short x = g->x - f->last_x; 
			short y = g->y - f->last_y;
			LONG dd = abs(x) + abs(y);

			if (d_xy < dd && 
				i != left_finger && //����������ߵ���ָ
				g->y > Y_MIN_RANGE && g->x < X_MAX_RANGE )  //���Դ�����ײ�,��ֹ��������Եʱ����, ���� ����������ұߴﵽ6800�� ���-6300�� �ϱ� 7700�� �±�-200 ����Լ����, 13��ʼǱ����Խ��
			{
				d_xy = dd;
				dx = x; dy = y;
				////
				curr_index = i; 
			}

			if (i == 0) { last_x1 = f->last_x; last_y1 = f->last_y; }
			else if (i == 1) { last_x2 = f->last_x; last_y2 = f->last_y; }

			//
			f->last_x = g->x;
			f->last_y = g->y;
			////
		}
		//////
	}
	
	///��������

	dx = modify_value(dx);
	dy = -modify_value(dy);

	//�ȶ�����������ָ�Ӵ������峬��һ��ʱ�䣬������dx��dy
	dt.QuadPart = (tick.QuadPart - tp->last_stable_tick.QuadPart)*tp->tick_count / 10000;

	if (dt.QuadPart <= STABLE_INTERVAL_MSEC ) { 
		////
		dx = dy = 0;
	}
	////
	
	if (!pr->is_finger || (pr->state & 0x80 ) || pr->state == 0 ) { //��ָ�뿪
		///
		dx = dy = 0; 

		tp->is_wheel = FALSE;
		tp->is_3finger_drag = FALSE;
	}

	if (pr->clicked) {//��ס��ǰһ��ʱ�䣬����dx��dyΪ 0

		dt.QuadPart = (tick.QuadPart - tp->first_clicked_tick.QuadPart)*tp->tick_count / 10000;
		///
		if (dt.QuadPart < STABLE_INTERVAL_MSEC * 1 ) {
			dx = dy = 0; 
		}

		if(tp->light_press_pre_state != 2) tp->light_press_pre_state = 2; ////�Ѿ�����
	}

	/////
	evt.dx = dx;
	evt.dy = dy;

	if (abs(dx) > 2 || abs(dy) > 2 ) { //�������ƶ�
		////
		if (tp->light_press_pre_state == 1) tp->light_press_pre_state = 3;
	}
	////
	
	if (pr->is_finger) {
		///��¼��ס�����壬������ָ���������ж��ᰴ
		if (tp->max_finger_count < pr->finger_number) tp->max_finger_count = pr->finger_number; /////

		if (pr->finger_number == 2 && tp->max_finger_count == 2 ) {
			tp_finger* g1 = (tp_finger*)(data + TP_HEADER_SIZE);
			tp_finger* g2 = (tp_finger*)(data + TP_HEADER_SIZE + TP_FINGER_SIZE);
			////
			tp->right_finger_press_small = two_finger_small(g1, g2);
		}
	}

	evt.button = 0; ////
	if (pr->clicked) { //�Ѿ�����
		evt.button = 1; // left button

		////�ж��ǲ����Ҽ�
		tp_finger* g = (tp_finger*)(data + TP_HEADER_SIZE);

		if (tp->first_clicked_finger_count == 1 && g->x > RIGHT_BUTTON_LIMIT) {

			evt.button = 2; /////
		}

		//////
	}
	else if(finger_change && !tp->is_finger ) {// �ж��ᰴ,��ָ�뿪֮��
		///
		dt.QuadPart = (tick.QuadPart - tp->last_finger_tick.QuadPart )*tp->tick_count / 10000; //����

		////
		if ( dt.QuadPart > 0 && dt.QuadPart <= PRESS_INTERVAL_MSEC ) /// �ᰴ�ж�
		{ 
			////

			if (tp->max_finger_count == 1) {
				if(tp->light_press_pre_state == 1 ) { //�ᰴ�ڼ� û�����ƶ�,Ҳû����
					is_called_evt = TRUE;
					evt.button = 1;
				}
			}
			else if (tp->max_finger_count == 2 && tp->right_finger_press_small ) { //������ָ������������ָ�������ƶ���Χ
				if (tp->light_press_pre_state == 1) { //�ᰴ�ڼ� û�����ƶ���Ҳû����
					is_called_evt = TRUE;
					evt.button = 2;
				}
			}
			/////
			if (is_called_evt) {
				
				evt.dx = evt.dy = 0; evt.v_wheel = evt.h_wheel = 0;

				tp->evt_cbk(&evt, tp->evt_param);

				evt.button = 0;
				tp->evt_cbk(&evt, tp->evt_param);
			}
			/////
		}
		else {
			tp->light_press_pre_state = 0; ////����ʱ�䣬�����ж���
		}

		////
		tp->max_finger_count = 0; ///
		tp->right_finger_press_small = FALSE;
		//
	}
	///////
	if (!is_called_evt) {
		
		///�жϹ���,������ָͬʱ�ƶ�������
		if (pr->finger_number == 2 && !pr->clicked && tp->is_finger && !number_change ) {
			struct tp_finger* g1 = (struct tp_finger*)(data + TP_HEADER_SIZE);
			struct tp_finger* g2 = (struct tp_finger*)(data + TP_HEADER_SIZE + TP_FINGER_SIZE);
			////
			short dx1 = g1->x - last_x1; short dy1 = g1->y - last_y1;
			short dx2 = g2->x - last_x2; short dy2 = g2->y - last_y2;
			dx1 = modify_value(dx1); dy1 = modify_value(dy1);
			dx2 = modify_value(dx2); dy2 = modify_value(dy2);
			const int DT = 2;
	//		DPT("dx1=%d, dy1=%d;  dx2=%d, dy2=%d; [%d, %d] -> [%d, %d]\n", dx1, dy1, dx2, dy2, g1->x, g1->y, g2->x, g2->y );
			if (abs(dx1 - dx2) <= DT && abs(dy1 - dy2) <= DT && 
				(abs(dx1)>= 0 || abs(dy1)>= 0 ) && 
				g1->y > Y_MIN_RANGE && g2->y > Y_MIN_RANGE && 
				two_finger_small(g1,g2) //������ָ��಻��
				) //
			{ 
				if (!tp->is_wheel) {
					tp->is_wheel = TRUE;
					tp->last_wheel_tick = tick;
				}
				////
			}
			
			if (tp->is_wheel) {
				evt.dx = evt.dy = 0;
				dt.QuadPart = (tick.QuadPart - tp->last_wheel_tick.QuadPart)*tp->tick_count / 10000; //����
				if (dt.QuadPart > PRESS_INTERVAL_MSEC || tp->light_press_pre_state == 3 ) {
					
					if (abs(dx1) >= 1 || abs(dy1) >= 1) {
						if (abs(dx1) >= abs(dy1)) {
							if (dx1 > 0) evt.h_wheel = 1; else evt.h_wheel = -1;
						}
						else {
							if (dy1 > 0) evt.v_wheel = 1; else evt.v_wheel = -1;
						}
					}
				    ///
				}
				///
			}
			////////////////
		}

		///�ж���ָ����
		if (pr->finger_number == 3 && !pr->clicked && tp->is_finger && !number_change) {
			///
			if (!tp->is_3finger_drag) {
				struct tp_finger* g[3];
				g[0] = (struct tp_finger*)(data + TP_HEADER_SIZE);
				g[1] = (struct tp_finger*)(data + TP_HEADER_SIZE + TP_FINGER_SIZE);
				g[2] = (struct tp_finger*)(data + TP_HEADER_SIZE + 2 * TP_FINGER_SIZE);
				LONG i, j;
				for (i = 0; i < 3; ++i) {
					LONG k = i;
					for (j = i + 1; j < 3; ++j) if (g[k]->x > g[j]->x)k = j;
					if (k != i) { tp_finger* t = g[i]; g[i] = g[k]; g[k] = t; }
				}
				////

				if (two_finger_small(g[0], g[1]) && two_finger_small(g[1], g[2]) && (abs(dx)>1 || abs(dy)>1 ) ) { ///��ָ��಻��
					///
					tp->is_3finger_drag = TRUE;
				}
				/////
			}

			//////
			if (tp->is_3finger_drag) {
				////
				evt.button = 1;   //�϶�
			}

			///////
		}
		else {
			if (tp->is_3finger_drag)tp->is_3finger_drag = FALSE;
		}

		////////
		if (
			tp->last_evt.button == evt.button &&
			tp->last_evt.dx == evt.dx && tp->last_evt.dy == evt.dy &&
			tp->last_evt.v_wheel == evt.v_wheel && tp->last_evt.h_wheel == evt.h_wheel)
		{
			tp->evt_repeat_count++;
		}
		else {
			tp->evt_repeat_count = 0;
		}
		//////
		if(tp->evt_repeat_count < 8 || evt.v_wheel || evt.h_wheel )
			tp->evt_cbk(&evt, tp->evt_param); ////

		///
		tp->last_evt = evt;
	}

	/////

}

