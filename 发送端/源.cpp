#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<queue>
#include<thread>
#include<winsock2.h>
#include<mutex>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

//����
#define send_IP "127.0.0.1"
#define send_Port 6000
#define recv_IP "127.0.0.1"
#define recv_Port 2345
SOCKET sockSend;//�������Ͷ�socket�����󶨴����Э��UDP�����ݱ��׽��֣�IPv4
SOCKADDR_IN addrSend;//���巢�Ͷ˵�ַ
SOCKADDR_IN addrRecv;//������ն˵�ַ
int addr_len = sizeof(sockaddr_in);
const int max_size = 1024;//һ�����ݰ������������
const int SEND_TIMES = 20;//����ش�����
const int WAIT_TIME = 0.2 * CLOCKS_PER_SEC;//��ʱ���ȴ�ʱ�䣬1s
int seq = 0;//�������к�
char* file_buf;//�����ļ�������
int file_len = 0;//�����ļ�����

//��ȡ��ǰϵͳ���ں�ʱ��
char* get_time()
{
	time_t now = time(0); // �� now ת��Ϊ�ַ�����ʽ 
	char* dt = ctime(&now);
	return dt;
}

//�������ݰ�
struct packet
{
	char flag;//��־λ '0':SYN;'1':ACK;'2':FIN
	bool is_final_file;//������ݰ��Ƿ�Ϊ�ļ����һ�� true:�ǣ�false:��
	bool not_empty;//���ݰ��ǿձ�־ true:�ǿ�;false:��
	DWORD send_ip, recv_ip;
	u_short send_port, recv_port;
	int Seq;//���
	int ACK;//Ӧ�����
	int len;//����
	u_short checksum;//16λ��У���
	char data[max_size];//�������

	packet(); //���캯��
};

//���ݰ���ʼ��
packet::packet() {
	//���������ݰ�����Ϊȫ��
	memset(this, 0, sizeof(packet));
	send_ip = DWORD(send_IP);
	recv_ip = DWORD(recv_IP);
	send_port = send_Port;
	recv_port = recv_Port;
	Seq = 0;
	ACK = 0;
	len = 0;
	checksum = 0;
}

//����У���
void set_checksum(packet& p)
{
	//У�������
	memset(&p.checksum, 0, sizeof(p.checksum));
	//�������ݰ����ж��ٸ�16�ֽ�����ȡ��
	int count = (sizeof(packet) + 1) / 2;
	//ָ���һ��16λ
	u_short* bit16 = (u_short*)(&p);
	u_long sum = 0;
	for (int i = 0; i < count; i++)
	{
		//ʵ�ַ������
		sum += *(bit16++);
		if (sum & 0xffff0000)
		{
			sum -= 0xffff;
			sum += 1;
		}
	}
	p.checksum = ~sum;
	return;
}

//��֤У���
bool is_checksum(packet& p) {
	int count = (sizeof(packet) + 1) / 2;
	u_short* bit16 = (u_short*)(&p);
	u_long sum = 0;
	for (int i = 0; i < count; i++) {
		sum += *(bit16++);
		if (sum & 0xffff0000) {
			sum -= 0xffff;
			sum += 1;
		}
	}
	if ((sum == 0xffff))
		return true;
	else
		return false;
}

