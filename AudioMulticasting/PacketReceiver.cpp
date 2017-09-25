#include "stdafx.h"
#include "PacketReceiver.h"


PacketReceiver::PacketReceiver(char* ip, char* port)
{
	OpenTcpConnection(ip, port);
	Connect();
}

int PacketReceiver::OpenUdpConnection(char* ip, int port){
	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//Create a socket
	if ((ConnectSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &(server.sin_addr));
	server.sin_port = htons(27010);

	//jodi server data pathai client ke tkhn oi data paowar jonno port e listen korte hoi tai bind kora lage
	//kintu jodi client server ke data pathai tobe server keo port e listen korte korbe so jehetu ekport e dui
	//jon listen korte pare na tai hoi alada port use korte hobe noi ekjon ke bind korte hobe arek jon ke bind kora jabe na
	//ekhane clienthandler e bind kora hoiche ekta port e tai serverhandler class e bind kora jabe na oi port e
	//karon ei system e shudhu server ckient kei data pathabe
	if (bind(ConnectSocket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	puts("Bind done");

	Remaddr.sin_family = AF_INET;
	Remaddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &(Remaddr.sin_addr));
}
void PacketReceiver::parse(long &ik, long &dk, BYTE* datak, BYTE* recvdata){

	int s1 = sizeof(long);

	int s3 = sizeof(long);


	BYTE* ikc = new BYTE[s1];
	BYTE* dkc = new BYTE[s3];


	int i = 0, j = 0, k = 0;
	if (recvdata[i] == 'i'){
		BYTE a, b, c;
		for (i = 0; i < s1; i++){
			a = recvdata[i + 1];
			ikc[i] = a;
		}
		ik = *(long*)ikc;
		if (recvdata[(i + 1) + j] == 'l'){
			for (j = 0; j < s3; j++){
				b = recvdata[(i + 1) + j + 1];
				dkc[j] = b;
			}
			dk = *(long*)dkc;
			if (recvdata[(i + 1) + (j + 1) + k] == 'd'){

				for (k = 0; k < dk; k++){
					c = recvdata[(i + 1) + (j + 1) + k + 1];
					datak[k] = c;
				}
			}
			else{
				return;
			}
		}
		else{
			return;
		}
	}
	else{
		return;
	}
}
int PacketReceiver::Connect(){
	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		int iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	return 404;
}
int PacketReceiver::OpenTcpConnection(char *ip, char* port){
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//printf("\n............. %s %s ................\n", ip, portnum);

	// Resolve the server address and port
	iResult = getaddrinfo(ip, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
}
void PacketReceiver::tcpReceiver(){
	long packetid = 0;
	int iResult;
	char fresult[4000];
	long id = 0, len = 0;
	BYTE * datak = new BYTE[4000];

	while (true){
		memset(fresult, 0, 4000);
		memset(datak, 0, 4000);
		iResult = send(ConnectSocket, (char*)&packetid, sizeof(long), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return;
		}

		if (packetid == 188){
			printf("\nID = %d\n", id);
		}
		iResult = recv(ConnectSocket, fresult, 4000, 0);
		if (iResult>1){
			parse(id, len, datak, (BYTE*)fresult);


			if (id < 0){
				delete[] datak;
				return;
			}
			if (id == packetid)packetid++;
			else packetid = id + 1;
		
			printf("\nID = %d\n", id);

		}
	}
	delete datak;
}

void PacketReceiver::udpReceiver(){
	int iResult;
	char fresult[4000];
	int addlen = sizeof(Remaddr);

	while (true){
		memset(fresult, 0, 4000);
		iResult = recvfrom(ConnectSocket, fresult, 2048, 0, (sockaddr*)&Remaddr, &addlen);
		if (iResult > 0) {

			//outputDevice->UpdateBuffer(true, new  OutputDataFrame((BYTE*)fresult, 1920));
		}

	}
}

PacketReceiver::~PacketReceiver()
{
}
