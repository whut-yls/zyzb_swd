#ifndef __EC20_4G_H
#define __EC20_4G_H	 
#include "sys.h"



void EC200MqttClientTask(void const * argument);

void send_NetSync(uint8_t status);
void send_duan_wang(uint8_t duan);


									

extern uint8_t aTxBuffer0[];		//�ر��ϴε�socket����
extern uint8_t aTxBuffer1[];							//����ָ�������OK����ģ�������ͨ��
extern uint8_t aTxBuffer2[];				//���SIM���Ƿ���λ
extern uint8_t aTxBuffer3[];					//��ѯ�ź�����
extern uint8_t aTxBuffer4[];				//��ѯ����״̬
extern uint8_t aTxBuffer5[];			//��������
extern uint8_t aTxBuffer6[];				//��ѯ����״̬
extern uint8_t aTxBuffer7[];			//��ȡPDP��ַ
extern uint8_t aTxBuffer8[];				//����Ϊ����ģʽ
extern uint8_t aTxBuffer9[];				//	���ð������ϵ��豸��Ϣ
extern uint8_t aTxBuffer10[];						//��һ��MQTT�ͻ�������
extern uint8_t aTxBuffer11[];					//����MQTT������
extern uint8_t aTxBuffer12[];						//���ķ���������,���ﶩ������ɹ���������豸TOPIC�鿴����������⹦���ǿ����·����ݵ��豸		
extern uint8_t aTxBuffer13[];									//�����ϱ����⣬����Ĺ������豸�ϱ����ݵ��ƶ�		
extern uint8_t aTxBuffer14[];						//����������Ϣ
extern uint8_t aTxBuffer15[];						//���͵�JSON����
extern uint8_t aTxBuffer16[];					//���ݽ�����1a�������HEX��ʽ����

#endif