//�������ͺ����������sendto������������ţ��ǿռ��飬У���
bool my_sendto(packet& packet) {
	packet.not_empty = true;
	set_checksum(packet);
	int state;
	state = sendto(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, sizeof(sockaddr));
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//�������պ����������recvfrom������У��ͺͿհ����
bool my_recvfrom(packet& packet) {
	memset(packet.data, 0, sizeof(packet.data));
	int state;
	//���յ��İ�����packet��
	state = recvfrom(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, &addr_len);
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//���ֽ���
bool send_Conn()//���Ͷˣ������������
{
	packet send_syn, recv_ack;
	bool state1, state2;

	//���ͽ�������
	send_syn.flag = '0';//SYN
	send_syn.Seq = seq++;
	state1 = my_sendto(send_syn);
	if (state1)
		cout << "[log]" << get_time() << ">>>" << "������������ɹ�" << endl;
	else
	{
		cout << "[log]" << get_time() << ">>>" << "������������ʧ��" << endl;
		return false;
	}

	while (1)
	{
		//���ն˻�Ӧ
		state2 = my_recvfrom(recv_ack);
		if (state2)
		{
			cout << "[log]" << get_time() << ">>>" << "�յ���Ӧ�����ӳɹ���" << endl;
			return true;
		}
	}
}

//���ֶ���
bool send_Clo()
{
	packet send_fin, recv_fin;
	bool state1, state2;

	//���Ͷ�������
	send_fin.flag = '2';
	send_fin.Seq = seq++;
	state1 = my_sendto(send_fin);
	if (state1)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ�������ɹ�!" << endl;
	else
	{
		cout << "[log]" << get_time() << ">>>" << "���Ͷ�������ʧ��" << endl;
		return false;
	}

	while (1)
	{
		//���ն˻�Ӧ
		state2 = my_recvfrom(recv_fin);
		if (state2)
		{
			cout << "[log]" << get_time() << ">>>" << "�յ���Ӧ���Ͽ��ɹ���" << endl;
			return true;
		}
	}
}

//���ļ�
bool read_file()
{
	cout << "[log]" << get_time() << ">>>" << "�������ļ�·��:";
	string path; //cin >> path;
	path = "C:\\Users\\PC\\Desktop\\����\\ʵ��\\UDP\\�����ļ�\\1.jpg";
	ifstream file;
	//���ļ�
	file.open(path, ifstream::in | ios::binary);

	if (file)
	{
		cout << "[log]" << get_time() << ">>>" << "��ʼ��ȡ�ļ�" << endl;

		//��ȡ�ļ���С
		//��ָ��ָ���ļ�ĩβ��ƫ��0
		file.seekg(0, file.end);
		//��ȡָ�뵱ǰλ��
		file_len = file.tellg();
		//�ָ�ָ��Ϊ�ļ���ʼ
		file.seekg(0, file.beg);
		cout << "[log]" << get_time() << ">>>" << "�ļ���СΪ " << file_len << " Byte" << endl;    //�ֽ�Ϊ��λ

		//��ȡ�ļ�������file_buf����
		file_buf = new char[file_len + 1];
		file_buf[file_len] = '\0';
		file.read(file_buf, file_len);
		cout << "[log]" << get_time() << ">>>" << "�ļ���ȡ�ɹ�!" << endl;

		//�ر��ļ�
		file.close();
		return true;
	}
	else
	{
		cout << "[log]" << get_time() << ">>>" << "��ȡ�ļ�ʧ��" << endl;
		return false;
	}
}

//�ش�����(��Ų�����)
bool re_sendto(packet& packet)
{
	packet.not_empty = true;
	set_checksum(packet);
	int state;
	state = sendto(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, sizeof(sockaddr));
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

int base = 1;//���ڵײ�
int nextseqnum = 1;//��һ��Ҫ���͵����
int N = 5;//���ڴ�С
int packet_num = 0;     //��������
//���ͻ������
queue<packet> message_queue;
//���ջ�����
packet recv_packet;
//��ʱ��
clock_t start;
//�ش�����
int send_t = 0;

// Reno��һЩȫ�ֱ���
int cwnd = 1;     //ӵ������
int recv_window = 2560; // ���ݽ��ն˵Ļ���������ã�ÿ�ν��յ�ACK���ͼ�һ
int ssthresh = 16; // ��ֵ
int dupACKcount = 0; // �ظ�ACK����
int RenoState = 0; // ��־״̬��  0�������� 1��ӵ������ 2�����ٻָ�
uint16_t recover = 1;   //���ٻָ��׶α�����һ�����ķ������
//ӵ�����ƽ׶μ����õ�
int RTT_ACK = 0;

//���������־
void GBN_log()
{
	cout << "��ǰ����:[" << base << "-" << base + N - 1 << "]" << "����СΪ��" << N;
	cout << "����ǰ�������:" << nextseqnum << endl;
	return;
}

//��Щ������Ҫ��������ֹ����
mutex slock;

//�����߳�
void send_handle(int packet_num)
{
	if (nextseqnum < base + N && nextseqnum <= packet_num)
	{
		//����
		slock.lock();
		//�����־
		GBN_log();
		//Ҫ���͵İ�
		packet send_packet;
		//�������һ����
		if (nextseqnum != packet_num)
		{
			send_packet.is_final_file = false;
			send_packet.Seq = seq++;
			send_packet.len = max_size;
			for (int j = 0; j < send_packet.len; j++)
				send_packet.data[j] = file_buf[(nextseqnum - 1) * max_size + j];
			my_sendto(send_packet);
			//��ȷ���Է��Ƿ��յ����ȼ��뻺�����
			message_queue.push(send_packet);
			nextseqnum++;
		}
		//���һ����
		else
		{
			send_packet.is_final_file = true;
			send_packet.Seq = seq++;
			send_packet.len = file_len - (nextseqnum - 1) * max_size;
			for (int j = 0; j < send_packet.len; j++)
				send_packet.data[j] = file_buf[(nextseqnum - 1) * max_size + j];
			my_sendto(send_packet);
			//��ȷ���Է��Ƿ��յ����ȼ��뻺�����
			message_queue.push(send_packet);
			nextseqnum++;
		}
		//����
		slock.unlock();
	}
}

//�����߳�
void recv_handle()
{
	slock.lock();
	//����Ӧ��
	if (my_recvfrom(recv_packet))
	{
		//����Ƿ�ΪӦ����Լ�У����Ƿ���ȷ
		if (recv_packet.flag == '1' && is_checksum(recv_packet))
		{
			//ѡ��״̬
			switch (RenoState)
			{
				//�������׶�
			case 0:
				cout << "***** �������׶� *****" << endl;
				//�����־
				GBN_log();
				//�µ�ACK
				if (base == recv_packet.ACK)
				{
					cout << "���" << base << "�ѱ�ȷ�Ͻ���" << endl;
					//������δ����ֵ
					if (cwnd < ssthresh) {
						cwnd++;
						N = min(cwnd, recv_window); // ��Ҫ���·��ʹ��ڴ�С
						cout << "*** �������׶ν��յ���ACK��cwnd++ ***" << endl;
					}
					//����ֵ��
					else {
						RenoState = 1;
						cout << "*** �������׶ν���������ӵ������׶� ***" << endl;
					}

					// �����µ�ACK��dup=0
					dupACKcount = 0;
					//�����ƶ�һ��λ��
					base++;
					//���մ���--
					recv_window--;
					//������������
					N = min(cwnd, recv_window);
					//���ڵײ������ݰ�ȷ���յ��ˣ�������һ��
					message_queue.pop();
					//���¼�ʱ
					start = clock();
					send_t = 0;
				}
				//�ظ�ACK
				else
				{
					dupACKcount++;
					cout << "��� " << recv_packet.ACK << " ���ظ�ȷ�ϣ����ֶ�����" << endl;
					//do nothing
				}

				//�����ظ�ACK
				if (dupACKcount == 3)
				{
					cout << "*** ��⵽�����ظ�ACK����ʼ�ش���֮�������ٻָ��׶� ***" << endl;
					//���¼�ʱ
					start = clock();
					cout << "�ش����:[" << message_queue.front().Seq << "-" << message_queue.back().Seq << "]" << endl;
					/*
					if (send_t > SEND_TIMES)
					{
						base = nextseqnum;
						//��ն���
						while (!message_queue.empty())message_queue.pop();

						//����������
						ssthresh = 16;
						cwnd = 1;
						N = 5;
						dupACKcount = 0; // ���¼���
						N = min(cwnd, recv_window);
						RenoState = 0; // ���½����������׶�
						//ӵ�����ƽ׶μ����õ�
						int RTT_ACK = 0;
					}*/
					//else
					//{
						if (message_queue.size() != 0)
						{
							//send_t++;
							int t = message_queue.size();
							for (int i = 0; i < t; i++)
							{
								my_sendto(message_queue.front());
								message_queue.push(message_queue.front());
								message_queue.pop();
							}
						}
						//���ٻָ��׶�
						ssthresh = cwnd / 2;
						cwnd = ssthresh + 3;
						N = min(cwnd, recv_window); // ��Ҫ���·��ʹ��ڴ�С
						RenoState = 2;
						recover = message_queue.back().Seq;
						dupACKcount = 0;
					//}
				}
				break;

				//ӵ������׶�
			case 1:
				cout << "**** ӵ������׶� ****" << endl;
				GBN_log();
				//�µ�ACK
				if (base == recv_packet.ACK)
				{
					cout << "���" << base << "�ѱ�ȷ�Ͻ���" << endl;
					//���յ���ACK��������һ
					RTT_ACK++;
					//���������һ�������ڵ�ACK���򴰿�++��ʵ�����Ե���
					if (RTT_ACK == cwnd)
					{
						cwnd++;
						N = min(cwnd, recv_window); // ��Ҫ���·��ʹ��ڴ�С
						RTT_ACK = 0;
						cout << "*** ӵ������׶Σ��������Ե���һ ***" << endl;
					}

					// �����µ�ACK��dup=0
					dupACKcount = 0;
					//�����ƶ�һ��λ��
					base++;
					//���մ���--
					recv_window--;
					//������������
					N = min(cwnd, recv_window);
					//���ڵײ������ݰ�ȷ���յ��ˣ�������һ��
					message_queue.pop();
					//���¼�ʱ
					start = clock();
					send_t = 0;
				}
				//�ظ�ACK
				else
				{
					dupACKcount++;
					RTT_ACK = 0;   //�ֳ��ֶ����ˣ����Կ������������ˣ��Ǵ��ھ���������
					cout << "��� " << recv_packet.ACK << " ���ظ�ȷ�ϣ����ֶ�����" << endl;
					//do nothing
				}

				//�����ظ�ACK
				if (dupACKcount == 3)
				{
					cout << "*** ��⵽�����ظ�ACK����ʼ�ش���֮�������ٻָ��׶� ***" << endl;
					cout << "�ش����:[" << message_queue.front().Seq << "-" << message_queue.back().Seq << "]" << endl;
					//���¼�ʱ
					start = clock();
					/*
					if (send_t > SEND_TIMES)
					{
						base = nextseqnum;
						//��ն���
						while (!message_queue.empty())message_queue.pop();

						//����������
						ssthresh = 16;
						cwnd = 1;
						N = 5;
						dupACKcount = 0; // ���¼���
						N = min(cwnd, recv_window);
						RenoState = 0; // ���½����������׶�
						//ӵ�����ƽ׶μ����õ�
						int RTT_ACK = 0;
					}
					*/
					//else
					//{
						if (message_queue.size() != 0)
						{
							//send_t++;
							int t = message_queue.size();
							for (int i = 0; i < t; i++)
							{
								my_sendto(message_queue.front());
								message_queue.push(message_queue.front());
								message_queue.pop();
							}
						}
						//���ٻָ��׶�
						ssthresh = cwnd / 2;
						cwnd = ssthresh + 3;
						N = min(cwnd, recv_window); // ��Ҫ���·��ʹ��ڴ�С
						RenoState = 2;
						recover = message_queue.back().Seq;
						dupACKcount = 0;
					//}
				}
				break;

				//���ٻָ��׶�
			case 2:
				cout << "**** ���ٻָ��׶� ****" << endl;
				GBN_log();
				//�յ�������еײ������ݰ�
				if (base == recv_packet.ACK)
				{
					cout << "���" << base << "�ѱ�ȷ�Ͻ���" << endl;
					//�������е����ݰ��Ƿ��Ѿ��յ�ACK
					//���û�У���������ֿ��ٻָ�״̬��������յ��ˣ������ӵ������״̬
					//recover��δȷ�ϰ���������ģ�������Ѿ�ȷ�ϣ�������оͶ���ȷ����
					if (recv_packet.ACK >= recover)
					{
						RenoState = 1; // ����ӵ������׶�
						cwnd = ssthresh - 1;
						N = min(cwnd, recv_window); // ��Ҫ���·��ʹ��ڴ�С
					}

					dupACKcount = 0;
					base = base + 1; // ȷ��һ���ƶ�һ��λ��
					cwnd++;    // ÿ����һ����ȷ��ACK�����ھͼ�һ
					recv_window--;
					N = min(cwnd, recv_window);
					message_queue.pop();
					start = clock();
					send_t = 0;
				}
				//�����ظ���ACK
				else
				{
					dupACKcount++;
					cwnd++;
					N = min(cwnd, recv_window);
					cout << "��� " << recv_packet.ACK << " ���ظ�ȷ�ϣ����ֶ�����" << endl;
				}
				//�ظ����ACK
				if (dupACKcount == 6)
				{
					//�ڿ��ٻָ��׶λ��ڶ�������������6������ôֱ�ӽ�����������������
					cout << "*** ��⵽�����ظ�ACK����ʼ�ش���֮�����½����������׶� ***" << endl;
					cout << "�ش����:[" << message_queue.front().Seq << "-" << message_queue.back().Seq << "]" << endl;
					//�ش�
					//���¼�ʱ
					start = clock();
					/*
					if (send_t > SEND_TIMES)
					{
						base = nextseqnum;
						//��ն���
						while (!message_queue.empty())message_queue.pop();

						//����������
						ssthresh = 16;
						cwnd = 1;
						N = 5;
						dupACKcount = 0; // ���¼���
						N = min(cwnd, recv_window);
						RenoState = 0; // ���½����������׶�
						//ӵ�����ƽ׶μ����õ�
						int RTT_ACK = 0;
					}
					*/
					//else
					//{
						if (message_queue.size() != 0)
						{
							//send_t++;
							int t = message_queue.size();
							for (int i = 0; i < t; i++)
							{
								my_sendto(message_queue.front());
								message_queue.push(message_queue.front());
								message_queue.pop();
							}
						}
						ssthresh = cwnd / 2; // ��ֵ
						cwnd = 1;
						dupACKcount = 0; // ���¼���
						N = min(cwnd, recv_window);
						RenoState = 0; // ���½����������׶�
					//}
				}
				break;
			default:
				cout << "�����RenoState!" << endl;
				break;
			}
		}
		else;
		//���յ��Ļ�Ӧ������do nothing
	}
	slock.unlock();
}

void timeout()
{
	//��ʱ�ش�,��˳���ش���������е����ݰ�
	if (clock() - start > WAIT_TIME)
	{
		slock.lock();
		cout << "***��ʱ���ش�������δȷ�����ݰ�***" << endl;
		cout << "�ش����:[" << message_queue.front().Seq << "-" << message_queue.back().Seq << "]" << endl;
		//���¼�ʱ
		start = clock();

		/*
		if (send_t > SEND_TIMES)
		{
			base = nextseqnum;
			//��ն���
			while (!message_queue.empty())message_queue.pop();

			//����������
			ssthresh = 16;
			cwnd = 1;
			N = 5;
			dupACKcount = 0; // ���¼���
			N = min(cwnd, recv_window);
			RenoState = 0; // ���½����������׶�
			//ӵ�����ƽ׶μ����õ�
			int RTT_ACK = 0;
		}
		*/
		//else
		//{
			if (message_queue.size() != 0)
			{
				//send_t++;
				int t = message_queue.size();
				for (int i = 0; i < t; i++)
				{
					my_sendto(message_queue.front());
					message_queue.push(message_queue.front());
					message_queue.pop();
				}
			}
			ssthresh = cwnd / 2; // ��ֵ����
			cwnd = 1;          //ӵ�����ڱ�Ϊ1
			dupACKcount = 0; // ����ACK���¼���
			N = min(cwnd, recv_window);  //��������ѡ��ӵ�����ںͽ��ն˴����н�С��
			RenoState = 0; // �����������׶�
			cout << "***�����������׶Σ����ڴ�С����Ϊ" << N << "***" << endl;
		//}
		slock.unlock();
	}
}

void stop_wait_send_GBN_RENO()
{
	//�����ļ�
	//����
	packet_num = file_len / max_size + (file_len % max_size != 0);
	cout << "[log]" << get_time() << ">>>" << "��Ҫ���͵İ�������Ϊ" << packet_num << "�����ڴ�СΪ" << N << "�����ڷ����ļ�......" << endl;
	//ѭ���������ݰ�
	start = clock();
	while (base <= packet_num)
	{
		thread S_handle = thread(send_handle, packet_num);
		thread R_handle = thread(recv_handle);
		thread Time_handle = thread(timeout);
		S_handle.join();
		R_handle.join();
		Time_handle.join();
	}
}

int main()
{
	int state1, state2;  //����ֺ����ķ���ֵ
	bool state3;

	WORD wVersionRequested = MAKEWORD(2, 2); //������ϣ��ʹ�õ�socket����߰汾
	WSADATA wsaData;  //���õ�Socket����ϸ��Ϣ��ͨ��WSAStartup������ֵ

	//��ʼ��Socket DLL��Э��ʹ�õ�Socket�汾����ʼ���ɹ��򷵻�0������Ϊ�����int������
	state1 = WSAStartup(wVersionRequested, &wsaData);
	if (state1 == 0)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˳�ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˳�ʼ��Socket DLLʧ�ܣ�" << endl;

	//���Ͷ�socket����ַ��
	sockSend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	addrSend.sin_family = AF_INET;  //IPv4��ַ����
	addrSend.sin_addr.s_addr = inet_addr(send_IP);  //���Ͷ�IP
	addrSend.sin_port = htons(send_Port);   //���Ͷ˵Ķ˿ں�
	state2 = bind(sockSend, (SOCKADDR*)&addrSend, sizeof(SOCKADDR));//�󶨵�ַ

	//���÷��Ͷ�socket������
	u_long imode = 1;
	ioctlsocket(sockSend, FIONBIO, &imode);

	if (state2 == 0)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˰󶨱��ص�ַ�ɹ�" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˰󶨱��ص�ַʧ�ܣ�" << endl;

	//������ն˵�ַ
	addrRecv.sin_family = AF_INET;  //IPv4��ַ����
	addrRecv.sin_addr.s_addr = inet_addr(recv_IP);  //���ն�IP
	addrRecv.sin_port = htons(recv_Port);   //���ն˵Ķ˿ں�

	clock_t s, e;
	//��������
	state3 = send_Conn();
	if (state3)
	{
		if (read_file())
		{
			clock_t s = clock();
			//GBN_RENO
			stop_wait_send_GBN_RENO();
			clock_t e = clock();
			//��ӡ����Ч��
			cout << "[log]" << get_time() << ">>>" << "�����ļ�ʱ��Ϊ��" << (e - s) / CLOCKS_PER_SEC << "s" << endl;
			cout << "[log]" << get_time() << ">>>" << "������Ϊ:" << ((float)file_len) / ((e - s) / CLOCKS_PER_SEC) << " bytes/s " << endl << endl;
		}
		//�Ͽ�����
		send_Clo();
	}

	//����ʹ��Socket���ͷ�Socket��Դ
	closesocket(sockSend);
	WSACleanup();

	return 0;
}