#ifndef __USER_TCP_COM_H__
#define __USER_TCP_COM_H__

//#define	RECV_BUF_MAX	4196	//4096+4NULL
#define	RECV_BUF_MAX	4196
#define NET_MSS_LEN 1460
#define	NET_SOCK_NUM	2
#define NET_LINK_ID0	0
#define NET_LINK_ID1	1

#define	NET_LINK_OPEN	0x70	//�Ѿ�����
#define	NET_LINK_CLOSE	0x71	//�Ѿ��Ͽ�
#define	NET_LINK_RELINK	0x72	//�Ѿ�ʹ��

#define	NET_LINK_ERR	-1
#define	NET_LINK_SUCCESS	1
#endif
